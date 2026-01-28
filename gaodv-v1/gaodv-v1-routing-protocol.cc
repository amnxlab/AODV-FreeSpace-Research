/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Main Routing Protocol Implementation - Part 1 (Core)
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-routing-protocol.h"

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/energy-source.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <cmath>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1RoutingProtocol");
NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);

TypeId
RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::gaodvv1::RoutingProtocol")
            .SetParent<Ipv4RoutingProtocol>()
            .SetGroupName("Gaodvv1")
            .AddConstructor<RoutingProtocol>()
            // G-AODV specific attributes
            .AddAttribute("Alpha",
                          "Threshold factor (0.4-0.8)",
                          DoubleValue(0.6),
                          MakeDoubleAccessor(&RoutingProtocol::m_alpha),
                          MakeDoubleChecker<double>(0.4, 0.8))
            .AddAttribute("Ethf",
                          "Energy Threshold Factor",
                          DoubleValue(0.2),
                          MakeDoubleAccessor(&RoutingProtocol::m_ethf),
                          MakeDoubleChecker<double>())
            .AddAttribute("MaxRange",
                          "Max communication range (m)",
                          DoubleValue(100.0),
                          MakeDoubleAccessor(&RoutingProtocol::m_maxRange),
                          MakeDoubleChecker<double>())
            .AddAttribute("InitialEnergy",
                          "Initial energy for NEF calculation (J)",
                          DoubleValue(500.0),
                          MakeDoubleAccessor(&RoutingProtocol::m_initialEnergy),
                          MakeDoubleChecker<double>())
            .AddAttribute("EvaluationWindow",
                          "Timer T for RREQ collection at destination",
                          TimeValue(Seconds(2.0)),
                          MakeTimeAccessor(&RoutingProtocol::m_evaluationWindow),
                          MakeTimeChecker())
            .AddAttribute("W1", "REF weight for RREQEF",
                          DoubleValue(0.25),
                          MakeDoubleAccessor(&RoutingProtocol::m_w1),
                          MakeDoubleChecker<double>())
            .AddAttribute("W2", "REF weight for 1/distance",
                          DoubleValue(0.25),
                          MakeDoubleAccessor(&RoutingProtocol::m_w2),
                          MakeDoubleChecker<double>())
            .AddAttribute("W3", "REF weight for 1/hopCount",
                          DoubleValue(0.25),
                          MakeDoubleAccessor(&RoutingProtocol::m_w3),
                          MakeDoubleChecker<double>())
            .AddAttribute("W4", "REF weight for 1/delay",
                          DoubleValue(0.25),
                          MakeDoubleAccessor(&RoutingProtocol::m_w4),
                          MakeDoubleChecker<double>())
            .AddAttribute("PropagationSpeed",
                          "Signal propagation speed (m/s)",
                          DoubleValue(1500.0),
                          MakeDoubleAccessor(&RoutingProtocol::m_propagationSpeed),
                          MakeDoubleChecker<double>())
            // Standard AODV-like attributes
            .AddAttribute("HelloInterval",
                          "Hello interval",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&RoutingProtocol::m_helloInterval),
                          MakeTimeChecker())
            .AddAttribute("RreqRetries",
                          "Max RREQ retries",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_rreqRetries),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("ActiveRouteTimeout",
                          "Active route timeout",
                          TimeValue(Seconds(3)),
                          MakeTimeAccessor(&RoutingProtocol::m_activeRouteTimeout),
                          MakeTimeChecker())
            .AddAttribute("NetDiameter",
                          "Network diameter",
                          UintegerValue(35),
                          MakeUintegerAccessor(&RoutingProtocol::m_netDiameter),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("NodeTraversalTime",
                          "Node traversal time",
                          TimeValue(MilliSeconds(40)),
                          MakeTimeAccessor(&RoutingProtocol::m_nodeTraversalTime),
                          MakeTimeChecker())
            .AddAttribute("AllowedHelloLoss",
                          "Allowed hello loss",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_allowedHelloLoss),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("EnableHello",
                          "Enable hello messages",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::m_enableHello),
                          MakeBooleanChecker())
            .AddAttribute("EnableBroadcast",
                          "Enable broadcast",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::m_enableBroadcast),
                          MakeBooleanChecker())
            .AddAttribute("DestinationOnly",
                          "Destination only flag",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RoutingProtocol::m_destinationOnly),
                          MakeBooleanChecker())
            .AddAttribute("GratuitousReply",
                          "Gratuitous RREP",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::m_gratuitousReply),
                          MakeBooleanChecker());
    return tid;
}

