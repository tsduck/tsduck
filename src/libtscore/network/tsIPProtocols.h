//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtscore net
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
        ETHERTYPE_IPv4    = 0x0800,  //!< Ethernet protocol: IPv4.
        ETHERTYPE_ARP     = 0x0806,  //!< Ethernet protocol: ARP.
        ETHERTYPE_WOL     = 0x0842,  //!< Ethernet protocol: Wake-on-LAN.
        ETHERTYPE_RARP    = 0x8035,  //!< Ethernet protocol: RARP.
        ETHERTYPE_802_1Q  = 0x8100,  //!< Ethernet protocol: a 2-byte IEEE 802.1Q tag (VLAN) after EtherType, then real EtherType.
        ETHERTYPE_IPv6    = 0x86DD,  //!< Ethernet protocol: IPv6.
        ETHERTYPE_802_1AD = 0x88A8,  //!< Ethernet protocol: IEEE 802.1ad nested VLAN.
        ETHERTYPE_802_1AH = 0x88E7,  //!< Ethernet protocol: IEEE 802.1ah, Provider Backbone Bridges (PBB), aka MAC-in-MAC (MIM).
        ETHERTYPE_NULL    = 0xFFFF,  //!< Invalid Ethernet protocol identifier, can be used as placeholder.
    };

    //------------------------------------------------------------------------
    // IP protocol.
    //------------------------------------------------------------------------

    //
    // IPv4 header.
    //
    constexpr uint8_t IPv4_VERSION          =     4;   //!< Protocol version of IPv4 is ... 4 !
    constexpr size_t  IPv4_LENGTH_OFFSET    =     2;   //!< Offset of the total packet length in an IPv4 header.
    constexpr size_t  IPv4_FRAGMENT_OFFSET  =     6;   //!< Offset of the flags and fragments in an IPv4 header.
    constexpr size_t  IPv4_PROTOCOL_OFFSET  =     9;   //!< Offset of the protocol identifier in an IPv4 header.
    constexpr size_t  IPv4_CHECKSUM_OFFSET  =    10;   //!< Offset of the checksum in an IPv4 header.
    constexpr size_t  IPv4_SRC_ADDR_OFFSET  =    12;   //!< Offset of source IP address in an IPv4 header.
    constexpr size_t  IPv4_DEST_ADDR_OFFSET =    16;   //!< Offset of destination IP address in an IPv4 header.
    constexpr size_t  IPv4_MIN_HEADER_SIZE  =    20;   //!< Minimum size of an IPv4 header.
    constexpr size_t  IP_MAX_PACKET_SIZE    = 65536;   //!< Maximum size of an IP packet.

    //
    // IPv6 header.
    //
    constexpr uint8_t IPv6_VERSION          =  6;   //!< Protocol version of IPv6 is ... 6 !
    constexpr size_t  IPv6_LENGTH_OFFSET    =  4;   //!< Offset of the 16-bit payload length in an IPv6 header.
    constexpr size_t  IPv6_NEXTHEAD_OFFSET  =  6;   //!< Offset of the 8-bit "next header" field in an IPv6 header.
    constexpr size_t  IPv6_SRC_ADDR_OFFSET  =  8;   //!< Offset of source IP address in an IPv6 header.
    constexpr size_t  IPv6_DEST_ADDR_OFFSET = 24;   //!< Offset of destination IP address in an IPv6 header.
    constexpr size_t  IPv6_MIN_HEADER_SIZE  = 40;   //!< Minimum size of an IPv6 header.
    constexpr size_t  IPv6_EXT_HEADER_SIZE  =  8;   //!< Minimum size of an extended IPv6 header.

    //!
    //! Selected IP protocol identifiers.
    //!
    enum : uint8_t {
        IP_SUBPROTO_HOPxHOP  =   0,  //!< IP protocol: IPv6 extension header, Hop-by-Hop.
        IP_SUBPROTO_ICMP     =   1,  //!< IP protocol: Internet Control Message Protocol (ICMP).
        IP_SUBPROTO_IGMP     =   2,  //!< IP protocol: Internet Group Management Protocol (IGMP).
        IP_SUBPROTO_TCP      =   6,  //!< IP protocol: Transmission Control Protocol (TCP).
        IP_SUBPROTO_UDP      =  17,  //!< IP protocol: User Datagram Protocol (UDP).
        IP_SUBPROTO_V6_ENCAP =  41,  //!< IP protocol: IPv6 encapsulation.
        IP_SUBPROTO_ROUTING  =  43,  //!< IP protocol: IPv6 extension header, routing.
        IP_SUBPROTO_FRAGMENT =  44,  //!< IP protocol: IPv6 extension header, fragmentation of datagrams.
        IP_SUBPROTO_OSPF     =  89,  //!< IP protocol: Open Shortest Path First (OSPF).
        IP_SUBPROTO_SCTP     = 132,  //!< IP protocol: Stream Control Transmission Protocol (SCTP).
    };

    //!
    //! Get the name of an IP protocol (UDP, TCP, etc).
    //! @ingroup net
    //! @param [in] protocol Protocol identifier, as set in IP header.
    //! @param [in] long_format If false (the default), return a simple acronym.
    //! When true, return a full description string.
    //! @return The protocol name with optional description.
    //!
    TSCOREDLL UString IPProtocolName(uint8_t protocol, bool long_format = false);

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
    //! @ingroup net
    //! @param [in] seq1 First TCP sequence number.
    //! @param [in] seq2 Second TCP sequence number.
    //! @return True if @a seq1 strictly logically preceeds @a seq2.
    //!
    TSCOREDLL bool TCPOrderedSequence(uint32_t seq1, uint32_t seq2);

    //!
    //! Compute the difference between two TCP sequence numbers, including wrapping back at 0xFFFFFFFF.
    //! @ingroup net
    //! @param [in] seq1 First TCP sequence number.
    //! @param [in] seq2 Second TCP sequence number which must logically follow @a seq1.
    //! @return The difference between @a seq1 and @a seq2.
    //!
    TSCOREDLL uint32_t TCPSequenceDiff(uint32_t seq1, uint32_t seq2);

    //------------------------------------------------------------------------
    // Real-time Transport Protocol (RTP)
    //------------------------------------------------------------------------

    constexpr size_t   RTP_HEADER_SIZE =    12;  //!< Size in bytes of the fixed part of the RTP header.
    constexpr uint8_t  RTP_PT_MP2T     =    33;  //!< RTP payload type for MPEG2-TS.
    constexpr uint64_t RTP_RATE_MP2T   = 90000;  //!< RTP clock rate for MPEG2-TS.

    //!
    //! Definition of a number of RTP clock units as a std::chrono::duration type.
    //! @ingroup net
    //!
    using rtp_units = cn::duration<std::intmax_t, std::ratio<1, RTP_RATE_MP2T>>;

    //------------------------------------------------------------------------
    // Hyper-Text Transfer Protocol (HTTP)
    //------------------------------------------------------------------------

    //!
    //! Get the standard text for a HTTP status code.
    //! @param [in] status HTTP status code.
    //! @return The status text.
    //!
    TSCOREDLL UString HTTPStatusText(int status);

    //------------------------------------------------------------------------
    // VLAN encapsulation.
    //------------------------------------------------------------------------

    constexpr uint32_t VLAN_ID_NULL = 0xFFFFFFFF;  //!< Invalid VLAN identifier, can be used as placeholder.

    //!
    //! A structure which describes a VLAN identification.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL VLANId
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
        //! @endcond
    };

    //!
    //! A stack of VLAN identifiers, from outer to inner VLAN.
    //! @ingroup libtscore net
    //!
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(non-virtual-dtor) // The derived class allocates nothing and we only need the base destructor
    class TSCOREDLL VLANIdStack : public std::vector<VLANId>, public StringifyInterface
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
