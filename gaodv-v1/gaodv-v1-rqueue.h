/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Request Queue - Buffer packets during route discovery
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#ifndef GAODV_V1_RQUEUE_H
#define GAODV_V1_RQUEUE_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Queued packet entry
 */
class QueueEntry
{
public:
    using UnicastForwardCallback = Ipv4RoutingProtocol::UnicastForwardCallback;
    using ErrorCallback = Ipv4RoutingProtocol::ErrorCallback;

    /**
     * \brief Constructor
     * \param p Packet
     * \param header IP header
     * \param ucb Unicast forward callback
     * \param ecb Error callback
     */
    QueueEntry(Ptr<const Packet> p = nullptr,
               const Ipv4Header& header = Ipv4Header(),
               UnicastForwardCallback ucb = UnicastForwardCallback(),
               ErrorCallback ecb = ErrorCallback());

    bool operator==(const QueueEntry& o) const;

    // Getters and setters
    UnicastForwardCallback GetUnicastForwardCallback() const { return m_ucb; }
    void SetUnicastForwardCallback(UnicastForwardCallback ucb) { m_ucb = ucb; }
    ErrorCallback GetErrorCallback() const { return m_ecb; }
    void SetErrorCallback(ErrorCallback ecb) { m_ecb = ecb; }
    Ptr<const Packet> GetPacket() const { return m_packet; }
    void SetPacket(Ptr<const Packet> p) { m_packet = p; }
    Ipv4Header GetIpv4Header() const { return m_header; }
    void SetIpv4Header(Ipv4Header h) { m_header = h; }
    void SetExpireTime(Time exp) { m_expire = exp + Simulator::Now(); }
    Time GetExpireTime() const { return m_expire - Simulator::Now(); }

private:
    Ptr<const Packet> m_packet;   ///< Data packet
    Ipv4Header m_header;          ///< IP header
    UnicastForwardCallback m_ucb; ///< Unicast forward callback
    ErrorCallback m_ecb;          ///< Error callback
    Time m_expire;                ///< Expire time
};

/**
 * \ingroup gaodvv1
 * \brief Packet queue for buffering during route discovery
 */
class PacketQueue
{
public:
    /**
     * \brief Default constructor
     */
    PacketQueue();

    /**
     * \brief Enqueue a packet
     * \param entry Queue entry
     * \return true if enqueued successfully
     */
    bool Enqueue(QueueEntry& entry);

    /**
     * \brief Dequeue a packet for destination
     * \param dst Destination address
     * \param entry Output entry
     * \return true if dequeued successfully
     */
    bool Dequeue(Ipv4Address dst, QueueEntry& entry);

    /**
     * \brief Remove all packets for destination
     * \param dst Destination address
     */
    void DropPacketWithDst(Ipv4Address dst);

    /**
     * \brief Find packet for destination
     * \param dst Destination address
     * \return true if packet exists
     */
    bool Find(Ipv4Address dst);

    /**
     * \brief Get queue size
     * \return Number of entries
     */
    uint32_t GetSize();

    // Queue parameters
    void SetMaxQueueLen(uint32_t len) { m_maxLen = len; }
    uint32_t GetMaxQueueLen() const { return m_maxLen; }
    void SetQueueTimeout(Time t) { m_queueTimeout = t; }
    Time GetQueueTimeout() const { return m_queueTimeout; }

private:
    /**
     * \brief Remove expired entries
     */
    void Purge();

    /**
     * \brief Drop entry (call error callback)
     */
    void Drop(QueueEntry entry, std::string reason);

    std::vector<QueueEntry> m_queue;  ///< Packet queue
    uint32_t m_maxLen;                 ///< Max queue length
    Time m_queueTimeout;               ///< Entry timeout
};

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_RQUEUE_H