RoutingProtocol::RoutingProtocol()
    : m_alpha(0.6),
      m_ethf(0.2),
      m_maxRange(100.0),
      m_initialEnergy(500.0),
      m_evaluationWindow(Seconds(2.0)),
      m_w1(0.25), m_w2(0.25), m_w3(0.25), m_w4(0.25),
      m_propagationSpeed(1500.0),
      m_rreqRetries(2),
      m_ttlStart(1),
      m_ttlIncrement(2),
      m_ttlThreshold(7),
      m_netDiameter(35),
      m_timeoutBuffer(2),
      m_nodeTraversalTime(MilliSeconds(40)),
      m_helloInterval(Seconds(1)),
      m_allowedHelloLoss(2),
      m_activeRouteTimeout(Seconds(3)),
      m_deletePeriod(Seconds(15)),
      m_nextHopWait(MilliSeconds(50)),
      m_blackListTimeout(Seconds(5.6)),
      m_maxQueueLen(64),
      m_maxQueueTime(Seconds(30)),
      m_rreqRateLimit(10),
      m_rerrRateLimit(10),
      m_destinationOnly(false),
      m_gratuitousReply(true),
      m_enableHello(true),
      m_enableBroadcast(true),
      m_packetsSent(0),
      m_routingTable(Seconds(3)),
      m_rreqIdCache(Seconds(5)),
      m_nb(Seconds(2)),
      m_dpd(Seconds(10)),
      m_requestId(0),
      m_seqNo(0),
      m_rreqCount(0),
      m_rerrCount(0),
      m_htimer(Timer::CANCEL_ON_DESTROY),
      m_rreqRateLimitTimer(Timer::CANCEL_ON_DESTROY),
      m_rerrRateLimitTimer(Timer::CANCEL_ON_DESTROY),
      m_lastBcastTime(Seconds(0))
{
    m_netTraversalTime = Time(2 * m_netDiameter * m_nodeTraversalTime);
    m_pathDiscoveryTime = Time(2 * m_netTraversalTime);
    m_myRouteTimeout = Time(2 * m_activeRouteTimeout);
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

RoutingProtocol::~RoutingProtocol()
{
}

void
RoutingProtocol::DoDispose()
{
    m_ipv4 = nullptr;
    m_node = nullptr;
    m_lo = nullptr;
    
    for (auto& entry : m_socketAddresses)
    {
        entry.first->Close();
    }
    m_socketAddresses.clear();
    m_socketSubnetBroadcastAddresses.clear();
    
    Ipv4RoutingProtocol::DoDispose();
}

void
RoutingProtocol::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    
    if (m_enableHello && !m_htimer.IsRunning())
    {
        m_htimer.SetFunction(&RoutingProtocol::HelloTimerExpire, this);
        uint32_t startTime = m_uniformRandomVariable->GetInteger(0, 100);
        m_htimer.Schedule(MilliSeconds(startTime));
    }
    
    Ipv4RoutingProtocol::DoInitialize();
}

void
RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    *stream->GetStream() << "Node: " << m_node->GetId()
                         << " Time: " << Simulator::Now().As(unit)
                         << " GAODV-V1 Routing table\n";
    m_routingTable.Print(stream, unit);
}

int64_t
RoutingProtocol::AssignStreams(int64_t stream)
{
    m_uniformRandomVariable->SetStream(stream);
    return 1;
}

void
RoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    NS_ASSERT(!m_ipv4 && ipv4);
    
    m_ipv4 = ipv4;
    m_node = m_ipv4->GetObject<Node>();
    
    m_queue.SetMaxQueueLen(m_maxQueueLen);
    m_queue.SetQueueTimeout(m_maxQueueTime);
    m_rreqIdCache.SetLifetime(m_pathDiscoveryTime);
    m_nb.SetCallback(MakeCallback(&RoutingProtocol::HandleLinkFailure, this));
    
    Simulator::ScheduleNow(&RoutingProtocol::DoInitialize, this);
}

//==========================================================================
// G-AODV Core Methods
//==========================================================================

double
RoutingProtocol::ComputeNef() const
{
    Ptr<energy::EnergySource> energySrc = m_node->GetObject<energy::EnergySource>();
    if (energySrc)
    {
        double residual = energySrc->GetRemainingEnergy();
        double nef = residual / m_initialEnergy;
        NS_LOG_DEBUG("Node " << m_node->GetId() << " NEF: " << nef);
        return nef;
    }
    NS_LOG_WARN("No EnergySource on node " << m_node->GetId() << ". Assuming NEF=1.0");
    return 1.0;
}

double
RoutingProtocol::ComputeRef(const BufferedRreq& entry) const
{
    double rreqEf = entry.header.GetRreqEf();
    double dist = std::max(entry.distance, 1.0);
    double hops = std::max(static_cast<double>(entry.hopCount), 1.0);
    double delay = dist / m_propagationSpeed;
    delay = std::max(delay, 0.001);
    
    double ref = m_w1 * rreqEf +
                 m_w2 * (1.0 / dist) +
                 m_w3 * (1.0 / hops) +
                 m_w4 * (1.0 / delay);
    
    NS_LOG_DEBUG("REF: " << ref << " (RREQEF=" << rreqEf << ", dist=" << dist 
                 << ", hops=" << hops << ", delay=" << delay << ")");
    return ref;
}

Vector
RoutingProtocol::GetPosition() const
{
    Ptr<MobilityModel> mobility = m_node->GetObject<MobilityModel>();
    if (mobility)
    {
        return mobility->GetPosition();
    }
    NS_LOG_WARN("No MobilityModel on node " << m_node->GetId());
    return Vector(0, 0, 0);
}

double
RoutingProtocol::CalculateDistance(const Vector& a, const Vector& b)
{
    return std::sqrt((a.x - b.x) * (a.x - b.x) +
                     (a.y - b.y) * (a.y - b.y) +
                     (a.z - b.z) * (a.z - b.z));
}

bool
RoutingProtocol::IsMyOwnAddress(Ipv4Address addr) const
{
    for (const auto& entry : m_socketAddresses)
    {
        if (entry.second.GetLocal() == addr)
            return true;
    }
    return false;
}

