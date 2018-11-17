//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Basic definition of an MPEG-2 transport packet.
//
//----------------------------------------------------------------------------

#include "tsTSPacket.h"
#include "tsPCR.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// This constant is a null (or stuffing) packet.
//----------------------------------------------------------------------------

const ts::TSPacket ts::NullPacket = {{
    // Header: PID 0x1FFF
    0x47, 0x1F, 0xFF, 0x10,
    // Payload: 184 bytes 0xFF
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF
}};

//----------------------------------------------------------------------------
// This constant is an empty packet (no payload)
//----------------------------------------------------------------------------

const ts::TSPacket ts::EmptyPacket = {{
    // Header: PID 0x1FFF, has adaptation field, no payload, CC = 0
    0x47, 0x1F, 0xFF, 0x20,
    // Adaptation field length
    183,
    // Flags: none
    0x00,
    // Adaptation field stuffing: 182 bytes 0xFF
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF
}};

//----------------------------------------------------------------------------
// Sanity check routine. Ensure that the TSPacket structure can
// be used in contiguous memory array and array of packets.
// Can be used once at startup time in paranoid applications.
//----------------------------------------------------------------------------

void ts::TSPacket::SanityCheck()
{
    assert(sizeof(TSPacket) == PKT_SIZE);
    TSPacket p;
    assert(reinterpret_cast<uint8_t*>(&p) == p.b);
    TSPacket pa[2];
    assert(reinterpret_cast<char*>(&(pa[1])) == reinterpret_cast<char*>(&(pa[0])) + PKT_SIZE);
    TSPacketVector pv(2);
    assert(reinterpret_cast<char*>(&(pv[1])) == reinterpret_cast<char*>(&(pv[0])) + PKT_SIZE);
}

//----------------------------------------------------------------------------
// Check if the packet contains the start of a clear PES header.
//----------------------------------------------------------------------------

bool ts::TSPacket::startPES() const
{
    const uint8_t* const pl = getPayload();
    return hasValidSync() && getPUSI() && isClear() && hasPayload() &&
        getPayloadSize() >= 3 && pl[0] == 0x00 && pl[1] == 0x00 && pl[2] == 0x01;
}

//----------------------------------------------------------------------------
// These private methods compute the offset of PCR, OPCR.
// Return 0 if there is none.
//----------------------------------------------------------------------------

size_t ts::TSPacket::PCROffset () const
{
    return hasPCR() && b[4] >= 7 ? 6 : 0;
}

size_t ts::TSPacket::OPCROffset () const
{
    if (!hasOPCR()) {
        return 0;
    }
    else if (hasPCR()) {
        return b[4] >= 13 ? 12 : 0;
    }
    else {
        return b[4] >= 7 ? 6 : 0;
    }
}

//----------------------------------------------------------------------------
// Compute offset of splice_countdown
//----------------------------------------------------------------------------

size_t ts::TSPacket::spliceCountdownOffset() const
{
    if (!hasSpliceCountdown()) {
        return 0;
    }
    else if (hasPCR() && hasOPCR()) {
        return b[4] >= 14 ? 18 : 0;
    }
    else if (hasPCR() || hasOPCR()) {
        return b[4] >= 8 ? 12 : 0;
    }
    else {
        return b[4] >= 2 ? 6 : 0;
    }
}

//----------------------------------------------------------------------------
// Get PCR or OPCR - 42 bits
// Return 0 if not found.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::getPCR () const
{
    const size_t offset = PCROffset ();
    return offset == 0 ? 0 : GetPCR (b + offset);
}

uint64_t ts::TSPacket::getOPCR () const
{
    const size_t offset = OPCROffset ();
    return offset == 0 ? 0 : GetPCR (b + offset);
}

int8_t ts::TSPacket::getSpliceCountdown() const
{
    const size_t offset = spliceCountdownOffset();
    return offset == 0 ? 0 : *(b + offset);
}

//----------------------------------------------------------------------------
// Replace PCR or OPCR - 42 bits
// Throw exception PCRError if not present.
//----------------------------------------------------------------------------

void ts::TSPacket::setPCR(const uint64_t& pcr)
{
    size_t offset(PCROffset());
    if (offset == 0) {
        throw AdaptationFieldError(u"No PCR in packet, cannot replace");
    }
    else {
        PutPCR(b + offset, pcr);
    }
}

void ts::TSPacket::setOPCR(const uint64_t& opcr)
{
    size_t offset(OPCROffset());
    if (offset == 0) {
        throw AdaptationFieldError(u"No OPCR in packet, cannot replace");
    }
    else {
        PutPCR(b + offset, opcr);
    }
}

//----------------------------------------------------------------------------
// These private methods compute the offset of PTS, DTS.
// Return 0 if there is none.
//----------------------------------------------------------------------------

