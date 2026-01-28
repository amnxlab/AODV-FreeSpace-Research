/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * ID Cache Implementation
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-id-cache.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1IdCache");

IdCache::IdCache(Time lifetime)
    : m_lifetime(lifetime)
{
}

bool
IdCache::IsDuplicate(Ipv4Address origin, uint32_t id)
{
    NS_LOG_FUNCTION(this << origin << id);
    
    Purge();
    
    for (const auto& entry : m_idCache)
    {
        if (entry.m_origin == origin && entry.m_id == id)
        {
            NS_LOG_DEBUG("Duplicate found: " << origin << ":" << id);
            return true;
        }
    }
    
    // Not found - add new entry
    UniqueId newEntry;
    newEntry.m_origin = origin;
    newEntry.m_id = id;
    newEntry.m_expire = Simulator::Now() + m_lifetime;
    m_idCache.push_back(newEntry);
    
    NS_LOG_DEBUG("New entry added: " << origin << ":" << id);
    return false;
}

void
IdCache::Purge()
{
    m_idCache.erase(
        std::remove_if(m_idCache.begin(), m_idCache.end(), IsExpired()),
        m_idCache.end());
}

uint32_t
IdCache::GetSize()
{
    Purge();
    return m_idCache.size();
}

void
IdCache::SetLifetime(Time lifetime)
{
    m_lifetime = lifetime;
}

Time
IdCache::GetLifetime() const
{
    return m_lifetime;
}

} // namespace gaodvv1
} // namespace ns3