uint64_t
RoutingProtocol::MakeRreqKey(Ipv4Address origin, uint32_t id) const
{
    return (static_cast<uint64_t>(origin.Get()) << 32) | id;
}

double
RoutingProtocol::GetPacketDeliveryRatio() const
{
    if (m_packetsSent == 0)
        return 0.0;
    return static_cast<double>(m_receivedUids.size()) / m_packetsSent;
}

} // namespace gaodvv1
} // namespace ns3
/*
 * GAODV-V1: Routing Protocol Implementation - Part 2 (Routing Logic)
 */

#include "gaodv-v1-routing-protocol.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"

namespace ns3 {
namespace gaodvv1 {

//==========================================================================
// Ipv4RoutingProtocol Interface Implementation
//==========================================================================

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput(Ptr<Packet> p,
                              const Ipv4Header& header,
                              Ptr<NetDevice> oif,
                              Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << header.GetDestination());
    
    if (!p)
    {
        NS_LOG_DEBUG("Null packet");
        sockerr = Socket::ERROR_NOROUTETOHOST;
        return nullptr;
    }
    
    Ipv4Address dst = header.GetDestination();
    
    if (dst.IsLocalMulticast())
    {
        NS_LOG_DEBUG("Multicast not supported");
        sockerr = Socket::ERROR_NOROUTETOHOST;
        return nullptr;
    }
    
    // Check routing table for valid route
    RoutingTableEntry rt;
    if (m_routingTable.LookupValidRoute(dst, rt))
    {
        NS_LOG_DEBUG("Valid route found to " << dst);
        sockerr = Socket::ERROR_NOTERROR;
        return rt.GetRoute();
    }
    
    // No route - initiate route discovery
    NS_LOG_DEBUG("No route to " << dst << ", initiating route discovery");
    sockerr = Socket::ERROR_NOROUTETOHOST;
    
    // Start route discovery if not already in progress
    if (!m_routingTable.LookupRoute(dst, rt) || rt.GetFlag() != IN_SEARCH)
    {
        SendRequest(dst);
    }
    
    return LoopbackRoute(header, oif);
}

bool
RoutingProtocol::RouteInput(Ptr<const Packet> p,
                             const Ipv4Header& header,
                             Ptr<const NetDevice> idev,
                             const UnicastForwardCallback& ucb,
                             const MulticastForwardCallback& mcb,
                             const LocalDeliverCallback& lcb,
                             const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p->GetUid() << header.GetDestination());
    
    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No interfaces");
        return false;
    }
    
    Ipv4Address dst = header.GetDestination();
    Ipv4Address src = header.GetSource();
    
    // Duplicate detection
    if (m_dpd.IsDuplicate(p, header))
    {
        NS_LOG_DEBUG("Duplicate packet dropped");
        return true;
    }
    
    // Check for local delivery
    int32_t iface = m_ipv4->GetInterfaceForDevice(idev);
    if (iface >= 0)
    {
        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(iface); ++j)
        {
            Ipv4InterfaceAddress address = m_ipv4->GetAddress(iface, j);
            if (address.GetLocal() == dst)
            {
                NS_LOG_DEBUG("Local delivery to " << dst);
                m_receivedUids.insert(p->GetUid());
                lcb(p, header, iface);
                return true;
            }
        }
    }
    
    // Check for broadcast
    for (const auto& entry : m_socketAddresses)
    {
        if (entry.second.GetBroadcast() == dst)
        {
            NS_LOG_DEBUG("Broadcast delivery");
            m_receivedUids.insert(p->GetUid());
            lcb(p, header, m_ipv4->GetInterfaceForAddress(entry.second.GetLocal()));
            return true;
        }
    }
    
    // Forward packet
    return Forwarding(p, header, ucb, ecb);
}

bool
RoutingProtocol::Forwarding(Ptr<const Packet> p,
                             const Ipv4Header& header,
                             UnicastForwardCallback ucb,
                             ErrorCallback ecb)
{
    NS_LOG_FUNCTION(this << header.GetDestination());
    
    Ipv4Address dst = header.GetDestination();
    
    RoutingTableEntry rt;
    if (m_routingTable.LookupValidRoute(dst, rt))
    {
        Ptr<Ipv4Route> route = rt.GetRoute();
        NS_LOG_DEBUG("Forwarding to " << dst << " via " << route->GetGateway());
        
        UpdateRouteLifeTime(dst, m_activeRouteTimeout);
        UpdateRouteLifeTime(route->GetGateway(), m_activeRouteTimeout);
        
        m_packetsSent++;
        ucb(route, p, header);
        return true;
    }
    
    NS_LOG_DEBUG("No route to " << dst << ", queuing packet");
    
    // Queue packet and initiate route discovery
    QueueEntry qe(p, header, ucb, ecb);
    if (m_queue.Enqueue(qe))
    {
        if (!m_routingTable.LookupRoute(dst, rt) || rt.GetFlag() != IN_SEARCH)
        {
            SendRequest(dst);
        }
    }
    
    return true;
}

Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute(const Ipv4Header& header, Ptr<NetDevice> oif) const
{
    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(header.GetDestination());
    
    auto j = m_socketAddresses.begin();
    if (oif)
    {
        for (; j != m_socketAddresses.end(); ++j)
        {
            Ipv4Address addr = j->second.GetLocal();
            int32_t iface = m_ipv4->GetInterfaceForAddress(addr);
            if (oif == m_ipv4->GetNetDevice(static_cast<uint32_t>(iface)))
            {
                rt->SetSource(addr);
                break;
            }
        }
    }
    else
    {
        rt->SetSource(j->second.GetLocal());
    }
    
    rt->SetGateway(Ipv4Address("127.0.0.1"));
    rt->SetOutputDevice(m_lo);
    return rt;
}

