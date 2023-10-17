//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a raw IPv4 packet.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsIPv4Address.h"
#include "tsIPv4SocketAddress.h"
#include "tsIPProtocols.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a raw IPv4 packet.
    //! @ingroup net
    //!
    class TSDUCKDLL IPv4Packet
    {
    public:
        //!
        //! The concept of port is used by TCP and UDP.
        //!
        typedef AbstractNetworkAddress::Port Port;

        //!
        //! Default constructor.
        //!
        IPv4Packet() = default;

        //!
        //! Constructor from raw content.
        //! @param [in] data Address of the IP packet data.
        //! @param [in] size Size of the IP packet data.
        //!
        IPv4Packet(const void* data, size_t size);

        //!
        //! Reinitialize the IPv4 packet with new content.
        //! @param [in] data Address of the IP packet data.
        //! @param [in] size Size of the IP packet data.
        //! @return True on success, false if the packet is invalid.
        //!
        bool reset(const void* data, size_t size);

        //!
        //! Clear the packet content.
        //!
        void clear();

        //!
        //! Check if the IPv4 packet is valid.
        //! @return True if the packet is valid, false otherwise.
        //!
        bool isValid() const { return _valid; }

        //!
        //! Get the sub-protocol type (TCP, UDP, etc).
        //! @return The sub-protocol type, as defined by constants IPv4_PROTO_*.
        //!
        uint8_t protocol() const { return _proto_type; }

        //!
        //! Check if the packet is a valid TCP packet.
        //! @return True if the packet is a valid TCP packet.
        //!
        bool isTCP() const { return _valid && _proto_type == IPv4_PROTO_TCP; }

        //!
        //! Check if the packet is a valid UDP packet.
        //! @return True if the packet is a valid UDP packet.
        //!
        bool isUDP() const { return _valid && _proto_type == IPv4_PROTO_UDP; }

        //!
        //! Get the address of the IPv4 packet content.
        //! @return The address of the IPv4 packet content or a null pointer if the packet is invalid.
        //!
        const uint8_t* data() const { return _valid ? _data.data() : nullptr; }

        //!
        //! Get the size in bytes of the IPv4 packet content.
        //! @return The size in bytes of the IPv4 packet content.
        //!
        size_t size() const { return _valid ? _data.size() : 0; }

        //!
        //! Get the address of the IPv4 header.
        //! @return The address of the IPv4 header or a null pointer if the packet is invalid.
        //!
        const uint8_t* ipHeader() const { return _valid ? _data.data() : nullptr; }

        //!
        //! Get the size in bytes of the IPv4 header.
        //! @return The size in bytes of the IPv4 header.
        //!
        size_t ipHeaderSize() const { return _valid ? _ip_header_size : 0; }

        //!
        //! Get the address of the sub-protocol header (TCP header, UDP header, etc).
        //! @return The address of the sub-protocol header or a null pointer if the packet is invalid.
        //!
        const uint8_t* protocolHeader() const { return _valid ? _data.data() + _ip_header_size : nullptr; }

        //!
        //! Get the size in bytes of the sub-protocol header (TCP header, UDP header, etc).
        //! @return The size in bytes of the sub-protocol header.
        //!
        size_t protocolHeaderSize() const { return _valid ? _proto_header_size : 0; }

        //!
        //! Get the address of the sub-protocol payload data (TCP data, UDP data, etc).
        //! @return The address of the sub-protocol header payload data or a null pointer if the packet is invalid.
        //!
        const uint8_t* protocolData() const { return _valid ? _data.data() + _ip_header_size + _proto_header_size : nullptr; }

        //!
        //! Get the size in bytes of the sub-protocol payload data (TCP data, UDP data, etc).
        //! @return The size in bytes of the sub-protocol payload data.
        //!
        size_t protocolDataSize() const { return _valid ? _data.size() - _ip_header_size - _proto_header_size : 0; }

        //!
        //! Check if the IPv4 packet is fragmented.
        //! @return True if the packet is just a fragment of a larger packet.
        //!
        bool fragmented() const;

        //!
        //! Get the source IPv4 address.
        //! @return The source IPv4 address.
        //!
        IPv4Address sourceAddress() const;

        //!
        //! Get the destination IPv4 address.
        //! @return The destination IPv4 address.
        //!
        IPv4Address destinationAddress() const;

        //!
        //! Get the TCP or UDP source port.
        //! @return The TCP or UDP source port, zero for other protocols.
        //!
        Port sourcePort() const { return _valid ? _source_port : 0; }

        //!
        //! Get the TCP or UDP destination port.
        //! @return The TCP or UDP destination port, zero for other protocols.
        //!
        Port destinationPort() const { return _valid ? _destination_port : 0; }

        //!
        //! Get the source IPv4 socket address.
        //! @return The source IPv4 socket address.
        //!
        IPv4SocketAddress sourceSocketAddress() const;

        //!
        //! Get the destination IPv4 socket address.
        //! @return The destination IPv4 socket address.
        //!
        IPv4SocketAddress destinationSocketAddress() const;

        //!
        //! Get the TCP sequence number in the packet.
        //! @return The TCP sequence number or zero if this is not a TCP packet.
        //!
        uint32_t tcpSequenceNumber() const;

        //!
        //! Get the TCP SYN flag in the packet.
        //! @return The TCP SYN flag.
        //!
        bool tcpSYN() const;

        //!
        //! Get the TCP ACK flag in the packet.
        //! @return The TCP ACK flag.
        //!
        bool tcpACK() const;

        //!
        //! Get the TCP RST flag in the packet.
        //! @return The TCP RST flag.
        //!
        bool tcpRST() const;

        //!
        //! Get the TCP FIN flag in the packet.
        //! @return The TCP FIN flag.
        //!
        bool tcpFIN() const;

        //!
        //! Get the size in bytes of an IPv4 header from raw data.
        //! @param [in] data Address of the IP packet.
        //! @param [in] size Size of the IP packet or header (must be larger than the header size).
        //! @return The size in bytes of the IP header or zero on error.
        //!
        static size_t IPHeaderSize(const void* data, size_t size);

        //!
        //! Compute the checksum of an IPv4 header from raw data.
        //! @param [in] data Address of the IP packet.
        //! @param [in] size Size of the IP packet or header (must be larger than the header size).
        //! @return The computed checksum of the header.
        //!
        static uint16_t IPHeaderChecksum(const void* data, size_t size);

        //!
        //! Verify the checksum of an IPv4 header from raw data.
        //! @param [in] data Address of the IP packet.
        //! @param [in] size Size of the IP packet or header (must be larger than the header size).
        //! @return True if the checksum of the header if correct, false otherwise.
        //!
        static bool VerifyIPHeaderChecksum(const void* data, size_t size);

        //!
        //! Update the checksum of an IPv4 header as raw data.
        //! @param [in,out] data Address of the IP packet.
        //! @param [in] size Size of the IP packet or header (must be larger than the header size).
        //! @return True if the checksum was updated, false on incorrect buffer.
        //!
        static bool UpdateIPHeaderChecksum(void* data, size_t size);

    private:
        bool      _valid = false;
        uint8_t   _proto_type = 0;
        size_t    _ip_header_size = 0;
        size_t    _proto_header_size = 0;
        Port      _source_port = 0;
        Port      _destination_port = 0;
        ByteBlock _data {};
    };
}
