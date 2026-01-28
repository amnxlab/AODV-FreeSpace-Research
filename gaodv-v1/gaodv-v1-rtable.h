/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Routing Table
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_RTABLE_H
#define GAODV_V1_RTABLE_H

#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/timer.h"

#include <map>
#include <set>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Route flags
 */
enum RouteFlags
{
    VALID = 0,      ///< Valid route
    INVALID = 1,    ///< Invalid route
    IN_SEARCH = 2   ///< Route discovery in progress
};

/**
 * \ingroup gaodvv1
 * \brief Routing table entry
 */
class RoutingTableEntry
{
public:
    /**
     * \brief Default constructor
     */
    RoutingTableEntry(Ptr<NetDevice> dev = nullptr,
                      Ipv4Address dst = Ipv4Address(),
                      bool vSeqNo = false,
                      uint32_t seqNo = 0,
                      Ipv4InterfaceAddress iface = Ipv4InterfaceAddress(),
                      uint16_t hops = 0,
                      Ipv4Address nextHop = Ipv4Address(),
                      Time lifetime = Simulator::Now());

    ~RoutingTableEntry();

    // Getters and setters
    Ipv4Address GetDestination() const { return m_dst; }
    Ptr<Ipv4Route> GetRoute() const { return m_route; }
    void SetRoute(Ptr<Ipv4Route> rt) { m_route = rt; }
    void SetNextHop(Ipv4Address nextHop);
    Ipv4Address GetNextHop() const;
    void SetOutputDevice(Ptr<NetDevice> dev);
    Ptr<NetDevice> GetOutputDevice() const;
    Ipv4InterfaceAddress GetInterface() const { return m_iface; }
    void SetInterface(Ipv4InterfaceAddress iface) { m_iface = iface; }
    void SetValidSeqNo(bool s) { m_validSeqNo = s; }
    bool GetValidSeqNo() const { return m_validSeqNo; }
    void SetSeqNo(uint32_t sn) { m_seqNo = sn; }
    uint32_t GetSeqNo() const { return m_seqNo; }
    void SetHop(uint16_t hop) { m_hops = hop; }
    uint16_t GetHop() const { return m_hops; }
    void SetLifeTime(Time lt) { m_lifeTime = lt + Simulator::Now(); }
    Time GetLifeTime() const { return m_lifeTime - Simulator::Now(); }
    void SetFlag(RouteFlags flag) { m_flag = flag; }
    RouteFlags GetFlag() const { return m_flag; }
    void SetRreqCnt(uint8_t n) { m_reqCount = n; }
    uint8_t GetRreqCnt() const { return m_reqCount; }
    void IncrementRreqCnt() { m_reqCount++; }

    /**
     * \brief Set route as unidirectional (blacklist)
     * \param u True to set unidirectional
     */
    void SetUnidirectional(bool u) { m_blackListState = u; }
    bool IsUnidirectional() const { return m_blackListState; }
    void SetBlacklistTimeout(Time t) { m_blackListTimeout = t; }
    Time GetBlacklistTimeout() const { return m_blackListTimeout; }

    /**
     * \brief Precursor list management
     */
    bool InsertPrecursor(Ipv4Address id);
    bool LookupPrecursor(Ipv4Address id);
    bool DeletePrecursor(Ipv4Address id);
    void DeleteAllPrecursors();
    bool IsPrecursorListEmpty() const;
    void GetPrecursors(std::vector<Ipv4Address>& prec) const;

    /**
     * \brief Compare entries
     */
    bool operator==(const RoutingTableEntry& o) const;

    /**
     * \brief Print entry
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

    /// Timer for RREP-ACK
    Timer m_ackTimer;

private:
    Ipv4Address m_dst;            ///< Destination IP
    Ptr<Ipv4Route> m_route;       ///< Route to destination
    Ipv4InterfaceAddress m_iface; ///< Output interface
    bool m_validSeqNo;            ///< Valid sequence number flag
    uint32_t m_seqNo;             ///< Destination sequence number
    uint16_t m_hops;              ///< Hop count
    Time m_lifeTime;              ///< Route lifetime
    RouteFlags m_flag;            ///< Route flag
    uint8_t m_reqCount;           ///< RREQ retry count
    bool m_blackListState;        ///< Unidirectional link flag
    Time m_blackListTimeout;      ///< Blacklist timeout
    std::set<Ipv4Address> m_precursorList;  ///< Precursor list
};

/**
 * \ingroup gaodvv1
 * \brief Routing table for GAODV-V1
 */
class RoutingTable
{
public:
    /**
     * \brief Constructor
     * \param badLinkLifetime Lifetime for bad link detection
     */
    RoutingTable(Time badLinkLifetime);

    /**
     * \brief Add route entry
     * \param rt Entry to add
     * \return true on success
     */
    bool AddRoute(RoutingTableEntry& rt);

    /**
     * \brief Lookup route for destination
     * \param dst Destination address
     * \param rt Output entry
     * \return true if found
     */
    bool LookupRoute(Ipv4Address dst, RoutingTableEntry& rt);

    /**
     * \brief Lookup valid route for destination
     * \param dst Destination address
     * \param rt Output entry
     * \return true if valid route found
     */
    bool LookupValidRoute(Ipv4Address dst, RoutingTableEntry& rt);

    /**
     * \brief Delete route
     * \param dst Destination address
     * \return true if deleted
     */
    bool DeleteRoute(Ipv4Address dst);

    /**
     * \brief Update existing route
     * \param rt Updated entry
     * \return true on success
     */
    bool Update(RoutingTableEntry& rt);

    /**
     * \brief Set route as invalid
     * \param dst Destination address
     * \param unreachable Output list of unreachable destinations
     * \return true on success
     */
    bool SetEntryState(Ipv4Address dst, RouteFlags state);

    /**
     * \brief Delete all routes from interface
     * \param iface Interface address
     */
    void DeleteAllRoutesFromInterface(Ipv4InterfaceAddress iface);

    /**
     * \brief Clear all routes
     */
    void Clear();

    /**
     * \brief Purge expired routes
     */
    void Purge();

    /**
     * \brief Mark entries using next hop as invalid
     * \param nextHop Next hop address
     * \param unreachable Output list of unreachable destinations
     */
    void MarkLinkAsUnidirectional(Ipv4Address nextHop, Time blacklistTimeout);

    /**
     * \brief Print routing table
     */
    void Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

    /**
     * \brief Get list of destinations affected by broken link
     * \param nextHop Broken next hop
     * \param unreachable Output list
     */
    void GetListOfDestinationsWithNextHop(Ipv4Address nextHop,
                                          std::map<Ipv4Address, uint32_t>& unreachable);

    /**
     * \brief Invalidate routes through broken link
     * \param unreachable Map of unreachable destinations
     */
    void InvalidateRoutesWithDst(const std::map<Ipv4Address, uint32_t>& unreachable);

private:
    std::map<Ipv4Address, RoutingTableEntry> m_table;  ///< The routing table
    Time m_badLinkLifetime;                             ///< Bad link lifetime
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_RTABLE_H