//==========================================================================
// Interface Notifications
//==========================================================================

void
RoutingProtocol::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    
    if (l3->GetNAddresses(i) <= 0)
        return;
    
    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
    {
        m_lo = l3->GetNetDevice(i);
        return;
    }
    
    // Create socket
    Ptr<Socket> socket = Socket::CreateSocket(m_node, UdpSocketFactory::GetTypeId());
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvGaodvv1, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(iface.GetLocal(), GAODVV1_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    m_socketAddresses.insert(std::make_pair(socket, iface));
    
    // Subnet broadcast socket
    socket = Socket::CreateSocket(m_node, UdpSocketFactory::GetTypeId());
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvGaodvv1, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(iface.GetBroadcast(), GAODVV1_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    m_socketSubnetBroadcastAddresses.insert(std::make_pair(socket, iface));
    
    // Add broadcast route
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
    RoutingTableEntry rt(dev, iface.GetBroadcast(), true, 0, iface, 1,
                         iface.GetBroadcast(), Simulator::GetMaximumSimulationTime());
    m_routingTable.AddRoute(rt);
}

void
RoutingProtocol::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    if (socket)
    {
        socket->Close();
        m_socketAddresses.erase(socket);
    }
    
    socket = FindSubnetBroadcastSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    if (socket)
    {
        socket->Close();
        m_socketSubnetBroadcastAddresses.erase(socket);
    }
    
    m_routingTable.DeleteAllRoutesFromInterface(m_ipv4->GetAddress(i, 0));
}

void
RoutingProtocol::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    // Simplified - full implementation would create new sockets
}

void
RoutingProtocol::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    // Simplified - full implementation would remove sockets
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const
{
    for (const auto& entry : m_socketAddresses)
    {
        if (entry.second == addr)
            return entry.first;
    }
    return nullptr;
}

Ptr<Socket>
RoutingProtocol::FindSubnetBroadcastSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const
{
    for (const auto& entry : m_socketSubnetBroadcastAddresses)
    {
        if (entry.second == addr)
            return entry.first;
    }
    return nullptr;
}

bool
RoutingProtocol::UpdateRouteLifeTime(Ipv4Address addr, Time lifetime)
{
    RoutingTableEntry rt;
    if (m_routingTable.LookupRoute(addr, rt))
    {
        if (rt.GetFlag() == VALID)
        {
            rt.SetRreqCnt(0);
            rt.SetLifeTime(std::max(lifetime, rt.GetLifeTime()));
            m_routingTable.Update(rt);
            return true;
        }
    }
    return false;
}

void
RoutingProtocol::UpdateRouteToNeighbor(Ipv4Address sender, Ipv4Address receiver)
{
    RoutingTableEntry toNeighbor;
    if (!m_routingTable.LookupRoute(sender, toNeighbor))
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(dev, sender, false, 0,
                                   m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                                   1, sender, m_activeRouteTimeout);
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        toNeighbor.SetLifeTime(std::max(m_activeRouteTimeout, toNeighbor.GetLifeTime()));
        m_routingTable.Update(toNeighbor);
    }
}

void
RoutingProtocol::SendPacketFromQueue(Ipv4Address dst, Ptr<Ipv4Route> route)
{
    NS_LOG_FUNCTION(this << dst);
    
    QueueEntry qe;
    while (m_queue.Dequeue(dst, qe))
    {
        m_packetsSent++;
        qe.GetUnicastForwardCallback()(route, qe.GetPacket(), qe.GetIpv4Header());
    }
}

void
RoutingProtocol::SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
    socket->SendTo(packet, 0, InetSocketAddress(destination, GAODVV1_PORT));
}

void
RoutingProtocol::HandleLinkFailure(Ipv4Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    SendRerrWhenBreaksLinkToNextHop(addr);
}

} // namespace gaodvv1
} // namespace ns3
/*
 * GAODV-V1: Routing Protocol Implementation - Part 3 (G-AODV Flowchart Logic)
 * 
 * This file implements the core G-AODV flowchart decision logic.
 */

#include "gaodv-v1-routing-protocol.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"

namespace ns3 {
namespace gaodvv1 {

//==========================================================================
// G-AODV Flowchart: SendRequest
// "Start discovery Flooding RREQ with (x,y,z) & RREQEF"
//==========================================================================

void
RoutingProtocol::SendRequest(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    
    // Rate limiting
    if (m_rreqCount >= m_rreqRateLimit)
    {
        NS_LOG_DEBUG("RREQ rate limited");
        Simulator::Schedule(Seconds(1), &RoutingProtocol::SendRequest, this, dst);
        return;
    }
    
    // Create or update route entry for discovery
    RoutingTableEntry rt;
    if (!m_routingTable.LookupRoute(dst, rt))
    {
        Ptr<NetDevice> dev = nullptr;
        Ipv4InterfaceAddress iface;
        if (!m_socketAddresses.empty())
        {
            dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(
                m_socketAddresses.begin()->second.GetLocal()));
            iface = m_socketAddresses.begin()->second;
        }
        RoutingTableEntry newEntry(dev, dst, false, 0, iface, m_netDiameter,
                                   Ipv4Address(), Seconds(0));
        newEntry.SetFlag(IN_SEARCH);
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        rt.SetFlag(IN_SEARCH);
        m_routingTable.Update(rt);
    }
    
