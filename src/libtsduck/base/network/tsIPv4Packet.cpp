//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPv4Packet.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::IPv4Packet::IPv4Packet(const void* data, size_t size)
{
    reset(data, size);
}

void ts::IPv4Packet::clear()
{
    _valid = false;
    _proto_type = 0;
    _ip_header_size = 0;
    _proto_header_size = 0;
    _source_port = 0;
    _destination_port = 0;
    _data.clear();
}


//----------------------------------------------------------------------------
// Reinitialize the IPv4 packet with new content.
//----------------------------------------------------------------------------

bool ts::IPv4Packet::reset(const void* data, size_t size)
{
    // Clear previous content.
    clear();

    // Check that this looks like an IPv4 packet.
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    if ((_ip_header_size = IPHeaderSize(ip, size)) == 0 || GetUInt16BE(ip + IPv4_CHECKSUM_OFFSET) != IPHeaderChecksum(ip, _ip_header_size)) {
        return false; // not a valid IP packet.
    }

    // Packet size in header.
    size = std::min<size_t>(size, GetUInt16(ip + IPv4_LENGTH_OFFSET));

    // Validate and filter by protocol.
    switch (_proto_type = ip[IPv4_PROTOCOL_OFFSET]) {
        case IPv4_PROTO_TCP: {
            if (size < _ip_header_size + TCP_MIN_HEADER_SIZE) {
                return false; // packet too short
            }
            _proto_header_size = 4 * size_t((ip[_ip_header_size + TCP_HEADER_LENGTH_OFFSET] >> 4) & 0x0F);
            if (size < _ip_header_size + _proto_header_size || _proto_header_size < TCP_MIN_HEADER_SIZE) {
                return false; // packet too short
            }
            _source_port = GetUInt16BE(ip + _ip_header_size + TCP_SRC_PORT_OFFSET);
            _destination_port = GetUInt16BE(ip + _ip_header_size + TCP_DEST_PORT_OFFSET);
            break;
        }
        case IPv4_PROTO_UDP: {
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
            _source_port = GetUInt16BE(ip + _ip_header_size + UDP_SRC_PORT_OFFSET);
            _destination_port = GetUInt16BE(ip + _ip_header_size + UDP_DEST_PORT_OFFSET);
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

bool ts::IPv4Packet::fragmented() const
{
    return _valid && (
        (_data[IPv4_FRAGMENT_OFFSET] & 0x20) != 0 ||                      // "More Fragments" bit set
        (GetUInt16BE(_data.data() + IPv4_FRAGMENT_OFFSET) & 0x1FFF) != 0  // "Fragment Offset" not zero
    );
}


//----------------------------------------------------------------------------
// Get the source and destination IPv4 addresses and ports.
//----------------------------------------------------------------------------

ts::IPv4Address ts::IPv4Packet::sourceAddress() const
{
    if (_valid) {
        assert(_data.size() >= IPv4_SRC_ADDR_OFFSET + 4);
        return IPv4Address(GetUInt32BE(&_data[IPv4_SRC_ADDR_OFFSET]));
    }
    else {
        return IPv4Address(); // invalid address
    }
}

ts::IPv4Address ts::IPv4Packet::destinationAddress() const
{
    if (_valid) {
        assert(_data.size() >= IPv4_DEST_ADDR_OFFSET + 4);
        return IPv4Address(GetUInt32BE(&_data[IPv4_DEST_ADDR_OFFSET]));
    }
    else {
        return IPv4Address(); // invalid address
    }
}

ts::IPv4SocketAddress ts::IPv4Packet::sourceSocketAddress() const
{
    if (_valid) {
        assert(_data.size() >= IPv4_SRC_ADDR_OFFSET + 4);
        return IPv4SocketAddress(GetUInt32BE(&_data[IPv4_SRC_ADDR_OFFSET]), _source_port);
    }
    else {
        return IPv4SocketAddress(); // invalid address
    }
}

ts::IPv4SocketAddress ts::IPv4Packet::destinationSocketAddress() const
{
    if (_valid) {
        assert(_data.size() >= IPv4_DEST_ADDR_OFFSET + 4);
        return IPv4SocketAddress(GetUInt32BE(&_data[IPv4_DEST_ADDR_OFFSET]), _destination_port);
    }
    else {
        return IPv4SocketAddress(); // invalid address
    }
}


//----------------------------------------------------------------------------
// Get the TCP characteristics in the packet.
//----------------------------------------------------------------------------

uint32_t ts::IPv4Packet::tcpSequenceNumber() const
{
    return isTCP() ? GetUInt32BE(&_data[_ip_header_size + TCP_SEQUENCE_OFFSET]) : 0;
}

bool ts::IPv4Packet::tcpSYN() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x02) != 0;
}

bool ts::IPv4Packet::tcpACK() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x10) != 0;
}

bool ts::IPv4Packet::tcpRST() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x04) != 0;
}

bool ts::IPv4Packet::tcpFIN() const
{
    return isTCP() && (_data[_ip_header_size + TCP_FLAGS_OFFSET] & 0x01) != 0;
}


//----------------------------------------------------------------------------
// Get the size in bytes of an IPv4 header.
//----------------------------------------------------------------------------

size_t ts::IPv4Packet::IPHeaderSize(const void* data, size_t size)
{
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    size_t headerSize = 0;

    // The first byte of the header contains the IP version and the number
    // of 32-bit words in the header.
    if (ip != nullptr && size >= IPv4_MIN_HEADER_SIZE && ((ip[0] >> 4) & 0x0F) == IPv4_VERSION) {
        headerSize = sizeof(uint32_t) * size_t(ip[0] & 0x0F);
    }

    return headerSize <= size ? headerSize : 0;
}


//----------------------------------------------------------------------------
// Compute the checksum of an IPv4 header.
//----------------------------------------------------------------------------

uint16_t ts::IPv4Packet::IPHeaderChecksum(const void* data, size_t size)
{
    const size_t hSize = IPHeaderSize(data, size);
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    uint32_t checksum = 0;

    // Add all 16-bit words in the header (except checksum).
    for (size_t i = 0; i < hSize; i += 2) {
        if (i != IPv4_CHECKSUM_OFFSET) {
            checksum += GetUInt16(ip + i);
        }
    }

    // Add carries until they are all gone.
    while (checksum > 0xFFFF) {
        checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }

    // Take the complement.
    return hSize == 0 ? 0 : uint16_t(checksum ^ 0xFFFF);
}


//----------------------------------------------------------------------------
// Verify or update the checksum of an IPv4 header.
//----------------------------------------------------------------------------

bool ts::IPv4Packet::VerifyIPHeaderChecksum(const void* data, size_t size)
{
    const bool ok = IPHeaderSize(data, size) > 0;
    const uint8_t* ip = reinterpret_cast<const uint8_t*>(data);
    return ok && GetUInt16(ip + IPv4_CHECKSUM_OFFSET) == IPHeaderChecksum(data, size);
}

bool ts::IPv4Packet::UpdateIPHeaderChecksum(void* data, size_t size)
{
    const bool ok = IPHeaderSize(data, size) > 0;
    if (ok) {
        uint8_t* ip = reinterpret_cast<uint8_t*>(data);
        PutUInt16(ip + IPv4_CHECKSUM_OFFSET, IPHeaderChecksum(data, size));
    }
    return ok;
}
