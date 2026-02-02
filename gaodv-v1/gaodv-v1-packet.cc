/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * 
 * This is an original implementation following the G-AODV flowchart.
 * NOT based on gaodv-v0 - this is a fresh design.
 *
 * Copyright (c) 2026
 */

#include "gaodv-v1-packet.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"

#include <cmath>
#include <cstring>

namespace ns3 {
namespace gaodvv1 {

NS_LOG_COMPONENT_DEFINE("Gaodvv1Packet");

// Helper functions for double <-> uint64_t conversion (bit-preserving)
static inline uint64_t DoubleToUint64(double val)
{
    uint64_t result;
    std::memcpy(&result, &val, sizeof(double));
    return result;
}

static inline double Uint64ToDouble(uint64_t val)
{
    double result;
    std::memcpy(&result, &val, sizeof(double));
    return result;
}

//-----------------------------------------------------------------------------
// TypeHeader
//-----------------------------------------------------------------------------

TypeHeader::TypeHeader(MessageType t)
    : m_type(t),
      m_valid(true)
{
}

NS_OBJECT_ENSURE_REGISTERED(TypeHeader);

TypeId
TypeHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::gaodvv1::TypeHeader")
                            .SetParent<Header>()
                            .SetGroupName("Gaodvv1")
                            .AddConstructor<TypeHeader>();
    return tid;
}

TypeId
TypeHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
TypeHeader::GetSerializedSize() const
{
    return 1;
}

void
TypeHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8((uint8_t)m_type);
}

uint32_t
TypeHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint8_t type = i.ReadU8();
    m_valid = true;
    switch (type)
    {
    case GAODVV1_RREQ:
    case GAODVV1_RREP:
    case GAODVV1_RERR:
    case GAODVV1_RREP_ACK:
        m_type = (MessageType)type;
        break;
    default:
        m_valid = false;
    }
    return i.GetDistanceFrom(start);
}

void
TypeHeader::Print(std::ostream& os) const
{
    os << "GAODV-V1 ";
    switch (m_type)
    {
    case GAODVV1_RREQ:
        os << "RREQ";
        break;
    case GAODVV1_RREP:
        os << "RREP";
        break;
    case GAODVV1_RERR:
        os << "RERR";
        break;
    case GAODVV1_RREP_ACK:
        os << "RREP_ACK";
        break;
    default:
        os << "UNKNOWN";
    }
}

bool
TypeHeader::operator==(const TypeHeader& o) const
{
    return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream&
operator<<(std::ostream& os, const TypeHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RreqHeader
//-----------------------------------------------------------------------------

RreqHeader::RreqHeader()
    : m_flags(0),
      m_reserved(0),
      m_hopCount(0),
      m_requestId(0),
      m_dst(),
      m_dstSeqNo(0),
      m_origin(),
      m_originSeqNo(0),
      m_position(Vector(0.0, 0.0, 0.0)),
      m_rreqEfBits(DoubleToUint64(1.0))  // Default: full efficiency
{
}

NS_OBJECT_ENSURE_REGISTERED(RreqHeader);

TypeId
RreqHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::gaodvv1::RreqHeader")
                            .SetParent<Header>()
                            .SetGroupName("Gaodvv1")
                            .AddConstructor<RreqHeader>();
    return tid;
}

TypeId
RreqHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RreqHeader::GetSerializedSize() const
{
    // Standard fields: 4 + 4 + 4 + 4 + 4 + 4 = 24 bytes
    // GAODV-V1 extensions: 8 + 8 + 8 = 24 bytes (2 doubles for position X,Y + 1 for RREQEF)
    return 24 + 24;
}

void
RreqHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    
    // Standard AODV-like fields
    i.WriteU8(m_flags);
    i.WriteU8(m_reserved);
    i.WriteU8(m_hopCount);
    i.WriteU8(0);  // Unused padding byte
    i.WriteHtonU32(m_requestId);
    WriteTo(i, m_dst);
    i.WriteHtonU32(m_dstSeqNo);
    WriteTo(i, m_origin);
    i.WriteHtonU32(m_originSeqNo);
    
    // GAODV-V1 extensions: position (x, y) - 2D only for ground MANETs
    i.WriteHtonU64(DoubleToUint64(m_position.x));
    i.WriteHtonU64(DoubleToUint64(m_position.y));
    // Z axis removed - using 2D positioning only
    
    // GAODV-V1 extensions: RREQEF
    i.WriteHtonU64(m_rreqEfBits);
}