    // Build RREQ
    RreqHeader rreq;
    rreq.SetId(++m_requestId);
    rreq.SetDst(dst);
    rreq.SetDstSeqno(m_routingTable.LookupRoute(dst, rt) ? rt.GetSeqNo() : 0);
    rreq.SetOrigin(m_socketAddresses.begin()->second.GetLocal());
    rreq.SetOriginSeqno(++m_seqNo);
    rreq.SetHopCount(0);
    
    // G-AODV Extensions: Position and RREQEF
    rreq.SetPosition(GetPosition());
    rreq.SetRreqEf(ComputeNef());  // RREQEF = source's NEF
    
    rreq.SetGratuitousRrep(m_gratuitousReply);
    rreq.SetDestinationOnly(m_destinationOnly);
    
    // Record broadcast
    m_rreqIdCache.IsDuplicate(rreq.GetOrigin(), rreq.GetId());
    m_rreqCount++;
    
    // Broadcast RREQ
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rreq);
    TypeHeader tHeader(GAODVV1_RREQ);
    packet->AddHeader(tHeader);
    
    NS_LOG_INFO("GAODV-V1 Node " << m_node->GetId() << " sending RREQ to " << dst
                << " pos=(" << rreq.GetPosition().x << "," << rreq.GetPosition().y
                << "," << rreq.GetPosition().z << ") RREQEF=" << rreq.GetRreqEf());
    
    for (const auto& entry : m_socketAddresses)
    {
        Ptr<Socket> socket = entry.first;
        Ipv4InterfaceAddress iface = entry.second;
        Ptr<Packet> p = packet->Copy();
        socket->SendTo(p, 0, InetSocketAddress(iface.GetBroadcast(), GAODVV1_PORT));
    }
    
    // Schedule retry
    ScheduleRreqRetry(dst);
}

//==========================================================================
// G-AODV Flowchart: RecvRequest (Full Flowchart Implementation)
//==========================================================================

void
RoutingProtocol::RecvRequest(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
    NS_LOG_FUNCTION(this << receiver << sender);
    
    RreqHeader rreq;
    p->RemoveHeader(rreq);
    
    // STEP 3: Duplicate check - "1st RREQ message?"
    if (m_rreqIdCache.IsDuplicate(rreq.GetOrigin(), rreq.GetId()))
    {
        NS_LOG_DEBUG("Duplicate RREQ from " << rreq.GetOrigin() << " dropped");
        return;
    }
    
    // Increment hop count
    uint8_t hopCount = rreq.GetHopCount() + 1;
    rreq.SetHopCount(hopCount);
    
    // Update route to sender (neighbor)
    UpdateRouteToNeighbor(sender, receiver);
    
    // Create/update reverse route to origin
    RoutingTableEntry toOrigin;
    if (!m_routingTable.LookupRoute(rreq.GetOrigin(), toOrigin))
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(dev, rreq.GetOrigin(), true, rreq.GetOriginSeqno(),
                                   m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                                   hopCount, sender, m_activeRouteTimeout);
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        // Update if better
        bool update = false;
        if (!toOrigin.GetValidSeqNo())
            update = true;
        else if (int32_t(rreq.GetOriginSeqno()) - int32_t(toOrigin.GetSeqNo()) > 0)
            update = true;
        else if (rreq.GetOriginSeqno() == toOrigin.GetSeqNo() && hopCount < toOrigin.GetHop())
            update = true;
        
        if (update)
        {
            toOrigin.SetValidSeqNo(true);
            toOrigin.SetSeqNo(rreq.GetOriginSeqno());
            toOrigin.SetHop(hopCount);
            toOrigin.SetNextHop(sender);
            toOrigin.SetLifeTime(m_activeRouteTimeout);
            toOrigin.SetFlag(VALID);
            m_routingTable.Update(toOrigin);
        }
    }
    
    // STEP 2: Am I the destination? "Node of Destination?"
    if (IsMyOwnAddress(rreq.GetDst()))
    {
        NS_LOG_INFO("I am destination. Buffering RREQ from " << rreq.GetOrigin());
        
        // Buffer this RREQ
        BufferedRreq entry;
        entry.header = rreq;
        entry.sender = sender;
        entry.recvTime = Simulator::Now();
        entry.hopCount = hopCount;
        entry.distance = CalculateDistance(GetPosition(), rreq.GetPosition());
        
        uint64_t key = MakeRreqKey(rreq.GetOrigin(), rreq.GetId());
        m_bufferedRreqs[key] = entry;
        
        // Start Timer T if this is first RREQ for this request
        if (!m_evaluationTimer.IsPending())
        {
            m_evaluationTimer = Simulator::Schedule(m_evaluationWindow,
                                                    &RoutingProtocol::EvaluateAndSendRrep,
                                                    this);
            NS_LOG_DEBUG("Started evaluation timer T=" << m_evaluationWindow.GetSeconds() << "s");
        }
        return;
    }
    
    // STEP 4: "A valid route exist?" (Intermediate node with route to destination)
    RoutingTableEntry toDst;
    if (m_routingTable.LookupValidRoute(rreq.GetDst(), toDst) && !rreq.GetDestinationOnly())
    {
        if (toDst.GetValidSeqNo() && 
            int32_t(toDst.GetSeqNo()) - int32_t(rreq.GetDstSeqno()) >= 0)
        {
            NS_LOG_DEBUG("Intermediate node has valid route to " << rreq.GetDst());
            m_routingTable.LookupRoute(rreq.GetOrigin(), toOrigin);
            SendReplyByIntermediateNode(toDst, toOrigin, rreq.GetGratuitousRrep());
            return;
        }
    }
    
    // STEP 5: Calculate NEF and apply threshold "NEF >= ETHF and 0.4 <= α <= 0.8?"
    double myNef = ComputeNef();
    double threshold = m_alpha * m_ethf;
    
    if (myNef < threshold)
    {
        NS_LOG_DEBUG("Node " << m_node->GetId() << " dropping RREQ: NEF " 
                     << myNef << " < threshold " << threshold);
        return;
    }
    
    // STEP 6: "NEF < RREQEF?"
    double rreqEf = rreq.GetRreqEf();
    if (myNef < rreqEf)
    {
        // My NEF is lower - update both position AND RREQEF
        rreq.SetRreqEf(myNef);
        NS_LOG_DEBUG("Updated RREQEF: " << rreqEf << " -> " << myNef);
    }
    
    // Always update position (x,y,z) with current node's position
    rreq.SetPosition(GetPosition());
    
    // Rebroadcast RREQ
    NS_LOG_DEBUG("Rebroadcasting RREQ to " << rreq.GetDst());
    
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rreq);
    TypeHeader tHeader(GAODVV1_RREQ);
    packet->AddHeader(tHeader);
    
    for (const auto& sockEntry : m_socketAddresses)
    {
        Ptr<Socket> socket = sockEntry.first;
        Ipv4InterfaceAddress iface = sockEntry.second;
        Ptr<Packet> pkt = packet->Copy();
        socket->SendTo(pkt, 0, InetSocketAddress(iface.GetBroadcast(), GAODVV1_PORT));
    }
}

