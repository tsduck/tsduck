//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEPacket.h"
#include "tsIPv4Packet.h"
#include "tsMemory.h"

#define TS_DEFAULT_TTL 128  // Default Time To Live when creating datagrams.


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::MPEPacket::MPEPacket(const MPEPacket& other, ShareMode mode) :
    _is_valid(other._is_valid),
    _source_pid(other._source_pid),
    _dest_mac(other._dest_mac),
    _datagram()
{
    switch (mode) {
        case ShareMode::SHARE:
            _datagram = other._datagram;
            break;
        case ShareMode::COPY:
            if (other._is_valid) {
                _datagram = new ByteBlock(*other._datagram);
            }
            break;
        default:
            // should not get there
            assert (false);
    }
}

ts::MPEPacket::MPEPacket(MPEPacket&& other) noexcept :
    _is_valid(other._is_valid),
    _source_pid(other._source_pid),
    _dest_mac(other._dest_mac),
    _datagram(std::move(other._datagram))
{
}

ts::MPEPacket::MPEPacket(ByteBlockPtr datagram, ShareMode mode, const MACAddress& mac, PID pid) :
    _is_valid(!datagram.isNull() && FindUDP(datagram->data(), datagram->size())),
    _source_pid(pid),
    _dest_mac(mac)
{
    if (_is_valid) {
        switch (mode) {
            case ShareMode::SHARE:
                _datagram = datagram;
                break;
            case ShareMode::COPY:
                _datagram = new ByteBlock(*datagram);
                break;
            default:
                // should not get there
                assert (false);
        }
    }
}

ts::MPEPacket::MPEPacket(const Section& section)
{
    copy(section);
}


//----------------------------------------------------------------------------
// Locate UDP payload and size in our datagram.
//----------------------------------------------------------------------------

bool ts::MPEPacket::findUDP(const uint8_t** udpHeader, const uint8_t** udpAddress, size_t* udpSize) const
{
    return _is_valid && FindUDP(_datagram->data(), _datagram->size(), udpHeader, udpAddress, udpSize);
}

bool ts::MPEPacket::findUDP(uint8_t** udpHeader, uint8_t** udpAddress, size_t* udpSize)
{
    return _is_valid && FindUDP(_datagram->data(), _datagram->size(), const_cast<const uint8_t**>(udpHeader), const_cast<const uint8_t**>(udpAddress), udpSize);
}


//----------------------------------------------------------------------------
// Locate UDP payload and size in a datagram.
//----------------------------------------------------------------------------

bool ts::MPEPacket::FindUDP(const uint8_t* dgAddress, size_t dgSize, const uint8_t** udpHeader, const uint8_t** udpAddress, size_t* udpSize)
{
    // Validate presence of header and get its size.
    const size_t ipHeaderSize = IPv4Packet::IPHeaderSize(dgAddress, dgSize);
    if (ipHeaderSize == 0) {
        return false;
    }

    // The total length of the datagram is in byte 2.
    size_t length = GetUInt16(dgAddress + 2);

    // Byte 9 contains the protocol identifier.
    const uint8_t ipProto = dgAddress[IPv4_PROTOCOL_OFFSET];

    // Check that we have at least a complete UDP packet.
    if (ipProto != IPv4_PROTO_UDP || length < ipHeaderSize + UDP_HEADER_SIZE || dgSize < length) {
        return false;
    }

    // Total length of UPD header + data.
    length = GetUInt16(dgAddress + ipHeaderSize + 4);
    if (length < UDP_HEADER_SIZE || dgSize < ipHeaderSize + length) {
        return false;
    }

    // Found a valid UDP datagram.
    if (udpHeader != nullptr) {
        *udpHeader = dgAddress + ipHeaderSize;
    }
    if (udpAddress != nullptr) {
        *udpAddress = dgAddress + ipHeaderSize + UDP_HEADER_SIZE;
    }
    if (udpSize != nullptr) {
        *udpSize = length - UDP_HEADER_SIZE;
    }
    return true;
}


