/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Neighbor Management Implementation
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-neighbors.h"

#include "ns3/log.h"
#include "ns3/wifi-mac-header.h"

#include <algorithm>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1Neighbors");

Neighbors::Neighbors(Time delay)
    : m_timer(Timer::CANCEL_ON_DESTROY),
      m_delay(delay)
{
    m_timer.SetDelay(m_delay);
    m_timer.SetFunction(&Neighbors::Purge, this);
}

bool
Neighbors::IsNeighbor(Ipv4Address addr)
{
    Purge();
    
    for (const auto& neighbor : m_neighbors)
    {
        if (neighbor.m_neighborAddress == addr)
        {
            return true;
        }
    }
    return false;
}

void
Neighbors::Update(Ipv4Address addr, Time expire)
{
    NS_LOG_FUNCTION(this << addr << expire);
    
    for (auto& neighbor : m_neighbors)
    {
        if (neighbor.m_neighborAddress == addr)
        {
            neighbor.m_expireTime = std::max(expire + Simulator::Now(), 
                                             neighbor.m_expireTime);
            if (neighbor.m_hardwareAddress == Mac48Address())
            {
                neighbor.m_hardwareAddress = LookupMacAddress(addr);
            }
            return;
        }
    }
    
    // New neighbor
    NS_LOG_DEBUG("Adding new neighbor: " << addr);
    Neighbor neighbor(addr, LookupMacAddress(addr), expire + Simulator::Now());
    m_neighbors.push_back(neighbor);
    
    Purge();
}

void
Neighbors::AddArpCache(Ptr<ArpCache> arp)
{
    m_arp.push_back(arp);
}

void
Neighbors::DelArpCache(Ptr<ArpCache> arp)
{
    m_arp.erase(std::remove(m_arp.begin(), m_arp.end(), arp), m_arp.end());
}

Callback<void, const WifiMacHeader&>
Neighbors::GetTxErrorCallback() const
{
    return MakeCallback(&Neighbors::ProcessTxError, const_cast<Neighbors*>(this));
}

void
Neighbors::SetCallback(Callback<void, Ipv4Address> cb)
{
    m_handleLinkFailure = cb;
}

void
Neighbors::GetListOfNeighbors(std::vector<Ipv4Address>& neighbors)
{
    Purge();
    neighbors.clear();
    
    for (const auto& neighbor : m_neighbors)
    {
        neighbors.push_back(neighbor.m_neighborAddress);
    }
}

void
Neighbors::Clear()
{
    m_neighbors.clear();
}

void
Neighbors::Purge()
{
    if (m_neighbors.empty())
    {
        return;
    }
    
    Time now = Simulator::Now();
    
    for (auto it = m_neighbors.begin(); it != m_neighbors.end();)
    {
        if (it->m_expireTime < now)
        {
            it = m_neighbors.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
Neighbors::ScheduleTimer()
{
    m_timer.Cancel();
    m_timer.Schedule();
}

void
Neighbors::ProcessTxError(const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << hdr.GetAddr1());
    
    Mac48Address addr = hdr.GetAddr1();
    
    for (auto it = m_neighbors.begin(); it != m_neighbors.end(); ++it)
    {
        if (it->m_hardwareAddress == addr)
        {
            NS_LOG_DEBUG("Link failure detected for " << it->m_neighborAddress);
            
            if (!it->m_close)
            {
                it->m_close = true;
                
                if (!m_handleLinkFailure.IsNull())
                {
                    m_handleLinkFailure(it->m_neighborAddress);
                }
            }
            return;
        }
    }
}

Mac48Address
Neighbors::LookupMacAddress(Ipv4Address addr)
{
    for (const auto& arp : m_arp)
    {
        ArpCache::Entry* entry = arp->Lookup(addr);
        if (entry != nullptr && entry->IsAlive() && !entry->IsExpired())
        {
            return Mac48Address::ConvertFrom(entry->GetMacAddress());
        }
    }
    return Mac48Address();
}

} // namespace gaodvv1
} // namespace ns3
