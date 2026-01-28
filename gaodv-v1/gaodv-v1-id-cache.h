/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * ID Cache - Duplicate RREQ Detection
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_ID_CACHE_H
#define GAODV_V1_ID_CACHE_H

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Unique packet identifier (origin address + request ID)
 */
class IdCache
{
public:
    /**
     * \brief Constructor
     * \param lifetime Cache entry lifetime
     */
    IdCache(Time lifetime);

    /**
     * \brief Check if (origin, id) pair is duplicate and insert if not
     * \param origin Originator address
     * \param id Request ID
     * \return true if duplicate, false if new entry
     */
    bool IsDuplicate(Ipv4Address origin, uint32_t id);

    /**
     * \brief Remove expired entries
     */
    void Purge();

    /**
     * \brief Get number of entries
     * \return Number of cached entries
     */
    uint32_t GetSize();

    /**
     * \brief Set cache entry lifetime
     * \param lifetime New lifetime
     */
    void SetLifetime(Time lifetime);

    /**
     * \brief Get cache entry lifetime
     * \return Current lifetime
     */
    Time GetLifetime() const;

private:
    /**
     * \brief Cache entry structure
     */
    struct UniqueId
    {
        Ipv4Address m_origin;  ///< Originator address
        uint32_t m_id;         ///< Request ID
        Time m_expire;         ///< Expiration time
    };

    /**
     * \brief Check if entry is expired
     * \param u Entry to check
     * \return true if expired
     */
    struct IsExpired
    {
        bool operator()(const UniqueId& u) const
        {
            return (u.m_expire < Simulator::Now());
        }
    };

    std::vector<UniqueId> m_idCache;  ///< Cache of seen (origin, id) pairs
    Time m_lifetime;                   ///< Cache entry lifetime
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_ID_CACHE_H