//----------------------------------------------------------------------------
// Clear content.
//----------------------------------------------------------------------------

void ts::MPEPacket::clear()
{
    _is_valid = false;
    _source_pid = PID_NULL;
    _dest_mac.clear();
    _datagram.clear();
}


//----------------------------------------------------------------------------
// Assignment and duplication.
//----------------------------------------------------------------------------

ts::MPEPacket& ts::MPEPacket::operator=(const MPEPacket& other)
{
    if (&other != this) {
        _is_valid = other._is_valid;
        _source_pid = other._source_pid;
        _dest_mac = other._dest_mac;
        _datagram = other._datagram;
    }
    return *this;
}

ts::MPEPacket& ts::MPEPacket::operator=(MPEPacket&& other) noexcept
{
    if (&other != this) {
        _is_valid = other._is_valid;
        _source_pid = other._source_pid;
        _dest_mac = other._dest_mac;
        _datagram = std::move(other._datagram);
    }
    return *this;
}

ts::MPEPacket& ts::MPEPacket::copy(const MPEPacket& other)
{
    if (&other != this) {
        _is_valid = other._is_valid;
        _source_pid = other._source_pid;
        _dest_mac = other._dest_mac;
        _datagram = other._is_valid ? new ByteBlock(*other._datagram) : nullptr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Copy content from a DSM-CC MPE section.
// See ETSI EN 301 192, section 7.1.
//----------------------------------------------------------------------------

ts::MPEPacket& ts::MPEPacket::copy(const Section& section)
{
    // Clear previous content.
    clear();

    // Locate the section content, including header.
    const uint8_t* data = section.content();
    size_t size = section.size();

    // We need a DSM-CC private data section.
    // The fixed part of the section is 12 bytes.
    // And there must be a 4-byte trailing checksum or CRC32.
    // The version field is used here as scrambling control and LLC/SNAP flag.
    // We do not support scrambled or LLC/SNAP encapsulated datagrams.
    if (!section.isValid() || section.tableId() != TID_DSMCC_PD || size < 16 || section.version() != 0) {
        // Invalid section for MPE.
        return *this;
    }

    // Get source PID.
    _source_pid = section.sourcePID();

    // Get destination MAC address.
    // The two least significant bytes are in fact the table id extension.
    _dest_mac.setAddress(data[11], data[10], data[9], data[8], data[4], data[3]);

    // Get the datagram from the rest of the section.
    // Do not include trailing 4 bytes (checksum or CRC32).
    _datagram = new ByteBlock(data + 12, size - 16);

    // Check that the datagram contains a UDP/IP packet.
    _is_valid = true;
    _is_valid = findUDP();

    // Finally got an MPE packet.
    return *this;
}


//----------------------------------------------------------------------------
// Create a DSM-CC MPE section containing the MPE packet.
//----------------------------------------------------------------------------

void ts::MPEPacket::createSection(Section& section) const
{
    // Clear previous content of section object.
    section.clear();

    // Leaave an empty section if this packet is invalid.
    if (!_is_valid) {
        return;
    }

    // Create a buffer for the section content.
    // The allocated area will be directly used by the section object.
    // The initial size is the fixed part of the section, before the datagram.
    ByteBlockPtr bbp(new ByteBlock(12));
    ByteBlock& data(*bbp);

    // Section is a DSM-CC Private Data.
    data[0] = TID_DSMCC_PD;
    // Size in bytes 1-2 will be filled later.
    // Get the MAC address bytes.
    _dest_mac.getAddress(data[11], data[10], data[9], data[8], data[4], data[3]);
    // Clear datagram, current section.
    data[5] = 0xC1;
    // Current section number
    data[6] = 0;
    // Last section number
    data[7] = 0;

    // Append datagram.
    data.append(*_datagram);

    // Reserve 4 bytes for the CRC32.
    data.enlarge(4);

    // Update size fields.
    // We set section_syntax_indicator=1 and private_indicator=0.
    PutUInt16(data.data() + 1, uint16_t(0xB000 | ((data.size() - 3) & 0x0FFF)));

    // Set the section content and recompute CRC32.
    section.reload(bbp, _source_pid, CRC32::COMPUTE);
}


//----------------------------------------------------------------------------
// Make sure the UDP datagram is valid.
//----------------------------------------------------------------------------

void ts::MPEPacket::configureUDP(bool force, size_t udpSize)
{
    if (force || !_is_valid) {
        // Recreate a new datagram.
        const size_t totalSize = IPv4_MIN_HEADER_SIZE + UDP_HEADER_SIZE + udpSize;

        if (_datagram.isNull()) {
            // Recreate a completely new datagram.
            // Zero is a suitable default for most fields.
            _datagram = new ByteBlock(totalSize, 0);
        }
        else {
            // Simply resize the current packet.
            // Existing fields such as addresses and ports are preserved.
            _datagram->resize(totalSize);
        }

        // Force required IP header fields.
        uint8_t* ip = _datagram->data();
        ip[0] = (IPv4_VERSION << 4) | (IPv4_MIN_HEADER_SIZE / sizeof(uint32_t));
        PutUInt16(ip + 2, uint16_t(totalSize));
        ip[8] = TS_DEFAULT_TTL;
        ip[9] = IPv4_PROTO_UDP;

        // Recompute IP header checksum.
        IPv4Packet::UpdateIPHeaderChecksum(ip, IPv4_MIN_HEADER_SIZE);

        // Set required UDP header fields.
        PutUInt16(ip + IPv4_MIN_HEADER_SIZE + 4, uint16_t(totalSize - IPv4_MIN_HEADER_SIZE));

        _is_valid = true;
    }
}


//----------------------------------------------------------------------------
// Get/set the source IP address.
//----------------------------------------------------------------------------

ts::IPv4Address ts::MPEPacket::sourceIPAddress() const
{
    IPv4Address addr;
    if (_is_valid) {
        assert(!_datagram.isNull());
        assert(_datagram->size() >= IPv4_MIN_HEADER_SIZE);
        addr.setAddress(GetUInt32(_datagram->data() +  IPv4_SRC_ADDR_OFFSET));
    }
    return addr;
}

void ts::MPEPacket::setSourceIPAddress(const IPv4Address& ip)
{
    // Make sure we have a valid datagram.
    configureUDP(false, 0);
    assert(!_datagram.isNull());
    assert(_datagram->size() >= IPv4_MIN_HEADER_SIZE);

    // Replace address.
    PutUInt32(_datagram->data() + IPv4_SRC_ADDR_OFFSET, ip.address());

    // Recompute IP header checksum.
    IPv4Packet::UpdateIPHeaderChecksum(_datagram->data(), _datagram->size());
}


//----------------------------------------------------------------------------
// Get/set the destination IP address.
//----------------------------------------------------------------------------

ts::IPv4Address ts::MPEPacket::destinationIPAddress() const
{
    IPv4Address addr;
    if (_is_valid) {
        assert(!_datagram.isNull());
        assert(_datagram->size() >= IPv4_MIN_HEADER_SIZE);
        addr.setAddress(GetUInt32(_datagram->data() +  IPv4_DEST_ADDR_OFFSET));
    }
    return addr;
}

void ts::MPEPacket::setDestinationIPAddress(const IPv4Address& ip)
{
    // Make sure we have a valid datagram.
    configureUDP(false, 0);
    assert(!_datagram.isNull());
    assert(_datagram->size() >= IPv4_MIN_HEADER_SIZE);

    // Replace address.
    PutUInt32(_datagram->data() + IPv4_DEST_ADDR_OFFSET, ip.address());

    // Recompute IP header checksum.
    IPv4Packet::UpdateIPHeaderChecksum(_datagram->data(), _datagram->size());
}


//----------------------------------------------------------------------------
// Get/set the UDP ports.
//----------------------------------------------------------------------------

uint16_t ts::MPEPacket::sourceUDPPort() const
{
    // Source port in bytes 0-1 of UDP header.
    const uint8_t* udpHeader = nullptr;
    return findUDP(&udpHeader) ? GetUInt16(udpHeader) : 0;
}

void ts::MPEPacket::setSourceUDPPort(uint16_t port)
{
    uint8_t* udpHeader = nullptr;
    if (findUDP(&udpHeader)) {
        // Source port in bytes 0-1 of UDP header.
        PutUInt16(udpHeader, port);
        // Force UDP header checksum to zero, meaning unused.
        udpHeader[6] = udpHeader[7] = 0;
    }
}

uint16_t ts::MPEPacket::destinationUDPPort() const
{
    // Destination port in bytes 2-3 of UDP header.
    const uint8_t* udpHeader = nullptr;
    return findUDP(&udpHeader) ? GetUInt16(udpHeader + 2) : 0;
}

void ts::MPEPacket::setDestinationUDPPort(uint16_t port)
{
    uint8_t* udpHeader = nullptr;
    if (findUDP(&udpHeader)) {
        // Destination port in bytes 2-3 of UDP header.
        PutUInt16(udpHeader + 2, port);
        // Force UDP header checksum to zero, meaning unused.
        udpHeader[6] = udpHeader[7] = 0;
    }
}


//----------------------------------------------------------------------------
// Get/set the source/destination socket address.
//----------------------------------------------------------------------------

ts::IPv4SocketAddress ts::MPEPacket::sourceSocket() const
{
    return IPv4SocketAddress(sourceIPAddress(), sourceUDPPort());
}

void ts::MPEPacket::setSourceSocket(const IPv4SocketAddress& sock)
{
    if (sock.hasAddress()) {
        setSourceIPAddress(sock);
    }
    if (sock.hasPort()) {
        setSourceUDPPort(sock.port());
    }
}

ts::IPv4SocketAddress ts::MPEPacket::destinationSocket() const
{
    return IPv4SocketAddress(destinationIPAddress(), destinationUDPPort());
}

void ts::MPEPacket::setDestinationSocket(const IPv4SocketAddress& sock)
{
    if (sock.hasAddress()) {
        setDestinationIPAddress(sock);
    }
    if (sock.hasPort()) {
        setDestinationUDPPort(sock.port());
    }
}


//----------------------------------------------------------------------------
// Access to the binary content of the UDP message.
//----------------------------------------------------------------------------

const uint8_t* ts::MPEPacket::udpMessage() const
{
    const uint8_t* addr = nullptr;
    return findUDP(nullptr, &addr) ? addr : nullptr;
}

size_t ts::MPEPacket::udpMessageSize() const
{
    size_t size = 0;
    return findUDP(nullptr, nullptr, &size) ? size : 0;
}


//----------------------------------------------------------------------------
// Replace the binary content of the UDP message.
//----------------------------------------------------------------------------

bool ts::MPEPacket::setUDPMessage(const uint8_t* data, size_t size)
{
    if (data == nullptr || size > 0xFFFF - IPv4_MIN_HEADER_SIZE - UDP_HEADER_SIZE) {
        // Incorrect parameters.
        return false;
    }
    else {
        // Make sure we have a valid datagram with the right size.
        configureUDP(true, size);

        // Locate UDP payload.
        uint8_t* udpAddress = nullptr;
        size_t udpSize = 0;
        findUDP(nullptr, &udpAddress, &udpSize);
        assert(udpAddress != nullptr);
        assert(udpSize == size);

        // Replace UDP payload.
        std::memcpy(udpAddress, data, size);
        return true;
    }
}
