/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Routing Table Implementation
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-rtable.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1RoutingTable");

//-----------------------------------------------------------------------------
// RoutingTableEntry Implementation
//-----------------------------------------------------------------------------

RoutingTableEntry::RoutingTableEntry(Ptr<NetDevice> dev,
                                     Ipv4Address dst,
                                     bool vSeqNo,
                                     uint32_t seqNo,
                                     Ipv4InterfaceAddress iface,
                                     uint16_t hops,
                                     Ipv4Address nextHop,
                                     Time lifetime)
    : m_dst(dst),
      m_iface(iface),
      m_validSeqNo(vSeqNo),
      m_seqNo(seqNo),
      m_hops(hops),
      m_lifeTime(lifetime + Simulator::Now()),
      m_flag(VALID),
      m_reqCount(0),
      m_blackListState(false),
      m_blackListTimeout(Simulator::Now()),
      m_ackTimer(Timer::CANCEL_ON_DESTROY)
{
    m_route = Create<Ipv4Route>();
    m_route->SetDestination(dst);
    m_route->SetGateway(nextHop);
    m_route->SetSource(iface.GetLocal());
    m_route->SetOutputDevice(dev);
}

RoutingTableEntry::~RoutingTableEntry()
{
}

void
RoutingTableEntry::SetNextHop(Ipv4Address nextHop)
{
    m_route->SetGateway(nextHop);
}

Ipv4Address
RoutingTableEntry::GetNextHop() const
{
    return m_route->GetGateway();
}

void
RoutingTableEntry::SetOutputDevice(Ptr<NetDevice> dev)
{
    m_route->SetOutputDevice(dev);
}

Ptr<NetDevice>
RoutingTableEntry::GetOutputDevice() const
{
    return m_route->GetOutputDevice();
}

bool
RoutingTableEntry::InsertPrecursor(Ipv4Address id)
{
    if (m_precursorList.find(id) != m_precursorList.end())
    {
        return true;  // Already exists
    }
    m_precursorList.insert(id);
    return true;
}

bool
RoutingTableEntry::LookupPrecursor(Ipv4Address id)
{
    return (m_precursorList.find(id) != m_precursorList.end());
}

bool
RoutingTableEntry::DeletePrecursor(Ipv4Address id)
{
    auto it = m_precursorList.find(id);
    if (it != m_precursorList.end())
    {
        m_precursorList.erase(it);
        return true;
    }
    return false;
}

void
RoutingTableEntry::DeleteAllPrecursors()
{
    m_precursorList.clear();
}

bool
RoutingTableEntry::IsPrecursorListEmpty() const
{
    return m_precursorList.empty();
}

void
RoutingTableEntry::GetPrecursors(std::vector<Ipv4Address>& prec) const
{
    prec.clear();
    for (const auto& addr : m_precursorList)
    {
        prec.push_back(addr);
    }
}

bool
RoutingTableEntry::operator==(const RoutingTableEntry& o) const
{
    return (m_dst == o.m_dst && m_seqNo == o.m_seqNo &&
            m_hops == o.m_hops && m_flag == o.m_flag);
}

void
RoutingTableEntry::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    std::ostream* os = stream->GetStream();
    *os << std::setiosflags(std::ios::fixed);
    
    *os << m_dst << "\t" << GetNextHop() << "\t" 
        << m_iface.GetLocal() << "\t"
        << std::setw(2) << m_hops << "\t"
        << std::setw(6) << m_seqNo << "\t"
        << std::setprecision(3) << (m_lifeTime - Simulator::Now()).As(unit) << "\t"
        << (m_flag == VALID ? "UP" : (m_flag == IN_SEARCH ? "SRCH" : "DOWN"))
        << "\n";
}

//-----------------------------------------------------------------------------
// RoutingTable Implementation
//-----------------------------------------------------------------------------

RoutingTable::RoutingTable(Time badLinkLifetime)
    : m_badLinkLifetime(badLinkLifetime)
{
}

bool
RoutingTable::AddRoute(RoutingTableEntry& rt)
{
    NS_LOG_FUNCTION(this << rt.GetDestination());
    
    Purge();
    
    if (m_table.find(rt.GetDestination()) != m_table.end())
    {
        NS_LOG_DEBUG("Route to " << rt.GetDestination() << " already exists");
        return false;
    }
    
    m_table[rt.GetDestination()] = rt;
    NS_LOG_DEBUG("Added route to " << rt.GetDestination());
    return true;
}

