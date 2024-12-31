//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPPacket::IPPacket(const void* data, size_t size)
{
    reset(data, size);
}

void ts::IPPacket::clear()
{
    _valid = false;
    _proto_type = 0;
    _ip_header_size = 0;
    _proto_header_size = 0;
    _source.clear();
    _destination.clear();
    _data.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the IPv4 packet with new content.
//----------------------------------------------------------------------------

bool ts::IPPacket::reset(const void* data, size_t size)
{
    // Clear previous content.
    clear();

    // Check that this looks like an IPv4 or IPv6 packet. Get header size.
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    if ((_ip_header_size = IPHeaderSize(ip, size, &_proto_type)) == 0) {
        return false;
    }

    // The IP version is in the first 4 bits.
    const uint8_t version = ip[0] >> 4;
    if (version == IPv4_VERSION) {
        // Verify the IPv4 header checksum. Some IPv4 implementation leaves it to zero, meaning not computed, don't check.
        const uint16_t checksum = GetUInt16BE(ip + IPv4_CHECKSUM_OFFSET);
        if (checksum != 0 && checksum != IPHeaderChecksum(ip, _ip_header_size)) {
            return false;
        }

        // Extract complete packet size from header.
        size = std::min<size_t>(size, GetUInt16(ip + IPv4_LENGTH_OFFSET));

        // Source and destination addresses.
        _source.setAddress(GetUInt32BE(ip + IPv4_SRC_ADDR_OFFSET));
        _destination.setAddress(GetUInt32BE(ip + IPv4_DEST_ADDR_OFFSET));
    }
    else {
        assert(version == IPv6_VERSION);
        // Complete packet size.
        size = std::min<size_t>(size, IPv6_MIN_HEADER_SIZE + GetUInt16(ip + IPv6_LENGTH_OFFSET));

        // Source and destination addresses.
        _source.setAddress(ip + IPv6_SRC_ADDR_OFFSET, IPAddress::BYTES6);
        _destination.setAddress(ip + IPv6_DEST_ADDR_OFFSET, IPAddress::BYTES6);
    }

    // Validate and filter by protocol.
    switch (_proto_type) {
        case IP_SUBPROTO_TCP: {
            if (size < _ip_header_size + TCP_MIN_HEADER_SIZE) {
                return false; // packet too short
            }
            _proto_header_size = 4 * size_t((ip[_ip_header_size + TCP_HEADER_LENGTH_OFFSET] >> 4) & 0x0F);
            if (size < _ip_header_size + _proto_header_size || _proto_header_size < TCP_MIN_HEADER_SIZE) {
                return false; // packet too short
            }
            _source.setPort(GetUInt16BE(ip + _ip_header_size + TCP_SRC_PORT_OFFSET));
            _destination.setPort(GetUInt16BE(ip + _ip_header_size + TCP_DEST_PORT_OFFSET));
            break;
        }
        case IP_SUBPROTO_UDP: {
            if (size < _ip_header_size + UDP_HEADER_SIZE) {
                return false; // packet too short
            }
            const size_t udp_length = GetUInt16BE(ip + _ip_header_size + UDP_LENGTH_OFFSET);
            if (size < _ip_header_size + udp_length) {
                return false; // packet too short
            }
            else if (size > _ip_header_size + udp_length) {
                // Suspect packet, data after UDP payload, should we consider it as invalid?
                size = _ip_header_size + udp_length;
            }
            _proto_header_size = UDP_HEADER_SIZE;
            _source.setPort(GetUInt16BE(ip + _ip_header_size + UDP_SRC_PORT_OFFSET));
            _destination.setPort(GetUInt16BE(ip + _ip_header_size + UDP_DEST_PORT_OFFSET));
            break;
        }
        default:
            _proto_header_size = 0;
            break;
    }

    // Packet is valid.
    _data.copy(data, size);
    return _valid = true;
}


//----------------------------------------------------------------------------
// Check if the IPv4 packet is fragmented.
//----------------------------------------------------------------------------

bool ts::IPPacket::fragmented() const
{
    return _valid && _source.generation() == IP::v4 && (
        (_data[IPv4_FRAGMENT_OFFSET] & 0x20) != 0 ||                      // "More Fragments" bit set
        (GetUInt16BE(_data.data() + IPv4_FRAGMENT_OFFSET) & 0x1FFF) != 0  // "Fragment Offset" not zero
    );
}


//----------------------------------------------------------------------------
// Get the TCP characteristics in the packet.
//----------------------------------------------------------------------------

uint32_t ts::IPPacket::tcpSequenceNumber() const
{
    return isTCP() ? GetUInt32BE(&_data[_ip_header_size + TCP_SEQUENCE_OFFSET]) : 0;
}

bool ts::IPPacket::tcpSYN() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x02) != 0;
}