uint32_t
RreqHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    
    // Standard AODV-like fields
    m_flags = i.ReadU8();
    m_reserved = i.ReadU8();
    m_hopCount = i.ReadU8();
    i.ReadU8();  // Unused padding byte
    m_requestId = i.ReadNtohU32();
    ReadFrom(i, m_dst);
    m_dstSeqNo = i.ReadNtohU32();
    ReadFrom(i, m_origin);
    m_originSeqNo = i.ReadNtohU32();
    
    // GAODV-V1 extensions: position (x, y) - 2D only for ground MANETs
    m_position.x = Uint64ToDouble(i.ReadNtohU64());
    m_position.y = Uint64ToDouble(i.ReadNtohU64());
    m_position.z = 0.0;  // Z axis not transmitted - set to 0
    
    // GAODV-V1 extensions: RREQEF
    m_rreqEfBits = i.ReadNtohU64();
    
    return i.GetDistanceFrom(start);
}

void
RreqHeader::Print(std::ostream& os) const
{
    os << "GAODV-V1 RREQ: dst=" << m_dst << " origin=" << m_origin
       << " id=" << m_requestId << " hops=" << (uint32_t)m_hopCount
       << " pos=(" << m_position.x << "," << m_position.y << ")"
       << " RREQEF=" << GetRreqEf();
}

void
RreqHeader::SetRreqEf(double ef)
{
    m_rreqEfBits = DoubleToUint64(ef);
}

double
RreqHeader::GetRreqEf() const
{
    return Uint64ToDouble(m_rreqEfBits);
}

// Flag bit positions
static const uint8_t RREQ_JOIN_REQ = 0x80;       // J flag
static const uint8_t RREQ_REPAIR = 0x40;         // R flag
static const uint8_t RREQ_GRATUITOUS = 0x20;     // G flag
static const uint8_t RREQ_DST_ONLY = 0x10;       // D flag
static const uint8_t RREQ_UNKNOWN_SEQNO = 0x08;  // U flag

void
RreqHeader::SetGratuitousRrep(bool f)
{
    if (f)
        m_flags |= RREQ_GRATUITOUS;
    else
        m_flags &= ~RREQ_GRATUITOUS;
}

bool
RreqHeader::GetGratuitousRrep() const
{
    return (m_flags & RREQ_GRATUITOUS);
}

void
RreqHeader::SetDestinationOnly(bool f)
{
    if (f)
        m_flags |= RREQ_DST_ONLY;
    else
        m_flags &= ~RREQ_DST_ONLY;
}

bool
RreqHeader::GetDestinationOnly() const
{
    return (m_flags & RREQ_DST_ONLY);
}

void
RreqHeader::SetUnknownSeqno(bool f)
{
    if (f)
        m_flags |= RREQ_UNKNOWN_SEQNO;
    else
        m_flags &= ~RREQ_UNKNOWN_SEQNO;
}

bool
RreqHeader::GetUnknownSeqno() const
{
    return (m_flags & RREQ_UNKNOWN_SEQNO);
}

bool
RreqHeader::operator==(const RreqHeader& o) const
{
    return (m_flags == o.m_flags && m_reserved == o.m_reserved &&
            m_hopCount == o.m_hopCount && m_requestId == o.m_requestId &&
            m_dst == o.m_dst && m_dstSeqNo == o.m_dstSeqNo &&
            m_origin == o.m_origin && m_originSeqNo == o.m_originSeqNo &&
            m_position.x == o.m_position.x && m_position.y == o.m_position.y &&
            m_rreqEfBits == o.m_rreqEfBits);  // Z axis removed - 2D only
}