size_t ts::TSPacket::PTSOffset() const
{
    if (!startPES()) {
        return 0;
    }
    const size_t pl_size = getPayloadSize();
    const uint8_t* const pl = getPayload();
    if (pl_size < 14 || !IsLongHeaderSID(pl[3])) {
        return 0;
    }
    const uint8_t pts_dts_flags = pl[7] >> 6;
    if ((pts_dts_flags & 0x02) == 0 ||
        (pts_dts_flags == 0x02 && (pl[9] & 0xF1) != 0x21) ||
        (pts_dts_flags == 0x03 && (pl[9] & 0xF1) != 0x31) ||
        (pl[11] & 0x01) != 0x01 ||
        (pl[13] & 0x01) != 0x01) {
        return 0;
    }
    return pl + 9 - b;
}

size_t ts::TSPacket::DTSOffset () const
{
    if (!startPES()) {
        return 0;
    }
    const size_t pl_size = getPayloadSize();
    const uint8_t* const pl = getPayload();
    if (pl_size < 19 ||
        (pl[7] & 0xC0) != 0xC0 ||
        (pl[9] & 0xF1) != 0x31 ||
        (pl[11] & 0x01) != 0x01 ||
        (pl[13] & 0x01) != 0x01 ||
        (pl[14] & 0xF1) != 0x11 ||
        (pl[16] & 0x01) != 0x01 ||
        (pl[18] & 0x01) != 0x01) {
        return 0;
    }
    return pl + 14 - b;
}

//----------------------------------------------------------------------------
// Get PTS or DTS at specified offset. Return 0 if offset is zero.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::getPDTS (size_t offset) const
{
    if (offset == 0) {
        return 0;
    }
    else {
        return (uint64_t(b[offset] & 0x0E) << 29) |
            (uint64_t(GetUInt16(b + offset + 1) & 0xFFFE) << 14) |
            (uint64_t(GetUInt16(b + offset + 3)) >> 1);
    }
}

//----------------------------------------------------------------------------
// Set PTS or DTS at specified offset.
//----------------------------------------------------------------------------

void ts::TSPacket::setPDTS(uint64_t pdts, size_t offset)
{
    if (offset != 0) {
        b[offset] = (b[offset] & 0xF1) | (uint8_t(pdts >> 29) & 0x0E);
        PutUInt16(b + offset + 1, (GetUInt16(b + offset + 1) & 0x0001) | (uint16_t(pdts >> 14) & 0xFFFE));
        PutUInt16(b + offset + 3, (GetUInt16(b + offset + 3) & 0x0001) | (uint16_t(pdts << 1) & 0xFFFE));
    }
}


//----------------------------------------------------------------------------
// Error message fragment indicating the number of packets previously
// read in a binary file
//----------------------------------------------------------------------------

namespace {
    ts::UString AfterPackets(const std::streampos& position)
    {
        const int64_t packets = int64_t(std::streamoff(position)) / ts::PKT_SIZE;
        if (packets > 0 && position != std::streampos(-1)) {
            return ts::UString::Format(u" after %'d TS packets", {packets});
        }
        else {
            return ts::UString();
        }
    }
}

//----------------------------------------------------------------------------
// Read packet on standard streams.
// Return true on success, false on error.
// On input, if check_sync is true, the sync byte of the
// input packet is checked. If it is not valid, set the
// failbit of the stream and return false.
//----------------------------------------------------------------------------

std::istream& ts::TSPacket::read(std::istream& strm, bool check_sync, Report& report)
{
    if (!strm) {
        return strm;
    }

    std::streampos position(strm.tellg());
    strm.read(reinterpret_cast<char*>(b), PKT_SIZE);
    size_t insize = size_t(strm.gcount());

    if (insize == PKT_SIZE) {
        // Got a complete TS packet
        if (check_sync && b[0] != SYNC_BYTE) {
            // Complete packet read but wrong sync byte.
            // Flawfinder: ignore: completely fooled here, std::ostream::setstate has nothing to do with PRNG.
            strm.setstate(std::ios::failbit);
            report.error(u"synchronization lost%s, got 0x%X instead of 0x%X at start of TS packet", {AfterPackets(position), b[0], SYNC_BYTE});
        }
    }
    else if (!strm.eof()) {
        // Not an EOF, actual I/O error
        report.error(u"I/O error while reading TS packet" + AfterPackets (position));
    }
    else if (insize > 0) {
        // EOF, got partial packet.
        // Flawfinder: ignore: completely fooled here, std::ostream::setstate has nothing to do with PRNG.
        strm.setstate(std::ios::failbit);
        report.error(u"truncated TS packet (%d bytes)%s", {insize, AfterPackets(position)});
    }

    return strm;
}

//----------------------------------------------------------------------------
// Write packet on standard streams.
// Return true on success, false on error.
//----------------------------------------------------------------------------

std::ostream& ts::TSPacket::write(std::ostream& strm, Report& report) const
{
    if (strm) {
        strm.write(reinterpret_cast <const char*> (b), PKT_SIZE);
        if (!strm) {
            report.error(u"error writing TS packet into binary stream");
        }
    }
    return strm;
}

