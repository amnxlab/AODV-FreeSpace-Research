/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * Request Queue Implementation
 * 
 * Original implementation following the G-AODV flowchart.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-rqueue.h"

#include "ns3/log.h"

#include <algorithm>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1RequestQueue");

//-----------------------------------------------------------------------------
// QueueEntry Implementation
//-----------------------------------------------------------------------------

QueueEntry::QueueEntry(Ptr<const Packet> p,
                       const Ipv4Header& header,
                       UnicastForwardCallback ucb,
                       ErrorCallback ecb)
    : m_packet(p),
      m_header(header),
      m_ucb(ucb),
      m_ecb(ecb),
      m_expire(Seconds(0))
{
}

bool
QueueEntry::operator==(const QueueEntry& o) const
{
    return ((m_packet == o.m_packet) && 
            (m_header.GetDestination() == o.m_header.GetDestination()) &&
            (m_expire == o.m_expire));
}

//-----------------------------------------------------------------------------
// PacketQueue Implementation
//-----------------------------------------------------------------------------

PacketQueue::PacketQueue()
    : m_maxLen(64),
      m_queueTimeout(Seconds(30))
{
}

bool
PacketQueue::Enqueue(QueueEntry& entry)
{
    NS_LOG_FUNCTION(this << entry.GetIpv4Header().GetDestination());
    
    Purge();
    
    if (m_queue.size() >= m_maxLen)
    {
        NS_LOG_DEBUG("Queue full, dropping oldest entry");
        Drop(m_queue.front(), "Queue full");
        m_queue.erase(m_queue.begin());
    }
    
    entry.SetExpireTime(m_queueTimeout);
    m_queue.push_back(entry);
    return true;
}

bool
PacketQueue::Dequeue(Ipv4Address dst, QueueEntry& entry)
{
    NS_LOG_FUNCTION(this << dst);
    
    Purge();
    
    for (auto it = m_queue.begin(); it != m_queue.end(); ++it)
    {
        if (it->GetIpv4Header().GetDestination() == dst)
        {
            entry = *it;
            m_queue.erase(it);
            return true;
        }
    }
    return false;
}

void
PacketQueue::DropPacketWithDst(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    
    Purge();
    
    for (auto it = m_queue.begin(); it != m_queue.end();)
    {
        if (it->GetIpv4Header().GetDestination() == dst)
        {
            Drop(*it, "Route not found");
            it = m_queue.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool
PacketQueue::Find(Ipv4Address dst)
{
    for (const auto& entry : m_queue)
    {
        if (entry.GetIpv4Header().GetDestination() == dst)
        {
            return true;
        }
    }
    return false;
}

uint32_t
PacketQueue::GetSize()
{
    Purge();
    return m_queue.size();
}

void
PacketQueue::Purge()
{
    // Remove expired entries
    for (auto it = m_queue.begin(); it != m_queue.end();)
    {
        if (it->GetExpireTime() < Seconds(0))
        {
            Drop(*it, "Timeout");
            it = m_queue.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void
PacketQueue::Drop(QueueEntry entry, std::string reason)
{
    NS_LOG_LOGIC("Dropping packet to " << entry.GetIpv4Header().GetDestination() 
                 << ": " << reason);
    
    entry.GetErrorCallback()(entry.GetPacket(), entry.GetIpv4Header(),
                             Socket::ERROR_NOROUTETOHOST);
}

} // namespace gaodvv1
} // namespace ns3