std::ostream&
operator<<(std::ostream& os, const RreqHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RrepHeader
//-----------------------------------------------------------------------------

RrepHeader::RrepHeader(uint8_t prefixSize,
                       uint8_t hopCount,
                       Ipv4Address dst,
                       uint32_t dstSeqNo,
                       Ipv4Address origin,
                       Time lifetime)
    : m_flags(0),
      m_prefixSize(prefixSize),
      m_hopCount(hopCount),
      m_dst(dst),
      m_dstSeqNo(dstSeqNo),
      m_origin(origin),
      m_lifeTime(lifetime.GetMilliSeconds())
{
}

NS_OBJECT_ENSURE_REGISTERED(RrepHeader);

TypeId
RrepHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::gaodvv1::RrepHeader")
                            .SetParent<Header>()
                            .SetGroupName("Gaodvv1")
                            .AddConstructor<RrepHeader>();
    return tid;
}

TypeId
RrepHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RrepHeader::GetSerializedSize() const
{
    return 20;  // 1 + 1 + 1 + 1 + 4 + 4 + 4 + 4 = 20 bytes
}

void
RrepHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_flags);
    i.WriteU8(m_prefixSize);
    i.WriteU8(m_hopCount);
    i.WriteU8(0);  // Reserved
    WriteTo(i, m_dst);
    i.WriteHtonU32(m_dstSeqNo);
    WriteTo(i, m_origin);
    i.WriteHtonU32(m_lifeTime);
}

uint32_t
RrepHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_flags = i.ReadU8();
    m_prefixSize = i.ReadU8();
    m_hopCount = i.ReadU8();
    i.ReadU8();  // Reserved
    ReadFrom(i, m_dst);
    m_dstSeqNo = i.ReadNtohU32();
    ReadFrom(i, m_origin);
    m_lifeTime = i.ReadNtohU32();
    return i.GetDistanceFrom(start);
}

void
RrepHeader::Print(std::ostream& os) const
{
    os << "GAODV-V1 RREP: dst=" << m_dst << " origin=" << m_origin
       << " seqno=" << m_dstSeqNo << " hops=" << (uint32_t)m_hopCount
       << " lifetime=" << m_lifeTime << "ms";
}

static const uint8_t RREP_ACK_REQUIRED = 0x40;

void
RrepHeader::SetAckRequired(bool f)
{
    if (f)
        m_flags |= RREP_ACK_REQUIRED;
    else
        m_flags &= ~RREP_ACK_REQUIRED;
}

bool
RrepHeader::GetAckRequired() const
{
    return (m_flags & RREP_ACK_REQUIRED);
}

void
RrepHeader::SetLifeTime(Time t)
{
    m_lifeTime = t.GetMilliSeconds();
}

Time
RrepHeader::GetLifeTime() const
{
    return MilliSeconds(m_lifeTime);
}

void
RrepHeader::SetHello(Ipv4Address origin, uint32_t srcSeqNo, Time lifetime)
{
    m_flags = 0;
    m_prefixSize = 0;
    m_hopCount = 0;
    m_dst = origin;
    m_dstSeqNo = srcSeqNo;
    m_origin = origin;
    m_lifeTime = lifetime.GetMilliSeconds();
}

bool
RrepHeader::operator==(const RrepHeader& o) const
{
    return (m_flags == o.m_flags && m_prefixSize == o.m_prefixSize &&
            m_hopCount == o.m_hopCount && m_dst == o.m_dst &&
            m_dstSeqNo == o.m_dstSeqNo && m_origin == o.m_origin &&
            m_lifeTime == o.m_lifeTime);
}

std::ostream&
operator<<(std::ostream& os, const RrepHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RrepAckHeader
//-----------------------------------------------------------------------------

RrepAckHeader::RrepAckHeader()
    : m_reserved(0)
{
}

NS_OBJECT_ENSURE_REGISTERED(RrepAckHeader);

TypeId
RrepAckHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::gaodvv1::RrepAckHeader")
                            .SetParent<Header>()
                            .SetGroupName("Gaodvv1")
                            .AddConstructor<RrepAckHeader>();
    return tid;
}

TypeId
RrepAckHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RrepAckHeader::GetSerializedSize() const
{
    return 1;
}

void
RrepAckHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_reserved);
}