bool
RoutingTable::LookupRoute(Ipv4Address dst, RoutingTableEntry& rt)
{
    NS_LOG_FUNCTION(this << dst);
    
    Purge();
    
    auto it = m_table.find(dst);
    if (it == m_table.end())
    {
        NS_LOG_DEBUG("Route to " << dst << " not found");
        return false;
    }
    
    rt = it->second;
    NS_LOG_DEBUG("Route to " << dst << " found");
    return true;
}

bool
RoutingTable::LookupValidRoute(Ipv4Address dst, RoutingTableEntry& rt)
{
    NS_LOG_FUNCTION(this << dst);
    
    if (!LookupRoute(dst, rt))
    {
        return false;
    }
    
    if (rt.GetFlag() != VALID)
    {
        NS_LOG_DEBUG("Route to " << dst << " is not valid");
        return false;
    }
    
    return true;
}

bool
RoutingTable::DeleteRoute(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    
    auto it = m_table.find(dst);
    if (it == m_table.end())
    {
        return false;
    }
    
    m_table.erase(it);
    return true;
}

bool
RoutingTable::Update(RoutingTableEntry& rt)
{
    NS_LOG_FUNCTION(this << rt.GetDestination());
    
    auto it = m_table.find(rt.GetDestination());
    if (it == m_table.end())
    {
        NS_LOG_DEBUG("Route to " << rt.GetDestination() << " not found for update");
        return false;
    }
    
    it->second = rt;
    return true;
}

bool
RoutingTable::SetEntryState(Ipv4Address dst, RouteFlags state)
{
    NS_LOG_FUNCTION(this << dst << state);
    
    auto it = m_table.find(dst);
    if (it == m_table.end())
    {
        return false;
    }
    
    it->second.SetFlag(state);
    return true;
}

void
RoutingTable::DeleteAllRoutesFromInterface(Ipv4InterfaceAddress iface)
{
    NS_LOG_FUNCTION(this << iface.GetLocal());
    
    for (auto it = m_table.begin(); it != m_table.end();)
    {
        if (it->second.GetInterface() == iface)
        {
            it = m_table.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
RoutingTable::Clear()
{
    m_table.clear();
}

void
RoutingTable::Purge()
{
    if (m_table.empty())
    {
        return;
    }
    
    for (auto it = m_table.begin(); it != m_table.end();)
    {
        if (it->second.GetLifeTime() < Seconds(0) && it->second.GetFlag() != IN_SEARCH)
        {
            // Check blacklist
            if (it->second.IsUnidirectional())
            {
                if (it->second.GetBlacklistTimeout() < Simulator::Now())
                {
                    NS_LOG_DEBUG("Removing blacklisted route to " << it->first);
                    it = m_table.erase(it);
                    continue;
                }
            }
            else if (it->second.GetFlag() == INVALID)
            {
                it = m_table.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void
RoutingTable::MarkLinkAsUnidirectional(Ipv4Address nextHop, Time blacklistTimeout)
{
    NS_LOG_FUNCTION(this << nextHop);
    
    for (auto& entry : m_table)
    {
        if (entry.second.GetNextHop() == nextHop)
        {
            entry.second.SetUnidirectional(true);
            entry.second.SetBlacklistTimeout(Simulator::Now() + blacklistTimeout);
        }
    }
}

void
RoutingTable::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    std::ostream* os = stream->GetStream();
    *os << "\nGAODV-V1 Routing Table\n";
    *os << "Destination\tNextHop\t\tInterface\tHops\tSeqNo\tLifeTime\tState\n";
    
    for (const auto& entry : m_table)
    {
        entry.second.Print(stream, unit);
    }
    *os << "\n";
}

void
RoutingTable::GetListOfDestinationsWithNextHop(Ipv4Address nextHop,
                                                std::map<Ipv4Address, uint32_t>& unreachable)
{
    NS_LOG_FUNCTION(this << nextHop);
    
    Purge();
    unreachable.clear();
    
    for (const auto& entry : m_table)
    {
        if (entry.second.GetNextHop() == nextHop)
        {
            unreachable[entry.first] = entry.second.GetSeqNo();
        }
    }
}

void
RoutingTable::InvalidateRoutesWithDst(const std::map<Ipv4Address, uint32_t>& unreachable)
{
    NS_LOG_FUNCTION(this);
    
    for (const auto& un : unreachable)
    {
        auto it = m_table.find(un.first);
        if (it != m_table.end())
        {
            it->second.SetFlag(INVALID);
            it->second.SetSeqNo(un.second);
        }
    }
}

} // namespace gaodvv1
} // namespace ns3
