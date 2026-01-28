/*
 * GAODV-V1: Geometric and Energy-Aware AODV Routing Protocol
 * 
 * This is an original implementation following the G-AODV flowchart.
 * NOT based on gaodv-v0 - this is a fresh design.
 *
 * Copyright (c) 2026
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef GAODV_V1_PACKET_H
#define GAODV_V1_PACKET_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/vector.h"

#include <iostream>
#include <map>

namespace ns3 {
namespace gaodvv1 {

/**
 * \ingroup gaodvv1
 * \brief Message types for GAODV-V1 protocol
 */
enum MessageType
{
    GAODVV1_RREQ = 1,     //!< Route Request
    GAODVV1_RREP = 2,     //!< Route Reply
    GAODVV1_RERR = 3,     //!< Route Error
    GAODVV1_RREP_ACK = 4  //!< Route Reply Acknowledgment
};

/**
 * \ingroup gaodvv1
 * \brief Type Header for GAODV-V1 messages
 * 
 * Identifies the type of control message (RREQ, RREP, RERR, RREP_ACK)
 */
class TypeHeader : public Header
{
public:
    /**
     * \brief Constructor
     * \param t Message type (default: RREQ)
     */
    TypeHeader(MessageType t = GAODVV1_RREQ);

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * \brief Get the message type
     * \return the message type
     */
    MessageType Get() const { return m_type; }

    /**
     * \brief Check if header is valid
     * \return true if valid
     */
    bool IsValid() const { return m_valid; }

    bool operator==(const TypeHeader& o) const;

private:
    MessageType m_type;  ///< Message type
    bool m_valid;        ///< Validity flag
};

std::ostream& operator<<(std::ostream& os, const TypeHeader& h);

/**
 * \ingroup gaodvv1
 * \brief Route Request (RREQ) Header for GAODV-V1
 * 
 * Extended RREQ with geometric and energy-aware fields:
 * - Position (x, y, z): 3D coordinates of the forwarding node
 * - RREQEF: Route Request Efficiency Factor
 * 
 * Packet Format:
 * \verbatim
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     Flags     |   Reserved    |   Hop Count   |    Unused     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                            RREQ ID                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Destination IP Address                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  Destination Sequence Number                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Originator IP Address                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  Originator Sequence Number                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Position X (64-bit)                       |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Position Y (64-bit)                       |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     Position Z (64-bit)                       |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      RREQEF (64-bit)                          |
 * |                                                               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * \endverbatim
 * 
 * Total size: 24 (standard) + 32 (extensions) = 56 bytes
 */
class RreqHeader : public Header
{
public:
    /**
     * \brief Default constructor
     */
    RreqHeader();

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // === Standard AODV-like fields ===
    
    void SetHopCount(uint8_t count) { m_hopCount = count; }
    uint8_t GetHopCount() const { return m_hopCount; }

    void SetId(uint32_t id) { m_requestId = id; }
    uint32_t GetId() const { return m_requestId; }

    void SetDst(Ipv4Address dst) { m_dst = dst; }
    Ipv4Address GetDst() const { return m_dst; }

    void SetDstSeqno(uint32_t seqno) { m_dstSeqNo = seqno; }
    uint32_t GetDstSeqno() const { return m_dstSeqNo; }

    void SetOrigin(Ipv4Address origin) { m_origin = origin; }
    Ipv4Address GetOrigin() const { return m_origin; }

    void SetOriginSeqno(uint32_t seqno) { m_originSeqNo = seqno; }
    uint32_t GetOriginSeqno() const { return m_originSeqNo; }

    // === GAODV-V1 Geometric Extensions ===
    
    /**
     * \brief Set the 3D position in the RREQ
     * \param pos The (x, y, z) position vector
     */
    void SetPosition(Vector pos) { m_position = pos; }
    
    /**
     * \brief Get the 3D position from the RREQ
     * \return The position vector
     */
    Vector GetPosition() const { return m_position; }

    // === GAODV-V1 Energy Extensions ===
    
    /**
     * \brief Set the Route Request Efficiency Factor
     * \param ef The efficiency factor (0.0 to 1.0)
     */
    void SetRreqEf(double ef);
    
    /**
     * \brief Get the Route Request Efficiency Factor
     * \return The efficiency factor
     */
    double GetRreqEf() const;

    // === Flag operations ===
    
    void SetGratuitousRrep(bool f);
    bool GetGratuitousRrep() const;
    
    void SetDestinationOnly(bool f);
    bool GetDestinationOnly() const;
    
    void SetUnknownSeqno(bool f);
    bool GetUnknownSeqno() const;

