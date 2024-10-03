//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup net
//!  Definitions of protocols for IP networking
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    //------------------------------------------------------------------------
    // Ethernet II link layer.
    //------------------------------------------------------------------------

    constexpr size_t ETHER_DEST_ADDR_OFFSET =  0;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_SRC_ADDR_OFFSET  =  6;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_TYPE_OFFSET      = 12;   //!< Offset of destination MAC address in an Ethernet II header.
    constexpr size_t ETHER_HEADER_SIZE      = 14;   //!< Size of an Ethernet II header.
    constexpr size_t ETHER_ADDR_SIZE        =  6;   //!< Size in bytes of a MAC address in an Ethernet II header.
    constexpr size_t ETHER_CRC_SIZE         =  4;   //!< Size in bytes of the trailing CRC in an Ethernet II frame.

    //!
    //! Selected Ethernet II protocol type identifiers.
    //! @see https://en.wikipedia.org/wiki/EtherType
    //!
    enum : uint16_t {
        ETHERTYPE_IPv4    = 0x0800,  //!< Protocol identifier for IPv4.
        ETHERTYPE_ARP     = 0x0806,  //!< Protocol identifier for ARP.
        ETHERTYPE_WOL     = 0x0842,  //!< Protocol identifier for Wake-on-LAN.
        ETHERTYPE_RARP    = 0x8035,  //!< Protocol identifier for RARP.
        ETHERTYPE_802_1Q  = 0x8100,  //!< Protocol identifier for a 2-byte IEEE 802.1Q tag (VLAN) after EtherType, then real EtherType.
        ETHERTYPE_IPv6    = 0x86DD,  //!< Protocol identifier for IPv6.
        ETHERTYPE_802_1AD = 0x88A8,  //!< Protocol identifier for IEEE 802.1ad nested VLAN.
        ETHERTYPE_802_1AH = 0x88E7,  //!< Protocol identifier for IEEE 802.1ah, Provider Backbone Bridges (PBB), aka MAC-in-MAC (MIM).
        ETHERTYPE_NULL    = 0xFFFF,  //!< Invalid protocol identifier, can be used as placeholder.
    };

    //------------------------------------------------------------------------
    // IPv4 protocol.
    //------------------------------------------------------------------------

    constexpr uint8_t IPv4_VERSION          =     4;   //!< Protocol version of IPv4 is ... 4 !
    constexpr size_t  IPv4_LENGTH_OFFSET    =     2;   //!< Offset of the total packet length in an IPv4 header.
    constexpr size_t  IPv4_FRAGMENT_OFFSET  =     6;   //!< Offset of the flags and fragments in an IPv4 header.
    constexpr size_t  IPv4_PROTOCOL_OFFSET  =     9;   //!< Offset of the protocol identifier in an IPv4 header.
    constexpr size_t  IPv4_CHECKSUM_OFFSET  =    10;   //!< Offset of the checksum in an IPv4 header.
    constexpr size_t  IPv4_SRC_ADDR_OFFSET  =    12;   //!< Offset of source IP address in an IPv4 header.
    constexpr size_t  IPv4_DEST_ADDR_OFFSET =    16;   //!< Offset of destination IP address in an IPv4 header.
    constexpr size_t  IPv4_MIN_HEADER_SIZE  =    20;   //!< Minimum size of an IPv4 header.
    constexpr size_t  IP_MAX_PACKET_SIZE    = 65536;   //!< Maximum size of an IP packet.

    //!
    //! Selected IP protocol identifiers.
    //!
    enum : uint8_t {
        IPv4_PROTO_ICMP     =   1,  //!< IPv4 protocol identifier for Internet Control Message Protocol (ICMP).
        IPv4_PROTO_IGMP     =   2,  //!< IPv4 protocol identifier for Internet Group Management Protocol (IGMP).
        IPv4_PROTO_TCP      =   6,  //!< IPv4 protocol identifier for Transmission Control Protocol (TCP).
        IPv4_PROTO_UDP      =  17,  //!< IPv4 protocol identifier for User Datagram Protocol (UDP).
        IPv4_PROTO_V6_ENCAP =  41,  //!< IPv4 protocol identifier for IPv6 encapsulation.
        IPv4_PROTO_OSPF     =  89,  //!< IPv4 protocol identifier for Open Shortest Path First (OSPF).
        IPv4_PROTO_SCTP     = 132,  //!< IPv4 protocol identifier for Stream Control Transmission Protocol (SCTP).
    };

    //!
    //! Get the name of an IP protocol (UDP, TCP, etc).
    //! @param [in] protocol Protocol identifier, as set in IP header.
    //! @param [in] long_format If false (the default), return a simple acronym.
    //! When true, return a full description string.
    //! @return The protocol name with optional description.
    //!
    TSDUCKDLL UString IPProtocolName(uint8_t protocol, bool long_format = false);

    //------------------------------------------------------------------------
    // User Datagram Protocol (UDP)
    //------------------------------------------------------------------------

    constexpr size_t UDP_SRC_PORT_OFFSET  = 0;   //!< Offset of source port in a UDP header.
    constexpr size_t UDP_DEST_PORT_OFFSET = 2;   //!< Offset of destination port in a UDP header.
    constexpr size_t UDP_LENGTH_OFFSET    = 4;   //!< Offset of packet length (UDP header + UDP payload) in a UDP header.
    constexpr size_t UDP_CHECKSUM_OFFSET  = 6;   //!< Offset of checksum in a UDP header.
    constexpr size_t UDP_HEADER_SIZE      = 8;   //!< Size of a UDP header.

    //------------------------------------------------------------------------
    // Transmission Control Protocol (TCP)
    //------------------------------------------------------------------------

    constexpr size_t TCP_SRC_PORT_OFFSET      =  0;   //!< Offset of source port in a TCP header.
    constexpr size_t TCP_DEST_PORT_OFFSET     =  2;   //!< Offset of destination port in a TCP header.
    constexpr size_t TCP_SEQUENCE_OFFSET      =  4;   //!< Offset of sequence number in a TCP header.
    constexpr size_t TCP_HEADER_LENGTH_OFFSET = 12;   //!< Offset of TCP header length in a TCP header (number of 32-bit words).
    constexpr size_t TCP_FLAGS_OFFSET         = 13;   //!< Offset of flags byte in a TCP header.
    constexpr size_t TCP_WSIZE_OFFSET         = 14;   //!< Offset of window size in a TCP header.
    constexpr size_t TCP_MIN_HEADER_SIZE      = 20;   //!< Minimum size in bytes of a TCP header.

    //! Maximum size in bytes of a TCP payload.
    constexpr size_t TCP_MAX_PAYLOAD_SIZE = IP_MAX_PACKET_SIZE - IPv4_MIN_HEADER_SIZE - TCP_MIN_HEADER_SIZE;

    //!
    //! Check if two TCP sequence numbers are ordered, including wrapping back at 0xFFFFFFFF.
    //! @param [in] seq1 First TCP sequence number.
    //! @param [in] seq2 Second TCP sequence number.
    //! @return True if @a seq1 strictly logically preceeds @a seq2.
    //!
    TSDUCKDLL bool TCPOrderedSequence(uint32_t seq1, uint32_t seq2);

    //!
    //! Compute the difference between two TCP sequence numbers, including wrapping back at 0xFFFFFFFF.
    //! @param [in] seq1 First TCP sequence number.
    //! @param [in] seq2 Second TCP sequence number which must logically follow @a seq1.
    //! @return The difference between @a seq1 and @a seq2.
    //!
    TSDUCKDLL uint32_t TCPSequenceDiff(uint32_t seq1, uint32_t seq2);

    //------------------------------------------------------------------------
    // Real-time Transport Protocol (RTP)
    //------------------------------------------------------------------------

    constexpr size_t   RTP_HEADER_SIZE =    12;  //!< Size in bytes of the fixed part of the RTP header.
    constexpr uint8_t  RTP_PT_MP2T     =    33;  //!< RTP payload type for MPEG2-TS.
    constexpr uint64_t RTP_RATE_MP2T   = 90000;  //!< RTP clock rate for MPEG2-TS.

    //!
    //! Definition of a number of RTP clock units as a std::chrono::duration type.
    //!
    using rtp_units = cn::duration<std::intmax_t, std::ratio<1, RTP_RATE_MP2T>>;

    //------------------------------------------------------------------------
    // VLAN encapsulation.
    //------------------------------------------------------------------------

    constexpr uint32_t VLAN_ID_NULL = 0xFFFFFFFF;  //!< Invalid VLAN identifier, can be used as placeholder.

    //!
    //! A structure which describes a VLAN identification.
    //!
    class TSDUCKDLL VLANId
    {
    public:
        //! The VLAN type identifies the encapsulation type of the VLAN.
        //! This is an Ethertype, typically one of ETHERTYPE_802_1Q, ETHERTYPE_802_1AD, ETHERTYPE_802_1AH.
        uint16_t type;

        //! The VLAN id identifies the VLAN in an encapsulation layer.
        //! - With ETHERTYPE_802_1Q, this is a 12-bit basic VLAN id or Customer VLAN identifier (C-VID) when encapsulated.
        //! - With ETHERTYPE_802_1AD, this is a 12-bit Backbone VLAN identifier (B-VID).
        //! - With ETHERTYPE_802_1AH, this is a 24-bit MIM Service identifier (I-SID).
        uint32_t id;

        //! @cond nodoxygen
        // Comparison, for use in containers.
        bool operator<(const VLANId& other) const { return ((uint64_t(type) << 32) | id) < ((uint64_t(other.type) << 32) | other.id); }
        bool operator==(const VLANId& other) const { return type == other.type && id == other.id; }
        TS_UNEQUAL_OPERATOR(VLANId)
        //! @endcond
    };

    //!
    //! A stack of VLAN identifiers, from outer to inner VLAN.
    //!
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(non-virtual-dtor) // The derived class allocates nothing and we only need the base destructor
    class TSDUCKDLL VLANIdStack : public std::vector<VLANId>, public StringifyInterface
    {
        TS_DEFAULT_COPY_MOVE(VLANIdStack);
    public:
        //! Explicit reference to superclass.
        using SuperClass = std::vector<VLANId>;

        //! Default constructor.
        VLANIdStack() = default;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //!
        //! Check if this VLAN id stack matches a template stack.
        //! @param other The template stack to compare with.
        //! @return True if this object contains at least as many elements as @a other and
        //! all elements in this object match their corresponding element en @a other.
        //! Two elements match if their values are identical or one contains a "null" value.
        //! Null values are ETHERTYPE_NULL and VLAN_ID_NULL.
        //!
        bool match(const VLANIdStack& other) const;
    };
    TS_POP_WARNING()
}