bool ts::IPPacket::tcpACK() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x10) != 0;
}

bool ts::IPPacket::tcpRST() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x04) != 0;
}

bool ts::IPPacket::tcpFIN() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x01) != 0;
}


//----------------------------------------------------------------------------
// Get the size in bytes of an IPv4 header.
//----------------------------------------------------------------------------

size_t ts::IPPacket::IPHeaderSize(const void* data, size_t size, uint8_t* protocol)
{
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    size_t hsize = 0;

    if (ip != nullptr && size >= 1) {
        // The IP version is in the first 4 bits.
        const uint8_t version = ip[0] >> 4;
        if (version == IPv4_VERSION && size >= IPv4_MIN_HEADER_SIZE) {
            // IPv4 header.
            hsize = sizeof(uint32_t) * size_t(ip[0] & 0x0F);
            if (protocol != nullptr) {
                *protocol = ip[IPv4_PROTOCOL_OFFSET];
            }
        }
        else if (version == IPv6_VERSION && size >= IPv6_MIN_HEADER_SIZE) {
            // IPv6 header.
            hsize = IPv6_MIN_HEADER_SIZE;
            uint8_t next = ip[IPv6_NEXTHEAD_OFFSET];
            // Skip a few known extension headers. There may be additional headers,
            // in which case the next one will be interpreted as "next protocol layer".
            while (hsize + IPv6_EXT_HEADER_SIZE >= size && (next == IP_SUBPROTO_HOPxHOP || next == IP_SUBPROTO_ROUTING || next == IP_SUBPROTO_FRAGMENT)) {
                // Size in bytes of extended header.
                const size_t ext_size = 8 + 8 * ip[hsize + 1];
                if (hsize + ext_size > size) {
                    // Invalid packet, extended header does not fit.
                    hsize = 0;
                    break;
                }
                else {
                    next = ip[hsize];
                    hsize += ext_size;
                }
            }
            if (protocol != nullptr) {
                *protocol = next;
            }
        }
    }

    // If the expected header size is larger than the data, then the header is incorrect.
    return hsize <= size ? hsize : 0;
}


//----------------------------------------------------------------------------
// Compute the checksum of an IPv4 header.
//----------------------------------------------------------------------------

uint16_t ts::IPPacket::IPHeaderChecksum(const void* data, size_t size)
{
    const size_t hsize = IPHeaderSize(data, size);
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    const uint8_t version = hsize > 0 ? (ip[0] >> 4) : 0;
    uint32_t checksum = 0;
    if (version == IPv4_VERSION) {

        // Add all 16-bit words in the header (except checksum).
        for (size_t i = 0; i < hsize; i += 2) {
            if (i != IPv4_CHECKSUM_OFFSET) {
                checksum += GetUInt16(ip + i);
            }
        }

        // Add carries until they are all gone.
        while (checksum > 0xFFFF) {
            checksum = (checksum & 0xFFFF) + (checksum >> 16);
        }

        // Take the complement.
        checksum = uint16_t(checksum ^ 0xFFFF);
    }
    return uint16_t(checksum);
}


//----------------------------------------------------------------------------
// Verify or update the checksum of an IPv4 header.
//----------------------------------------------------------------------------

bool ts::IPPacket::VerifyIPHeaderChecksum(const void* data, size_t size)
{
    bool ok = IPHeaderSize(data, size) > 0;
    if (ok) {
        const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
        const uint8_t version = (ip[0] >> 4);
        if (version == IPv4_VERSION) {
            const uint16_t checksum = GetUInt16(ip + IPv4_CHECKSUM_OFFSET);
            // When checksum is zero, this means "don't verify checksum".
            ok = checksum == 0 || checksum == IPHeaderChecksum(data, size);
        }
    }
    return ok;
}

bool ts::IPPacket::UpdateIPHeaderChecksum(void* data, size_t size)
{
    const bool ok = IPHeaderSize(data, size) > 0;
    if (ok) {
        uint8_t* ip = reinterpret_cast<uint8_t*>(data);
        const uint8_t version = (ip[0] >> 4);
        if (version == IPv4_VERSION) {
            PutUInt16(ip + IPv4_CHECKSUM_OFFSET, IPHeaderChecksum(data, size));
        }
    }
    return ok;
}
