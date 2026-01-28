/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Neighbor Management
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_NEIGHBORS_H
#define GAODV_V1_NEIGHBORS_H

#include "ns3/arp-cache.h"
#include "ns3/callback.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/timer.h"
#include "ns3/wifi-mac-header.h"

#include <vector>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Neighbor management for GAODV-V1
 */
class Neighbors
{
public:
    /**
     * \brief Constructor
     * \param delay Callback delay
     */
    Neighbors(Time delay);

    /**
     * \brief Neighbor entry
     */
    struct Neighbor
    {
        Ipv4Address m_neighborAddress;  ///< Neighbor address
        Mac48Address m_hardwareAddress; ///< MAC address
        Time m_expireTime;              ///< Expire time
        bool m_close;                   ///< Close flag (being deleted)

        Neighbor(Ipv4Address ip, Mac48Address mac, Time t)
            : m_neighborAddress(ip),
              m_hardwareAddress(mac),
              m_expireTime(t),
              m_close(false)
        {
        }
    };

    /**
     * \brief Check if neighbor exists
     * \param addr Neighbor address
     * \return true if exists
     */
    bool IsNeighbor(Ipv4Address addr);

    /**
     * \brief Update neighbor entry
     * \param addr Neighbor address
     * \param expire New expire time
     */
    void Update(Ipv4Address addr, Time expire);

    /**
     * \brief Add ARP cache for layer 2 notifications
     * \param arp ARP cache
     */
    void AddArpCache(Ptr<ArpCache> arp);

    /**
     * \brief Remove ARP cache
     * \param arp ARP cache
     */
    void DelArpCache(Ptr<ArpCache> arp);

    /**
     * \brief Get callback for TX error notification
     */
    Callback<void, const WifiMacHeader&> GetTxErrorCallback() const;

    /**
     * \brief Set link failure callback
     * \param cb Callback
     */
    void SetCallback(Callback<void, Ipv4Address> cb);

    /**
     * \brief Get neighbors list
     * \param neighbors Output list
     */
    void GetListOfNeighbors(std::vector<Ipv4Address>& neighbors);

    /**
     * \brief Clear all neighbors
     */
    void Clear();

    /**
     * \brief Purge expired neighbors
     */
    void Purge();

    /**
     * \brief Schedule neighbor timer
     */
    void ScheduleTimer();

private:
    /**
     * \brief Process TX error (link failure)
     * \param hdr MAC header
     */
    void ProcessTxError(const WifiMacHeader& hdr);

    /**
     * \brief Lookup MAC address for IP
     * \param addr IP address
     * \param mac Output MAC address
     * \return true if found
     */
    Mac48Address LookupMacAddress(Ipv4Address addr);

    std::vector<Neighbor> m_neighbors;           ///< Neighbor list
    std::vector<Ptr<ArpCache>> m_arp;            ///< ARP caches
    Callback<void, Ipv4Address> m_handleLinkFailure;  ///< Link failure callback
    Timer m_timer;                                ///< Purge timer
    Time m_delay;                                 ///< Timer delay
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_NEIGHBORS_H