//==========================================================================
// G-AODV Flowchart: EvaluateAndSendRrep
// "Timer expired" -> "RREP to be sent to source with high REF"
//==========================================================================

void
RoutingProtocol::EvaluateAndSendRrep()
{
    NS_LOG_FUNCTION(this);
    
    if (m_bufferedRreqs.empty())
    {
        NS_LOG_DEBUG("No buffered RREQs to evaluate");
        return;
    }
    
    // Find RREQ with highest REF
    double bestRef = -1.0;
    BufferedRreq bestEntry;
    
    for (const auto& kv : m_bufferedRreqs)
    {
        double ref = ComputeRef(kv.second);
        NS_LOG_DEBUG("RREQ from " << kv.second.header.GetOrigin() 
                     << " via " << kv.second.sender << " REF=" << ref);
        
        if (ref > bestRef)
        {
            bestRef = ref;
            bestEntry = kv.second;
        }
    }
    
    NS_LOG_INFO("Selected best path: origin=" << bestEntry.header.GetOrigin()
                << " via " << bestEntry.sender << " REF=" << bestRef);
    
    // Get route to origin and send RREP
    RoutingTableEntry toOrigin;
    if (m_routingTable.LookupRoute(bestEntry.header.GetOrigin(), toOrigin))
    {
        SendReply(bestEntry.header, toOrigin);
    }
    else
    {
        NS_LOG_WARN("No route to origin " << bestEntry.header.GetOrigin());
    }
    
    m_bufferedRreqs.clear();
}

} // namespace gaodvv1
} // namespace ns3
/*
 * GAODV-V1: Routing Protocol Implementation - Part 4 (RREP, RERR, Timers)
 */

#include "gaodv-v1-routing-protocol.h"

#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"

namespace ns3 {
namespace gaodvv1 {

//==========================================================================
// RREP Handling
//==========================================================================

void
RoutingProtocol::SendReply(const RreqHeader& rreq, const RoutingTableEntry& toOrigin)
{
    NS_LOG_FUNCTION(this << rreq.GetOrigin());
    
    RrepHeader rrep;
    rrep.SetHopCount(0);
    rrep.SetDst(rreq.GetDst());
    rrep.SetDstSeqno(m_seqNo);
    rrep.SetOrigin(rreq.GetOrigin());
    rrep.SetLifeTime(m_myRouteTimeout);
    
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rrep);
    TypeHeader tHeader(GAODVV1_RREP);
    packet->AddHeader(tHeader);
    
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
    if (socket)
    {
        NS_LOG_INFO("Sending RREP to " << toOrigin.GetNextHop());
        socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), GAODVV1_PORT));
    }
}

