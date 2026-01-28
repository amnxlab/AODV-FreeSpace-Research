/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Duplicate Packet Detection
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_DPD_H
#define GAODV_V1_DPD_H

#include "ns3/ipv4-header.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"

#include <map>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Duplicate Packet Detection for GAODV-V1
 * 
 * Detects duplicate packets based on source address, destination address,
 * identification field, and protocol.
 */
class DuplicatePacketDetection
{
public:
    /**
     * \brief Constructor
     * \param lifetime Cache entry lifetime
     */
    DuplicatePacketDetection(Time lifetime);

    /**
     * \brief Check if packet is duplicate
     * \param p Packet
     * \param header IP header
     * \return true if duplicate
     */
    bool IsDuplicate(Ptr<const Packet> p, const Ipv4Header& header);

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
     * \brief Cache key for duplicate detection
     */
    struct DpdKey
    {
        Ipv4Address m_source;
        Ipv4Address m_destination;
        uint16_t m_identification;
        uint8_t m_protocol;

        bool operator<(const DpdKey& o) const;
    };

    std::map<DpdKey, Time> m_cache;  ///< Duplicate detection cache
    Time m_lifetime;                  ///< Cache entry lifetime
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_DPD_H
