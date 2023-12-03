//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPcapFile.h"
#include "tsIPv4Packet.h"
#include "tsByteBlock.h"
#include "tsIntegerUtils.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PcapFile::~PcapFile()
{
    close();
}


//----------------------------------------------------------------------------
// Report an error (if fmt is not empty), set error indicator, return false.
//----------------------------------------------------------------------------

bool ts::PcapFile::error(Report& report, const UString& fmt, std::initializer_list<ArgMixIn> args)
{
    if (!fmt.empty()) {
        report.error(fmt, args);
    }
    _error = true;
    return false;
}


//----------------------------------------------------------------------------
// Open the file for read.
//----------------------------------------------------------------------------

bool ts::PcapFile::open(const fs::path& filename, Report& report)
{
    if (_in != nullptr) {
        report.error(u"already open");
        return false;
    }

    // Reset counters.
    _error = false;
    _file_size = 0;
    _packet_count = 0;
    _ipv4_packet_count = 0;
    _packets_size = 0;
    _ipv4_packets_size = 0;
    _first_timestamp = -1;
    _last_timestamp = -1;

    // Open the file.
    if (filename.empty() || filename == u"-") {
        // Use standard input.
        if (!SetBinaryModeStdin(report)) {
            return false;
        }
        _in = &std::cin;
        _name = u"standard input";
    }
    else {
        _file.open(filename, std::ios::in | std::ios::binary);
        if (!_file) {
            report.error(u"error opening %s", {filename});
            return false;
        }
        _in = &_file;
        _name = filename;
    }

    // Read the file header, starting with a 4-byte "magic" number.
    uint8_t magic[4];
    if (!readall(magic, sizeof(magic), report) || !readHeader(GetUInt32BE(magic), report)) {
        close();
        return false;
    }

    report.debug(u"opened %s, %s format version %d.%d, %s endian", {_name, _ng ? u"pcap-ng" : u"pcap", _major, _minor, _be ? u"big" : u"little"});
    return true;
}


//----------------------------------------------------------------------------
// Close the file.
//----------------------------------------------------------------------------

void ts::PcapFile::close()
{
    if (_file.is_open()) {
        _file.close();
    }
    _in = nullptr;
}


//----------------------------------------------------------------------------
// Read exactly "size" bytes. Return false if not enough bytes before eof.
//----------------------------------------------------------------------------

bool ts::PcapFile::readall(uint8_t* data, size_t size, Report& report)
{
    // Repeatedly read until all requested bytes are read.
    while (size > 0) {
        // Read at most "size" bytes.
        if (!_in->read(reinterpret_cast<char*>(data), size)) {
            // Read error, don't display error on end-of-file.
            if (!_in->eof()) {
                report.error(u"error reading %s", {_name});
            }
            return error(report);
        }

        // Get file size so far.
        const std::ios::pos_type fpos = _in->tellg();
        if (fpos != std::ios::pos_type(-1)) {
            _file_size = uint64_t(fpos);
        }

        // Actual number of bytes.
        const size_t insize = std::min(size_t(_in->gcount()), size);
        size -= insize;
        data += insize;
    }
    return true;
}


//----------------------------------------------------------------------------
// Read a file header, starting from a magic which was read as big endian.
//----------------------------------------------------------------------------

