/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Duplicate Packet Detection Implementation
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-dpd.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1Dpd");

bool
DuplicatePacketDetection::DpdKey::operator<(const DpdKey& o) const
{
    if (m_source.Get() < o.m_source.Get())
        return true;
    if (m_source.Get() > o.m_source.Get())
        return false;
    if (m_destination.Get() < o.m_destination.Get())
        return true;
    if (m_destination.Get() > o.m_destination.Get())
        return false;
    if (m_identification < o.m_identification)
        return true;
    if (m_identification > o.m_identification)
        return false;
    return m_protocol < o.m_protocol;
}

DuplicatePacketDetection::DuplicatePacketDetection(Time lifetime)
    : m_lifetime(lifetime)
{
}

bool
DuplicatePacketDetection::IsDuplicate(Ptr<const Packet> p, const Ipv4Header& header)
{
    NS_LOG_FUNCTION(this << p->GetUid());
    
    // Purge expired entries
    Time now = Simulator::Now();
    for (auto it = m_cache.begin(); it != m_cache.end();)
    {
        if (it->second < now)
        {
            it = m_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
    
    // Create key
    DpdKey key;
    key.m_source = header.GetSource();
    key.m_destination = header.GetDestination();
    key.m_identification = header.GetIdentification();
    key.m_protocol = header.GetProtocol();
    
    // Check if exists
    auto it = m_cache.find(key);
    if (it != m_cache.end())
    {
        NS_LOG_DEBUG("Duplicate packet detected");
        return true;
    }
    
    // Add to cache
    m_cache[key] = now + m_lifetime;
    return false;
}

void
DuplicatePacketDetection::SetLifetime(Time lifetime)
{
    m_lifetime = lifetime;
}

Time
DuplicatePacketDetection::GetLifetime() const
{
    return m_lifetime;
}

} // namespace gaodvv1
} // namespace ns3
