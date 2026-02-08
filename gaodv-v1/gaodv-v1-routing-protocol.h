/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Main Routing Protocol Header
 * 
 * Original implementation following the G-AODV flowchart.
 * NOT based on gaodv-v0 - this is a fresh design.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_ROUTING_PROTOCOL_H
#define GAODV_V1_ROUTING_PROTOCOL_H

#include "gaodv-v1-dpd.h"
#include "gaodv-v1-id-cache.h"
#include "gaodv-v1-neighbors.h"
#include "gaodv-v1-packet.h"
#include "gaodv-v1-rqueue.h"
#include "gaodv-v1-rtable.h"

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"

#include <map>
#include <set>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief GAODV-V1 Routing Protocol
 * 
 * Implements the G-AODV flowchart with:
 * - Geometric awareness (coordinate-carrying RREQ packets)
 * - Energy-aware routing (REF/NEF mechanism)
 * - Destination-side RREQ buffering with timer T
 * - Highest REF route selection
 */
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /// Protocol port number
    static const uint32_t GAODVV1_PORT = 656;

    RoutingProtocol();
    ~RoutingProtocol() override;

    // Inherited from Ipv4RoutingProtocol
    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;

    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& header,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback& ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback& lcb,
                    const ErrorCallback& ecb) override;

    void NotifyInterfaceUp(uint32_t interface) override;
    void NotifyInterfaceDown(uint32_t interface) override;
    void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void SetIpv4(Ptr<Ipv4> ipv4) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S) const override;

    /**
     * \brief Assign a fixed random variable stream number
     * \param stream First stream index
     * \return Number of streams used
     */
    int64_t AssignStreams(int64_t stream);

    //=== PDR Metrics ===
    
    /**
     * \brief Get Packet Delivery Ratio
     * \return PDR value (0.0 to 1.0)
     */
    double GetPacketDeliveryRatio() const;
    
    uint32_t GetTotalPacketsSent() const { return m_packetsSent; }
    uint32_t GetTotalPacketsReceived() const { return m_receivedUids.size(); }

protected:
    void DoInitialize() override;
    void Start();
    void DoDispose() override;