void
RoutingProtocol::SendReplyByIntermediateNode(RoutingTableEntry& toDst,
                                              RoutingTableEntry& toOrigin,
                                              bool gratuitousRrep)
{
    NS_LOG_FUNCTION(this);
    
    RrepHeader rrep;
    rrep.SetHopCount(toDst.GetHop());
    rrep.SetDst(toDst.GetDestination());
    rrep.SetDstSeqno(toDst.GetSeqNo());
    rrep.SetOrigin(toOrigin.GetDestination());
    rrep.SetLifeTime(std::min(toDst.GetLifeTime(), toOrigin.GetLifeTime()));
    
    // Update precursor lists
    toDst.InsertPrecursor(toOrigin.GetNextHop());
    toOrigin.InsertPrecursor(toDst.GetNextHop());
    m_routingTable.Update(toDst);
    m_routingTable.Update(toOrigin);
    
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rrep);
    TypeHeader tHeader(GAODVV1_RREP);
    packet->AddHeader(tHeader);
    
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
    if (socket)
    {
        NS_LOG_INFO("Intermediate RREP to " << toOrigin.GetNextHop());
        socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), GAODVV1_PORT));
    }
    
    // Gratuitous RREP
    if (gratuitousRrep)
    {
        RrepHeader gratRrep;
        gratRrep.SetHopCount(toOrigin.GetHop());
        gratRrep.SetDst(toOrigin.GetDestination());
        gratRrep.SetDstSeqno(toOrigin.GetSeqNo());
        gratRrep.SetOrigin(toDst.GetDestination());
        gratRrep.SetLifeTime(toOrigin.GetLifeTime());
        
        Ptr<Packet> gratPacket = Create<Packet>();
        gratPacket->AddHeader(gratRrep);
        gratPacket->AddHeader(tHeader);
        
        socket = FindSocketWithInterfaceAddress(toDst.GetInterface());
        if (socket)
        {
            socket->SendTo(gratPacket, 0, InetSocketAddress(toDst.GetNextHop(), GAODVV1_PORT));
        }
    }
}

void
RoutingProtocol::RecvReply(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
    NS_LOG_FUNCTION(this << receiver << sender);
    
    RrepHeader rrep;
    p->RemoveHeader(rrep);
    
    uint8_t hopCount = rrep.GetHopCount() + 1;
    rrep.SetHopCount(hopCount);
    
    // Update neighbor
    UpdateRouteToNeighbor(sender, receiver);
    
    // Update route to destination
    RoutingTableEntry toDst;
    Ipv4Address dst = rrep.GetDst();
    
    bool newRoute = false;
    if (!m_routingTable.LookupRoute(dst, toDst))
    {
        newRoute = true;
    }
    else if (!toDst.GetValidSeqNo() ||
             int32_t(rrep.GetDstSeqno()) - int32_t(toDst.GetSeqNo()) > 0 ||
             (rrep.GetDstSeqno() == toDst.GetSeqNo() && hopCount < toDst.GetHop()))
    {
        newRoute = true;
    }
    
    if (newRoute)
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry entry(dev, dst, true, rrep.GetDstSeqno(),
                                m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                                hopCount, sender, rrep.GetLifeTime());
        
        if (!m_routingTable.LookupRoute(dst, toDst))
        {
            m_routingTable.AddRoute(entry);
        }
        else
        {
            m_routingTable.Update(entry);
        }
        
        NS_LOG_INFO("Route to " << dst << " updated via " << sender << " hops=" << (int)hopCount);
    }
    
    // Am I the origin?
    if (IsMyOwnAddress(rrep.GetOrigin()))
    {
        NS_LOG_INFO("RREP received at origin for " << dst);
        
        // Cancel retry timer
        if (m_addressReqTimer.find(dst) != m_addressReqTimer.end())
        {
            m_addressReqTimer[dst].Cancel();
            m_addressReqTimer.erase(dst);
        }
        
        // Send queued packets
        m_routingTable.LookupRoute(dst, toDst);
        SendPacketFromQueue(dst, toDst.GetRoute());
        return;
    }
    
    // Forward RREP toward origin
    RoutingTableEntry toOrigin;
    if (m_routingTable.LookupValidRoute(rrep.GetOrigin(), toOrigin))
    {
        toOrigin.InsertPrecursor(sender);
        m_routingTable.Update(toOrigin);
        
        m_routingTable.LookupRoute(dst, toDst);
        toDst.InsertPrecursor(toOrigin.GetNextHop());
        m_routingTable.Update(toDst);
        
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(rrep);
        TypeHeader tHeader(GAODVV1_RREP);
        packet->AddHeader(tHeader);
        
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
        if (socket)
        {
            socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), GAODVV1_PORT));
        }
    }
}

void
RoutingProtocol::SendReplyAck(Ipv4Address neighbor)
{
    Ptr<Packet> packet = Create<Packet>();
    RrepAckHeader ack;
    packet->AddHeader(ack);
    TypeHeader tHeader(GAODVV1_RREP_ACK);
    packet->AddHeader(tHeader);
    
    for (const auto& entry : m_socketAddresses)
    {
        entry.first->SendTo(packet, 0, InetSocketAddress(neighbor, GAODVV1_PORT));
        return;
    }
}

void
RoutingProtocol::RecvReplyAck(Ipv4Address neighbor)
{
    NS_LOG_FUNCTION(this << neighbor);
    // Mark link as bidirectional
}

//==========================================================================
// RERR Handling
//==========================================================================