    bool operator==(const RreqHeader& o) const;

private:
    uint8_t m_flags;         ///< |J|R|G|D|U| bit flags
    uint8_t m_reserved;      ///< Reserved (must be 0)
    uint8_t m_hopCount;      ///< Hop count
    uint32_t m_requestId;    ///< RREQ ID
    Ipv4Address m_dst;       ///< Destination IP Address
    uint32_t m_dstSeqNo;     ///< Destination Sequence Number
    Ipv4Address m_origin;    ///< Originator IP Address
    uint32_t m_originSeqNo;  ///< Originator Sequence Number
    Vector m_position;       ///< 3D position (x, y, z)
    uint64_t m_rreqEfBits;   ///< RREQEF stored as bits for serialization
};

std::ostream& operator<<(std::ostream& os, const RreqHeader& h);

/**
 * \ingroup gaodvv1
 * \brief Route Reply (RREP) Header for GAODV-V1
 */
class RrepHeader : public Header
{
public:
    /**
     * \brief Constructor
     * \param prefixSize Prefix size (default 0)
     * \param hopCount Hop count (default 0)
     * \param dst Destination IP
     * \param dstSeqNo Destination sequence number
     * \param origin Originator IP
     * \param lifetime Route lifetime
     */
    RrepHeader(uint8_t prefixSize = 0,
               uint8_t hopCount = 0,
               Ipv4Address dst = Ipv4Address(),
               uint32_t dstSeqNo = 0,
               Ipv4Address origin = Ipv4Address(),
               Time lifetime = MilliSeconds(0));

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetHopCount(uint8_t count) { m_hopCount = count; }
    uint8_t GetHopCount() const { return m_hopCount; }

    void SetDst(Ipv4Address dst) { m_dst = dst; }
    Ipv4Address GetDst() const { return m_dst; }

    void SetDstSeqno(uint32_t seqno) { m_dstSeqNo = seqno; }
    uint32_t GetDstSeqno() const { return m_dstSeqNo; }

    void SetOrigin(Ipv4Address origin) { m_origin = origin; }
    Ipv4Address GetOrigin() const { return m_origin; }

    void SetLifeTime(Time t);
    Time GetLifeTime() const;

    void SetAckRequired(bool f);
    bool GetAckRequired() const;

    void SetPrefixSize(uint8_t sz) { m_prefixSize = sz; }
    uint8_t GetPrefixSize() const { return m_prefixSize; }

    /**
     * \brief Configure RREP as a Hello message
     */
    void SetHello(Ipv4Address origin, uint32_t srcSeqNo, Time lifetime);

    bool operator==(const RrepHeader& o) const;

private:
    uint8_t m_flags;       ///< Flags
    uint8_t m_prefixSize;  ///< Prefix size
    uint8_t m_hopCount;    ///< Hop count
    Ipv4Address m_dst;     ///< Destination IP
    uint32_t m_dstSeqNo;   ///< Destination sequence number
    Ipv4Address m_origin;  ///< Originator IP
    uint32_t m_lifeTime;   ///< Lifetime in milliseconds
};

std::ostream& operator<<(std::ostream& os, const RrepHeader& h);

/**
 * \ingroup gaodvv1
 * \brief Route Reply Acknowledgment (RREP-ACK) Header
 */
class RrepAckHeader : public Header
{
public:
    RrepAckHeader();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    bool operator==(const RrepAckHeader& o) const;

private:
    uint8_t m_reserved;  ///< Reserved (must be 0)
};

std::ostream& operator<<(std::ostream& os, const RrepAckHeader& h);

/**
 * \ingroup gaodvv1
 * \brief Route Error (RERR) Header for GAODV-V1
 */
class RerrHeader : public Header
{
public:
    RerrHeader();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetNoDelete(bool f);
    bool GetNoDelete() const;

    /**
     * \brief Add unreachable destination
     * \param dst Unreachable destination IP
     * \param seqNo Sequence number
     * \return true on success
     */
    bool AddUnDestination(Ipv4Address dst, uint32_t seqNo);
    
    /**
     * \brief Remove and return an unreachable destination
     * \param un Output pair (address, seqno)
     * \return true if a destination was removed
     */
    bool RemoveUnDestination(std::pair<Ipv4Address, uint32_t>& un);
    
    void Clear();
    
    uint8_t GetDestCount() const { return (uint8_t)m_unreachableDstSeqNo.size(); }

    bool operator==(const RerrHeader& o) const;

private:
    uint8_t m_flag;      ///< No delete flag
    uint8_t m_reserved;  ///< Reserved
    std::map<Ipv4Address, uint32_t> m_unreachableDstSeqNo;  ///< Unreachable destinations
};

std::ostream& operator<<(std::ostream& os, const RerrHeader& h);

} // namespace gaodvv1
} // namespace ns3

#endif // GAODV_V1_PACKET_H