//----------------------------------------------------------------------------
// This method displays the content of a transport packet.
// The flags indicate which part must be dumped. If DUMP_RAW or
// DUMP_PAYLOAD is specified, flags from Hexa::Flags
// may also be used. Indent indicates the base indentation of lines.
//----------------------------------------------------------------------------

std::ostream& ts::TSPacket::display(std::ostream& strm, uint32_t flags, size_t indent, size_t max_size) const
{
    const std::string margin(indent, ' ');

    // The 16 MSB contains flags specific to ts_dump_packet.
    // The 16 LSB contains flags for ts_hexa_dump.
    // Supply default dump option:

    if ((flags & 0xFFFF0000) == 0) {
        flags |= DUMP_RAW;
    }

    // Filter invalid packets

    if (!hasValidSync ()) {
        strm << margin << "**** INVALID PACKET ****" << std::endl;
        flags = (flags & 0x0000FFFF) | DUMP_RAW;
    }

    // Display full packet or payload only.

    const size_t header_size = getHeaderSize();
    const size_t payload_size = getPayloadSize();
    const uint8_t* const display_data = (flags & DUMP_PAYLOAD) ? b + header_size : b;
    const size_t display_size = std::min((flags & DUMP_PAYLOAD) ? payload_size : PKT_SIZE, max_size);

    // Handle single line mode.

    if (flags & UString::SINGLE_LINE) {
        strm << margin;
        if (flags & DUMP_TS_HEADER) {
            strm << UString::Format(u"PID: 0x%X, PUSI: %d, ", {getPID(), getPUSI()});
        }
        strm << UString::Dump(display_data, display_size, flags & 0x0000FFFF) << std::endl;
        return strm;
    }

    // A PES header starts with the 3-byte prefix 0x000001. A packet has a PES
    // header if the 'payload unit start' is set in the TS header and the
    // payload starts with 0x000001.
    //
    // Note that there is no risk to misinterpret the prefix: When 'payload
    // unit start' is set, the payload may also contains PSI/SI tables. In
    // that case, 0x000001 is not a possible value for the beginning of the
    // payload. With PSI/SI, a payload starting with 0x000001 would mean:
    //  0x00 : pointer field -> a section starts at next byte
    //  0x00 : table id -> a PAT
    //  0x01 : section_syntax_indicator field is 0, impossible for a PAT

    bool has_pes_header = hasValidSync() &&
                          getPUSI() &&
                          payload_size >= 3 &&
                          b[header_size] == 0x00 &&
                          b[header_size + 1] == 0x00 &&
                          b[header_size + 2] == 0x01;

    // Display TS header

    if (flags & DUMP_TS_HEADER) {
        strm << margin << "---- TS Header ----" << std::endl
             << margin << UString::Format(u"PID: %d (0x%X), header size: %d, sync: 0x%X", {getPID(), getPID(), header_size, b[0]}) << std::endl
             << margin << "Error: " << getTEI() << ", unit start: " << getPUSI() << ", priority: " << getPriority() << std::endl
             << margin << "Scrambling: " << int (getScrambling()) << ", continuity counter: " << int (getCC()) << std::endl
             << margin << "Adaptation field: " << UString::YesNo (hasAF()) << " (" << getAFSize () << " bytes)"
             << ", payload: " << UString::YesNo (hasPayload()) << " (" << getPayloadSize() << " bytes)" << std::endl;
        if (hasAF()) {
            strm << margin << "Discontinuity: " << getDiscontinuityIndicator()
                 << ", random access: " << getRandomAccessIndicator()
                 << ", ES priority: " << getESPI() << std::endl;
        }
        if (hasPCR() || hasOPCR()) {
            strm << margin;
            if (hasPCR()) {
                strm << UString::Format(u"PCR: 0x%011X", {getPCR()});
                if (hasOPCR()) {
                    strm << ", ";
                }
            }
            if (hasOPCR()) {
                strm << UString::Format(u"OPCR: 0x%011X", {getOPCR()});
            }
            strm << std::endl;
        }
    }

    // Display PES header

    if (has_pes_header && (flags & DUMP_PES_HEADER)) {
        uint8_t sid = b[header_size + 3];
        uint16_t length = GetUInt16(b + header_size + 4);
        strm << margin << "---- PES Header ----" << std::endl
             << margin << "Stream id: " << names::StreamId(sid, names::FIRST) << std::endl
             << margin << "PES packet length: " << length;
        if (length == 0) {
            strm << " (unbounded)";
        }
        strm << std::endl;
    }

    // Display full packet or payload in hexa

    if (flags & (DUMP_RAW | DUMP_PAYLOAD)) {
        if (flags & DUMP_RAW) {
            strm << margin << "---- Full TS Packet Content ----" << std::endl;
        }
        else {
            strm << margin << "---- TS Packet Payload (" << payload_size << " bytes) ----" << std::endl;
        }
        // The 16 LSB contains flags for Hexa.
        strm << UString::Dump(display_data, display_size, flags & 0x0000FFFF, indent);
    }

    return strm;
}