void
RoutingProtocol::SendRerrWhenBreaksLinkToNextHop(Ipv4Address nextHop)
{
    NS_LOG_FUNCTION(this << nextHop);
    
    if (m_rerrCount >= m_rerrRateLimit)
    {
        return;
    }
    
    std::map<Ipv4Address, uint32_t> unreachable;
    m_routingTable.GetListOfDestinationsWithNextHop(nextHop, unreachable);
    
    if (unreachable.empty())
        return;
    
    RerrHeader rerr;
    for (const auto& un : unreachable)
    {
        rerr.AddUnDestination(un.first, un.second);
    }
    
    m_routingTable.InvalidateRoutesWithDst(unreachable);
    
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(rerr);
    TypeHeader tHeader(GAODVV1_RERR);
    packet->AddHeader(tHeader);
    
    for (const auto& entry : m_socketAddresses)
    {
        entry.first->SendTo(packet, 0, InetSocketAddress(entry.second.GetBroadcast(), GAODVV1_PORT));
    }
    
    m_rerrCount++;
}

void
RoutingProtocol::RecvError(Ptr<Packet> p, Ipv4Address sender)
{
    NS_LOG_FUNCTION(this << sender);
    
    RerrHeader rerr;
    p->RemoveHeader(rerr);
    
    std::pair<Ipv4Address, uint32_t> un;
    std::map<Ipv4Address, uint32_t> unreachable;
    
    while (rerr.RemoveUnDestination(un))
    {
        RoutingTableEntry rt;
        if (m_routingTable.LookupRoute(un.first, rt) && rt.GetNextHop() == sender)
        {
            unreachable[un.first] = un.second;
        }
    }
    
    if (!unreachable.empty())
    {
        m_routingTable.InvalidateRoutesWithDst(unreachable);
    }
}

//==========================================================================
// Timers and Helpers
//==========================================================================

void
RoutingProtocol::RecvGaodvv1(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    
    while ((packet = socket->RecvFrom(senderAddress)))
    {
        InetSocketAddress senderInet = InetSocketAddress::ConvertFrom(senderAddress);
        Ipv4Address sender = senderInet.GetIpv4();
        Ipv4Address receiver = m_socketAddresses[socket].GetLocal();
        
        TypeHeader tHeader;
        packet->RemoveHeader(tHeader);
        
        if (!tHeader.IsValid())
        {
            NS_LOG_DEBUG("Invalid type header");
            continue;
        }
        
        switch (tHeader.Get())
        {
        case GAODVV1_RREQ:
            RecvRequest(packet, receiver, sender);
            break;
        case GAODVV1_RREP:
            RecvReply(packet, receiver, sender);
            break;
        case GAODVV1_RERR:
            RecvError(packet, sender);
            break;
        case GAODVV1_RREP_ACK:
            RecvReplyAck(sender);
            break;
        }
    }
}

void
RoutingProtocol::ScheduleRreqRetry(Ipv4Address dst)
{
    if (m_addressReqTimer.find(dst) == m_addressReqTimer.end())
    {
        Timer timer(Timer::CANCEL_ON_DESTROY);
        m_addressReqTimer[dst] = timer;
    }
    m_addressReqTimer[dst].SetFunction(&RoutingProtocol::RouteRequestTimerExpire, this);
    m_addressReqTimer[dst].SetArguments(dst);
    m_addressReqTimer[dst].Schedule(m_netTraversalTime);
}

void
RoutingProtocol::RouteRequestTimerExpire(Ipv4Address dst)
{
    RoutingTableEntry rt;
    if (m_routingTable.LookupRoute(dst, rt) && rt.GetFlag() == IN_SEARCH)
    {
        if (rt.GetRreqCnt() < m_rreqRetries)
        {
            rt.IncrementRreqCnt();
            m_routingTable.Update(rt);
            SendRequest(dst);
        }
        else
        {
            m_queue.DropPacketWithDst(dst);
            m_routingTable.DeleteRoute(dst);
        }
    }
}

void
RoutingProtocol::HelloTimerExpire()
{
    if (m_enableHello)
    {
        SendHello();
        m_htimer.Schedule(m_helloInterval);
    }
}

void
RoutingProtocol::SendHello()
{
    for (const auto& entry : m_socketAddresses)
    {
        RrepHeader helloHeader;
        helloHeader.SetHello(entry.second.GetLocal(), m_seqNo, 
                             Time(m_allowedHelloLoss * m_helloInterval));
        
        Ptr<Packet> packet = Create<Packet>();
        packet->AddHeader(helloHeader);
        TypeHeader tHeader(GAODVV1_RREP);
        packet->AddHeader(tHeader);
        
        entry.first->SendTo(packet, 0, 
                           InetSocketAddress(entry.second.GetBroadcast(), GAODVV1_PORT));
    }
}

void
RoutingProtocol::ProcessHello(const RrepHeader& rrepHeader, Ipv4Address receiver)
{
    NS_LOG_FUNCTION(this);
    m_nb.Update(rrepHeader.GetDst(), Time(m_allowedHelloLoss * m_helloInterval));
}

void
RoutingProtocol::AckTimerExpire(Ipv4Address neighbor, Time blacklistTimeout)
{
    m_routingTable.MarkLinkAsUnidirectional(neighbor, blacklistTimeout);
}

void
RoutingProtocol::RreqRateLimitTimerExpire()
{
    m_rreqCount = 0;
    m_rreqRateLimitTimer.Schedule(Seconds(1));
}

void
RoutingProtocol::RerrRateLimitTimerExpire()
{
    m_rerrCount = 0;
    m_rerrRateLimitTimer.Schedule(Seconds(1));
}

} // namespace gaodvv1
} // namespace ns3