uint32_t
RrepAckHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_reserved = i.ReadU8();
    return i.GetDistanceFrom(start);
}

void
RrepAckHeader::Print(std::ostream& os) const
{
    os << "GAODV-V1 RREP_ACK";
}

bool
RrepAckHeader::operator==(const RrepAckHeader& o) const
{
    return (m_reserved == o.m_reserved);
}

std::ostream&
operator<<(std::ostream& os, const RrepAckHeader& h)
{
    h.Print(os);
    return os;
}

//-----------------------------------------------------------------------------
// RerrHeader
//-----------------------------------------------------------------------------

RerrHeader::RerrHeader()
    : m_flag(0),
      m_reserved(0)
{
}

NS_OBJECT_ENSURE_REGISTERED(RerrHeader);

TypeId
RerrHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::gaodvv1::RerrHeader")
                            .SetParent<Header>()
                            .SetGroupName("Gaodvv1")
                            .AddConstructor<RerrHeader>();
    return tid;
}

TypeId
RerrHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RerrHeader::GetSerializedSize() const
{
    // 1 (flag) + 1 (reserved) + 1 (destCount) + 1 (reserved) + 8*destCount
    return 4 + 8 * m_unreachableDstSeqNo.size();
}

void
RerrHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_flag);
    i.WriteU8(m_reserved);
    i.WriteU8((uint8_t)m_unreachableDstSeqNo.size());
    i.WriteU8(0);  // Reserved
    
    for (auto it = m_unreachableDstSeqNo.begin(); it != m_unreachableDstSeqNo.end(); ++it)
    {
        WriteTo(i, it->first);
        i.WriteHtonU32(it->second);
    }
}

uint32_t
RerrHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_flag = i.ReadU8();
    m_reserved = i.ReadU8();
    uint8_t destCount = i.ReadU8();
    i.ReadU8();  // Reserved
    
    m_unreachableDstSeqNo.clear();
    for (uint8_t k = 0; k < destCount; ++k)
    {
        Ipv4Address address;
        ReadFrom(i, address);
        uint32_t seqNo = i.ReadNtohU32();
        m_unreachableDstSeqNo[address] = seqNo;
    }
    return i.GetDistanceFrom(start);
}

void
RerrHeader::Print(std::ostream& os) const
{
    os << "GAODV-V1 RERR: unreachable=" << m_unreachableDstSeqNo.size();
}

static const uint8_t RERR_NO_DELETE = 0x80;

void
RerrHeader::SetNoDelete(bool f)
{
    if (f)
        m_flag |= RERR_NO_DELETE;
    else
        m_flag &= ~RERR_NO_DELETE;
}

bool
RerrHeader::GetNoDelete() const
{
    return (m_flag & RERR_NO_DELETE);
}

bool
RerrHeader::AddUnDestination(Ipv4Address dst, uint32_t seqNo)
{
    if (m_unreachableDstSeqNo.find(dst) != m_unreachableDstSeqNo.end())
    {
        return true;  // Already exists
    }
    if (m_unreachableDstSeqNo.size() >= 255)
    {
        return false;  // Too many
    }
    m_unreachableDstSeqNo[dst] = seqNo;
    return true;
}

bool
RerrHeader::RemoveUnDestination(std::pair<Ipv4Address, uint32_t>& un)
{
    if (m_unreachableDstSeqNo.empty())
    {
        return false;
    }
    auto it = m_unreachableDstSeqNo.begin();
    un.first = it->first;
    un.second = it->second;
    m_unreachableDstSeqNo.erase(it);
    return true;
}

void
RerrHeader::Clear()
{
    m_unreachableDstSeqNo.clear();
    m_flag = 0;
    m_reserved = 0;
}

bool
RerrHeader::operator==(const RerrHeader& o) const
{
    return (m_flag == o.m_flag && m_reserved == o.m_reserved &&
            m_unreachableDstSeqNo == o.m_unreachableDstSeqNo);
}

std::ostream&
operator<<(std::ostream& os, const RerrHeader& h)
{
    h.Print(os);
    return os;
}

} // namespace gaodvv1
} // namespace ns3