bool ts::PcapFile::readHeader(uint32_t magic, Report& report)
{
    switch (magic) {
        case PCAP_MAGIC_BE:
        case PCAP_MAGIC_LE:
        case PCAPNS_MAGIC_BE:
        case PCAPNS_MAGIC_LE: {
            // This is a pcap file. Read 20 additional bytes for the rest of the header.
            uint8_t header[20];
            if (!readall(header, sizeof(header), report)) {
                return error(report);
            }
            _ng = false;
            _be = magic == PCAP_MAGIC_BE || magic == PCAPNS_MAGIC_BE;
            _major = get16(header);
            _minor = get16(header + 2);
            _if.resize(1); // only one interface in pcap files
            _if[0].link_type = get16(header + 18);
            _if[0].time_units = magic == PCAP_MAGIC_BE || magic == PCAP_MAGIC_LE ? MicroSecPerSec : NanoSecPerSec;
            _if[0].fcs_size = (header[16] & 0x10) == 0 ? 0 : 2 * ((header[16] >> 5) & 0x07);
            break;
        }
        case PCAPNG_MAGIC: {
            // This is a pcap-ng file. Read the complete section header, compute endianness.
            _ng = true;
            ByteBlock header;
            if (!readNgBlockBody(magic, header, report)) {
                return error(report);
            }
            if (header.size() < 16) {
                return error(report, u"invalid pcap-ng file, truncated section header in %s", {_name});
            }
            _major = get16(header.data() + 4);
            _minor = get16(header.data() + 6);
            _if.clear(); // will read interface descriptions in dedicated blocks.
            break;
        }
        default: {
            return error(report, u"invalid pcap file, unknown magic number 0x%X", {magic});
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Analyze a pcap-ng interface description.
//----------------------------------------------------------------------------

bool ts::PcapFile::analyzeNgInterface(const uint8_t* data, size_t size, Report& report)
{
    if (data == nullptr || size < 8) {
        return error(report, u"invalid pcap-ng interface description, %d bytes", {size});
    }

    InterfaceDesc ifd;
    ifd.link_type = get16(data);
    ifd.time_units = MicroSecPerSec;

    // Loop on options. Each option has 16-bit tag and 16-bit length.
    const uint8_t* end = data + size;
    data += 8;
    while (data + 4 <= end) {

        // Get tag and length.
        const uint16_t tag = get16(data);
        const uint16_t len = get16(data + 2);
        data += 4;
        if (data + len > end) {
            return error(report, u"corrupted option list in pcap-ng interface descriptiorn");
        }

        if (tag == PCAPNG_IF_FCSLEN && len == 1) {
            ifd.fcs_size = data[0];
        }
        else if (tag == PCAPNG_IF_TSOFFSET && len == 8) {
            ifd.time_offset = MicroSecPerSec * get64(data);
        }
        else if (tag == PCAPNG_IF_TSRESOL && len == 1) {
            if ((data[0] & 0x80) == 0) {
                ifd.time_units = Power10(data[0]);
            }
            else {
                ifd.time_units = 1LL << (data[0] & 0x7F);
            }
        }

        // Point to next option. Pad length to 4 bytes.
        data += round_up<uint16_t>(len, 4);
    }

    report.debug(u"pcap-ng interface#%d: link type: %d, time units/second: %'d, time offset: %'d microsec, FCS length: %d bytes",
                 {_if.size(), ifd.link_type, ifd.time_units, ifd.time_offset, ifd.fcs_size});

    // Add the interface description.
    _if.push_back(ifd);
    return true;
}


//----------------------------------------------------------------------------
// Read a pcap-ng block. The 32-bit block type has already been read.
//----------------------------------------------------------------------------

bool ts::PcapFile::readNgBlockBody(uint32_t block_type, ByteBlock& body, Report& report)
{
    body.clear();

    // Read the first "Block Total Length" field.
    uint8_t lenfield[4];
    if (!readall(lenfield, sizeof(lenfield), report)) {
        return error(report);
    }

    // If the block type is Section Header, then the endianness is given by the first 4 bytes.
    if (block_type == PCAPNG_SECTION_HEADER) {
        // Pcap-ng files have an endian-neutral block-type value for section header.
        // The byte order is defined by the 'byte-order magic' at the beginning of the section header block body.
        body.resize(4);
        if (!readall(body.data(), body.size(), report)) {
            body.clear();
            return error(report);
        }
        const uint32_t order_magic = GetUInt32BE(body.data());
        if (order_magic != PCAPNG_ORDER_BE && order_magic != PCAPNG_ORDER_LE) {
            body.clear();
            return error(report, u"invalid pcap-ng file, unknown 'byte-order magic' 0x%X in %s", {order_magic, _name});
        }
        _be = order_magic == PCAPNG_ORDER_BE;
    }

    // Interpret the packet size. The packet size include 12 additional bytes
    // for the block type and the two block length fields.
    const size_t size = get32(lenfield);
    if (size % 4 != 0 || size < 12 + body.size()) {
        body.clear();
        return error(report, u"invalid pcap-ng block length %d in %s", {size, _name});
    }

    // Read the rest of the block body.
    const size_t start = body.size();
    body.resize(size - 12);
    if (!readall(body.data() + start, body.size() - start, report)) {
        body.clear();
        return error(report);
    }

    // Read and check the last "Block Total Length" field.
    if (!readall(lenfield, sizeof(lenfield), report)) {
        return error(report);
    }
    const size_t last_size = get32(lenfield);
    if (size != last_size) {
        body.clear();
        return error(report, u"inconsistent pcap-ng block length in %s, leading length: %d, trailing length: %d", {_name, size, last_size});
    }
    return true;
}


//----------------------------------------------------------------------------
// Read the next IPv4 packet (headers included).
//----------------------------------------------------------------------------

bool ts::PcapFile::readIPv4(IPv4Packet& packet, MicroSecond& timestamp, Report& report)
{
    // Clear output values.
    packet.clear();
    timestamp = -1;

    // Check that the file is open.
    if (_in == nullptr) {
        report.error(u"no pcap file open");
        return false;
    }
    if (_error) {
        if (!_in->eof()) {
            report.debug(u"pcap file already in error state");
        }
        return false;
    }

    // Loop on file blocks until an IPv4 packet is found.
    for (;;) {

        // The captured packet will go there.
        ByteBlock buffer;
        size_t cap_start = 0;  // captured packet start index in buffer
        size_t cap_size = 0;   // captured packet size
        size_t orig_size = 0;  // original packet size (on network)
        size_t if_index = 0;   // interface index
        timestamp = -1;

        // We are at the beginning of a data block.
        if (_ng) {
            // Pcap-ng file, read block type value.
            uint8_t type_field[4];
            if (!readall(type_field, sizeof(type_field), report)) {
                return error(report);
            }
            const uint32_t type = get32(type_field);
            if (type == PCAPNG_SECTION_HEADER) {
                // Restart a new section, reinitialize all characteristics.
                if (!readHeader(type, report)) {
                    return error(report);
                }
                continue; // loop to next packet block
            }
            // Read one data block.
            if (!readNgBlockBody(type, buffer, report)) {
                return error(report);
            }
            if (type == PCAPNG_INTERFACE_DESC) {
                // Process an interface description.
                if (!analyzeNgInterface(buffer.data(), buffer.size(), report)) {
                    return error(report);
                }
                continue; // loop to next packet block
            }
            else if ((type == PCAPNG_ENHANCED_PACKET || type == PCAPNG_OBSOLETE_PACKET) && buffer.size() >= 20) {
                _packet_count++;
                cap_start = 20;
                cap_size = std::min<size_t>(get32(buffer.data() + 12), buffer.size() - 20);
                orig_size = get32(buffer.data() + 16);
                if_index = type == PCAPNG_OBSOLETE_PACKET ? get16(buffer.data()) : get32(buffer.data());
                if (if_index < _if.size() && _if[if_index].time_units != 0) {
                    const SubSecond units = _if[if_index].time_units;
                    const SubSecond tstamp = SubSecond(uint64_t(get32(buffer.data() + 4)) << 32) + SubSecond(get32(buffer.data() + 8));
                    // Take care to overflow in tstamp * MilliSecPerSec. Sometimes, the timestamp is a full time
                    // since 1970 with time unit being 1,000,000,000. The value is close to the 64-bit max.
                    if (units == MicroSecPerSec) {
                        timestamp = tstamp;
                    }
                    else if (units > MicroSecPerSec && units % MicroSecPerSec == 0) {
                        timestamp = tstamp / (units / MicroSecPerSec);
                    }
                    else if (units < MicroSecPerSec && MicroSecPerSec % units == 0) {
                        timestamp = tstamp * (MicroSecPerSec / units);
                    }
                    else if (mul_overflow(tstamp, MicroSecPerSec, tstamp * MicroSecPerSec)) {
                        timestamp = SubSecond((double(tstamp) * double(MicroSecPerSec)) / double(units));
                    }
                    else {
                        timestamp = (tstamp * MicroSecPerSec) / units;
                    }
                }
            }
            else if (type == PCAPNG_SIMPLE_PACKET && buffer.size() >= 4) {
                _packet_count++;
                cap_start = 4;
                orig_size = get32(buffer.data());
                cap_size = std::min(orig_size, buffer.size() - 4);
            }
            else {
                // This data block does not contain a captured packet, ignore it.
                continue;
            }
        }
        else {
            // Pcap file, beginning of a packet block. Read the 16-byte header.
            _packet_count++;
            uint8_t header[16];
            if (!readall(header, sizeof(header), report)) {
                return error(report);
            }
            const uint32_t tstamp = get32(header);
            const uint32_t sub_tstamp = get32(header + 4);
            cap_size = get32(header + 8);
            orig_size = get32(header + 12);

            // Compute time stamp. Time units is never null in pcap format.
            timestamp = (MicroSecond(tstamp) * MicroSecPerSec) + (SubSecond(sub_tstamp) * MicroSecPerSec) / _if[0].time_units;

            // Read packet data.
            buffer.resize(cap_size);
            if (!readall(buffer.data(), buffer.size(), report)) {
                return error(report);
            }
        }

        // Now process the captured packet.
        _packets_size += cap_size;
        if (orig_size > cap_size) {
            report.debug(u"truncated captured packet ignored (%d bytes, truncated to %d)", {orig_size, cap_size});
            continue; // loop to next packet block
        }

        // Get link type, adjust timestamp.
        InterfaceDesc ifd;
        if (if_index < _if.size()) {
            ifd = _if[if_index];
        }
        if (timestamp >= 0) {
            timestamp += ifd.time_offset;
            if (_first_timestamp < 0) {
                _first_timestamp = timestamp;
            }
            _last_timestamp = timestamp;
        }

        report.log(2, u"pcap data block: %d bytes, captured packet at offset %d, %d bytes (original: %d bytes), link type: %d",
                   {buffer.size(), cap_start, cap_size, orig_size, ifd.link_type});

        // Analyze the captured packet, trying to find an IPv4 datagram.
        if (ifd.link_type == LINKTYPE_NULL && cap_size > 4 && get32(buffer.data() + cap_start) == 2) {
            // BSD loopback encapsulation; the link layer header is a 4-byte field, in host byte order, containing 2 for IPv4 packets.
            cap_start += 4;
            cap_size -= 4;
        }
        else if (ifd.link_type == LINKTYPE_LOOP && cap_size > 4 && GetUInt32BE(buffer.data() + cap_start) == 2) {
            // OpenBSD loopback encapsulation; the link-layer header is a 4-byte field, in network byte order, containing 2 for IPv4 packets/
            cap_start += 4;
            cap_size -= 4;
        }
        else if ((ifd.link_type == LINKTYPE_ETHERNET || ifd.link_type == LINKTYPE_NULL || ifd.link_type == LINKTYPE_LOOP) &&
                 cap_size > ETHER_HEADER_SIZE + ifd.fcs_size && GetUInt16BE(buffer.data() + cap_start + ETHER_TYPE_OFFSET) == ETHERTYPE_IPv4)
        {
            // Ethernet frame: 14-byte header: destination MAC (6 bytes), source MAC (6 bytes), ether type (2 bytes, 0x0800 for IPv4).
            // This should apply to LINKTYPE_ETHERNET only. However, in some pcap files (not pcap-ng), it has been noticed that
            // LINKTYPE_NULL and LINKTYPE_LOOP can contain a raw Ethernet frame without the initial 4 bytes of encapsulation.
            cap_start += ETHER_HEADER_SIZE;
            cap_size -= ETHER_HEADER_SIZE + ifd.fcs_size;
        }
        else if (ifd.link_type == LINKTYPE_RAW && cap_size >= IPv4_MIN_HEADER_SIZE && (buffer[cap_start] >> 4) == 4) {
            // Raw IPv4 or IPv6 header (version in first byte), no encopsulation.
        }
        else {
            // Not an identified IPv4 packet.
            cap_size = 0;
        }

        // A possible IPv4 datagram was found.
        if (cap_size > 0) {
            if (packet.reset(buffer.data() + cap_start, cap_size)) {
                _ipv4_packet_count++;
                _ipv4_packets_size += cap_size;
                return true;
            }
            else {
                report.warning(u"invalid IPv4 datagram in pcap file, %d bytes (original: %d bytes), link type: %d", {cap_size, orig_size, ifd.link_type});
            }
        }
    }
}