private:
    //=== G-AODV Configurable Parameters (from Attributes) ===
    
    double m_alpha;              ///< Threshold factor α (0.5-0.7), default 0.6
    double m_ethf;               ///< Energy Threshold Factor
    double m_maxRange;           ///< Max communication range (meters)
    double m_initialEnergy;      ///< Initial energy for NEF calculation
    Time m_evaluationWindow;     ///< Timer T for destination RREQ collection
    
    // Weights removed per flowchart - path selection uses max RREQEF only
    
    //=== Standard AODV-like Parameters ===
    
    uint32_t m_rreqRetries;           ///< Max RREQ retries
    uint16_t m_ttlStart;              ///< Initial TTL
    uint16_t m_ttlIncrement;          ///< TTL increment per retry
    uint16_t m_ttlThreshold;          ///< TTL threshold for ring search
    uint16_t m_netDiameter;           ///< Network diameter
    uint16_t m_timeoutBuffer;         ///< Timeout buffer
    Time m_nodeTraversalTime;         ///< Node traversal time
    Time m_netTraversalTime;          ///< Network traversal time
    Time m_pathDiscoveryTime;         ///< Path discovery time
    Time m_myRouteTimeout;            ///< Destination route timeout
    Time m_helloInterval;             ///< Hello interval
    uint32_t m_allowedHelloLoss;      ///< Allowed hello loss
    Time m_activeRouteTimeout;        ///< Active route timeout
    Time m_deletePeriod;              ///< Delete period
    Time m_nextHopWait;               ///< Next hop wait time
    Time m_blackListTimeout;          ///< Blacklist timeout
    uint32_t m_maxQueueLen;           ///< Max queue length
    Time m_maxQueueTime;              ///< Max queue time
    uint32_t m_rreqRateLimit;         ///< RREQ rate limit
    uint32_t m_rerrRateLimit;         ///< RERR rate limit
    bool m_destinationOnly;           ///< Destination only flag
    bool m_gratuitousReply;           ///< Gratuitous RREP flag
    bool m_enableHello;               ///< Enable hello messages
    bool m_enableBroadcast;           ///< Enable broadcast

    //=== Destination-side RREQ Buffering (Flowchart: Timer T) ===
    
    /**
     * \brief Buffered RREQ entry at destination
     */
    struct BufferedRreq
    {
        RreqHeader header;       ///< RREQ header
        Ipv4Address sender;      ///< Immediate sender (for reverse path)
        Time recvTime;           ///< Reception time
        uint8_t hopCount;        ///< Hop count
        double distance;         ///< Distance from previous hop
    };
    
    std::map<uint64_t, BufferedRreq> m_bufferedRreqs;  ///< Buffered RREQs at destination
    EventId m_evaluationTimer;                          ///< Timer T event

    //=== PDR Tracking ===
    
    uint32_t m_packetsSent;           ///< Total packets sent
    std::set<uint64_t> m_receivedUids;  ///< Unique received packet UIDs

    //=== Core Components ===
    
    Ptr<Ipv4> m_ipv4;                                    ///< IPv4 reference
    Ptr<Node> m_node;                                    ///< Node reference
    RoutingTable m_routingTable;                         ///< Routing table
    PacketQueue m_queue;                                 ///< Packet queue
    IdCache m_rreqIdCache;                               ///< RREQ ID cache
    Neighbors m_nb;                                      ///< Neighbors
    DuplicatePacketDetection m_dpd;                      ///< DPD
    Ptr<NetDevice> m_lo;                                 ///< Loopback device
    
    std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;           ///< Socket to address
    std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketSubnetBroadcastAddresses;  ///< Subnet sockets
    
    uint32_t m_requestId;   ///< Request ID counter
    uint32_t m_seqNo;       ///< Sequence number
    uint32_t m_rreqCount;   ///< RREQ count for rate limiting
    uint32_t m_rerrCount;   ///< RERR count for rate limiting
    
    //=== Timers ===
    
    Timer m_htimer;                         ///< Hello timer
    Timer m_rreqRateLimitTimer;             ///< RREQ rate limit timer
    Timer m_rerrRateLimitTimer;             ///< RERR rate limit timer
    std::map<Ipv4Address, Timer> m_addressReqTimer;  ///< Address request timers
    
    Time m_lastBcastTime;                   ///< Last broadcast time
    Ptr<UniformRandomVariable> m_uniformRandomVariable;  ///< Random variable

    //==========================================================================
    // GAODV-V1 Core Protocol Methods (Flowchart Implementation)
    //==========================================================================
    
    /**
     * \brief Start route discovery (Flowchart: "Start discovery Flooding RREQ with (x,y,z) & RREQEF")
     * \param dst Destination address
     */
    void SendRequest(Ipv4Address dst);
    
    /**
     * \brief Process incoming RREQ (implements full G-AODV flowchart logic)
     * \param p Packet
     * \param receiver Receiver address
     * \param sender Sender address
     */
    void RecvRequest(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender);
    
    /**
     * \brief Send RREP back to origin (destination node)
     * \param rreq RREQ header
     * \param toOrigin Route to origin
     */
    void SendReply(const RreqHeader& rreq, const RoutingTableEntry& toOrigin);
    
    /**
     * \brief Intermediate node sends RREP (has valid route to destination)
     * \param toDst Route to destination
     * \param toOrigin Route to origin
     * \param gratuitousRrep Send gratuitous RREP
     */
    void SendReplyByIntermediateNode(RoutingTableEntry& toDst,
                                     RoutingTableEntry& toOrigin,
                                     bool gratuitousRrep);
    
    /**
     * \brief Evaluate buffered RREQs and send RREP via best path
     * (Flowchart: Timer T expired → "RREP to be sent to source with high REF")
     */
    void EvaluateAndSendRrep();
    
    /**
     * \brief Process incoming RREP
     * \param p Packet
     * \param receiver Receiver address
     * \param sender Sender address
     */
    void RecvReply(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender);
    
    /**
     * \brief Send RREP acknowledgment
     * \param neighbor Neighbor address
     */
    void SendReplyAck(Ipv4Address neighbor);
    
    /**
     * \brief Process RREP acknowledgment
     * \param neighbor Neighbor address
     */
    void RecvReplyAck(Ipv4Address neighbor);
    
    /**
     * \brief Send Route Error
     * \param unreachable Unreachable destination map
     */
    void SendRerrWhenBreaksLinkToNextHop(Ipv4Address nextHop);
    
    /**
     * \brief Process incoming RERR
     * \param p Packet
     * \param sender Sender address
     */
    void RecvError(Ptr<Packet> p, Ipv4Address sender);

    //==========================================================================
    // G-AODV Helper Methods
    //==========================================================================
    
    /**
     * \brief Compute Node Efficiency Factor (NEF)
     * NEF = residual_energy / initial_energy
     * \return NEF value (0.0 to 1.0)
     */
    double ComputeNef() const;
    
    // ComputeRef removed - path selection uses RREQEF directly per flowchart
    
    /**
     * \brief Get current node's 3D position
     * \return Position vector
     */
    Vector GetPosition() const;
    
    /**
     * \brief Calculate Euclidean distance between two 3D points
     * \param a First point
     * \param b Second point
     * \return Distance
     */
    static double CalculateDistance(const Vector& a, const Vector& b);
    
    /**
     * \brief Check if address belongs to this node
     * \param addr Address to check
     * \return true if own address
     */
    bool IsMyOwnAddress(Ipv4Address addr) const;
    
    /**
     * \brief Create unique key for RREQ (origin, id)
     * \param origin Origin address
     * \param id Request ID
     * \return 64-bit key
     */
    uint64_t MakeRreqKey(Ipv4Address origin, uint32_t id) const;

    //==========================================================================
    // Standard AODV-like Helper Methods
    //==========================================================================
    
    void RecvGaodvv1(Ptr<Socket> socket);
    Ptr<Socket> FindSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const;
    Ptr<Socket> FindSubnetBroadcastSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const;
    bool UpdateRouteLifeTime(Ipv4Address addr, Time lifetime);
    void UpdateRouteToNeighbor(Ipv4Address sender, Ipv4Address receiver);
    Ptr<Ipv4Route> LoopbackRoute(const Ipv4Header& header, Ptr<NetDevice> oif) const;
    bool Forwarding(Ptr<const Packet> p, const Ipv4Header& header,
                    UnicastForwardCallback ucb, ErrorCallback ecb);
    void SendPacketFromQueue(Ipv4Address dst, Ptr<Ipv4Route> route);
    void SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination);
    void ScheduleRreqRetry(Ipv4Address dst);
    void RouteRequestTimerExpire(Ipv4Address dst);
    void HelloTimerExpire();
    void SendHello();
    void ProcessHello(const RrepHeader& rrepHeader, Ipv4Address receiver);
    void AckTimerExpire(Ipv4Address neighbor, Time blacklistTimeout);
    void NotifyTxError(Ptr<const Packet> p);
    void HandleLinkFailure(Ipv4Address addr);
    void RerrRateLimitTimerExpire();
    void RreqRateLimitTimerExpire();
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_ROUTING_PROTOCOL_H
