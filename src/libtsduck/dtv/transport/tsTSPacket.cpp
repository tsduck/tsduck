//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPacket.h"
#include "tsPES.h"
#include "tsNamesFile.h"
#include "tsByteBlock.h"
#include "tsBuffer.h"
#include "tsNamesFile.h"


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
#if !defined(NDEBUG)
    assert(sizeof(TSPacket) == PKT_SIZE);
    TSPacket p;
    assert(reinterpret_cast<uint8_t*>(&p) == p.b);
    TSPacket pa[2];
    assert(reinterpret_cast<char*>(&(pa[1])) == reinterpret_cast<char*>(&(pa[0])) + PKT_SIZE);
    TSPacketVector pv(2);
    assert(reinterpret_cast<char*>(&(pv[1])) == reinterpret_cast<char*>(&(pv[0])) + PKT_SIZE);
#endif
}

//----------------------------------------------------------------------------
// TS packet copy functions.
//----------------------------------------------------------------------------

// Init this packet from a memory area.
void  ts::TSPacket::copyFrom(const void* source)
{
    assert(source != nullptr);
    std::memcpy(b, source, PKT_SIZE);
}

// Copy this packet content to a memory area.
void ts::TSPacket::copyTo(void* dest) const
{
    assert(dest != nullptr);
    std::memcpy(dest, b, PKT_SIZE);
}

// Static method to copy contiguous TS packets.
void ts::TSPacket::Copy(TSPacket* dest, const TSPacket* source, size_t count)
{
    assert(dest != nullptr);
    assert(source != nullptr);
    std::memcpy(dest->b, source->b, count * PKT_SIZE);
}

// Static method to copy contiguous TS packets from raw memory.
void ts::TSPacket::Copy(TSPacket* dest, const uint8_t* source, size_t count)
{
    assert(dest != nullptr);
    assert(source != nullptr);
    std::memcpy(dest->b, source, count * PKT_SIZE);
}

// Static method to copy contiguous TS packets into raw memory.
void ts::TSPacket::Copy(uint8_t* dest, const TSPacket* source, size_t count)
{
    assert(dest != nullptr);
    assert(source != nullptr);
    std::memcpy(dest, source->b, count * PKT_SIZE);
}

//----------------------------------------------------------------------------
// Initialize a TS packet.
//----------------------------------------------------------------------------

void ts::TSPacket::init(PID pid, uint8_t cc, uint8_t data)
{
    b[0] = 0x47;
    b[1] = uint8_t(pid >> 8) & 0x1F;
    b[2] = uint8_t(pid);
    b[3] = 0x10 | (cc & 0x0F); // no adaptation field, payload only.
    std::memset(b + 4, data, PKT_SIZE - 4);
}

//----------------------------------------------------------------------------
// Compute the size of the stuffing part in the adaptation_field.
//----------------------------------------------------------------------------

size_t ts::TSPacket::getAFStuffingSize() const
{
    if ((b[3] & 0x20) == 0 || b[4] == 0) {
        // No or empty adaptation field.
        return 0;
    }

    // Compute all present bytes in the adaptation fields.
    const uint8_t flags = b[5];
    size_t size = 1;              // Flags
    const uint8_t *data = b + 6;  // Point to first AF byte after flags

    if ((flags & 0x10) != 0) {
        // PCR present
        size += 6;
        data += 6;
    }
    if ((flags & 0x08) != 0) {
        // OPCR present
        size += 6;
        data += 6;
    }
    if ((flags & 0x04) != 0) {
        // Splicing point countdown present
        size += 1;
        data += 1;
    }
    if ((flags & 0x02) != 0) {
        // Transport private data present
        size += 1 + *data;
        data += 1 + *data;
    }
    if ((flags & 0x01) != 0 && data < b + PKT_SIZE) {
        // Adaptation field extension present
        size += 1 + *data;
        data += 1 + *data;
    }

    // Now return the stuffing size (make sur it is consistent with AF size).
    return size > b[4] ? 0 : b[4] - size;
}

//----------------------------------------------------------------------------
// Set the payload size.
//----------------------------------------------------------------------------

bool ts::TSPacket::setPayloadSize(size_t size, bool shift_payload, uint8_t pad)
{
    // Previous payload size.
    size_t plSize = getPayloadSize();

    if (size == plSize) {
        return true; // no change
    }
    else if (size < plSize) {
        // It is always possible to shrink the payload.
        if (shift_payload) {
            // Move the payload forward.
            std::memmove(b + PKT_SIZE - size, b + PKT_SIZE - plSize, size);
        }
        if ((b[3] & 0x20) == 0) {
            // No previous adaptation field, create one.
            b[3] |= 0x20; // AF presence flag.
            b[4] = 0;     // AF size
            // We just created a 1-byte adaptation field.
            if (--plSize == size) {
                // If that 1-byte AF is sufficient to reach the target payload size, we are done.
                return true;
            }
        }
        // If the adaptation field exists but is empty, create the flags field.
        if (b[4] == 0) {
            b[4] = 1; // new AF size
            b[5] = 0; // flags
            --plSize; // payload has shrunk by one byte
        }
        // Fill the stuffing extension with pad byte.
        std::memset(b + 5 + b[4], pad, plSize - size);
        // Adjust AF size.
        b[4] += uint8_t(plSize - size);
        return true;
    }
    else if (plSize + getAFStuffingSize() < size) {
        // Payload shall be extended but cannot reach the requested size, even with all current AF stuffing.
        return false;
    }
    else {
        // Payload can be be extended by removing some stuffing from adaptation field.
        const size_t add = size - plSize;
        if (shift_payload) {
            // Move the payload backward.
            std::memmove(b + PKT_SIZE - size, b + PKT_SIZE - plSize, plSize);
            // Fill the top part of the payload with the pad byte.
            std::memset(b + PKT_SIZE - add , pad, add);
        }
        b[4] -= uint8_t(add);
        return true;
    }
}

//----------------------------------------------------------------------------
// Check if the packet contains the start of a clear PES header.
//----------------------------------------------------------------------------

bool ts::TSPacket::startPES() const
{
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

    const uint8_t* const pl = getPayload();
    return hasValidSync() && !getTEI() && getPUSI() && isClear() && hasPayload() &&
           getPayloadSize() >= 3 && pl[0] == 0x00 && pl[1] == 0x00 && pl[2] == 0x01;
}


//----------------------------------------------------------------------------
// Get the address and size of the stuffing area of the PES header in the TS packet.
//----------------------------------------------------------------------------

bool ts::TSPacket::getPESHeaderStuffingArea(uint8_t*& addr, size_t& pes_size, size_t& ts_size)
{
    const uint8_t* c_addr = nullptr;
    const bool status = getPESHeaderStuffingArea(c_addr, pes_size, ts_size);
    addr = const_cast<uint8_t*>(c_addr);
    return status;
}

bool ts::TSPacket::getPESHeaderStuffingArea(const uint8_t*& addr, size_t& pes_size, size_t& ts_size) const
{
    // Reset output values when not found.
    addr = nullptr;
    pes_size = ts_size = 0;

    // TS packet payload:
    const uint8_t* const pl = getPayload();
    const size_t pl_size = getPayloadSize();

    // If there is no clear PES header inside the packet, nothing to do.
    if (!startPES() || pl_size < 9 || !IsLongHeaderSID(pl[3])) {
        // Can't get the start of a long header or this stream id does not have a long header.
        return false;
    }

    // Size of the PES header, may include stuffing.
    const size_t header_size = 9 + size_t(pl[8]);

    // Look for the offset of the stuffing in the PES packet.
    size_t offset = 9;
    const uint8_t PTS_DTS_flags = (pl[7] >> 6) & 0x03;
    if (offset < header_size && PTS_DTS_flags == 2) {
        offset += 5;  // skip PTS
    }
    if (offset < header_size && PTS_DTS_flags == 3) {
        offset += 10;  // skip PTS and DTS
    }
    if (offset < header_size && (pl[7] & 0x20) != 0) {
        offset += 6;  // ESCR_flag set, skip ESCR
    }
    if (offset < header_size && (pl[7] & 0x10) != 0) {
        offset += 3;  // ES_rate_flag set, skip ES_rate
    }
    if (offset < header_size && (pl[7] & 0x08) != 0) {
        offset += 1;  // DSM_trick_mode_flag set, skip trick mode
    }
    if (offset < header_size && (pl[7] & 0x04) != 0) {
        offset += 1;  // additional_copy_info_flag set, skip additional_copy_info
    }
    if (offset < header_size && (pl[7] & 0x02) != 0) {
        offset += 2;  // PES_CRC_flag set, skip previous_PES_packet_CRC
    }
    if (offset < header_size && offset < pl_size && (pl[7] & 0x01) != 0) {
        // PES_extension_flag set, analyze and skip PES extensions
        // First, get the flags indicating which extensions are present.
        const uint8_t flags = pl[offset++];
        if (offset < header_size && (flags & 0x80) != 0) {
            offset += 16; // PES_private_data_flag set
        }
        if (offset < header_size && offset < pl_size && (flags & 0x40) != 0) {
            offset += 1 + pl[offset]; // pack_header_field_flag set
        }
        if (offset < header_size && (flags & 0x20) != 0) {
            offset += 2; // program_packet_sequence_counter_flag set
        }
        if (offset < header_size && (flags & 0x10) != 0) {
            offset += 2; // P-STD_buffer_flag set
        }
        if (offset < header_size && offset < pl_size && (flags & 0x01) != 0) {
            offset += 1 + (pl[offset] & 0x7F); //  PES_extension_flag_2 set
        }
    }

    // Now, offset points to the beginning of the stuffing area in the PES header.
    if (offset < header_size && offset <= pl_size) {
        // The stuffing area exists and is not empty.
        // The part which is in the current TS packet can be smaller or even empty.
        addr = pl + offset;
        pes_size = header_size - offset;
        ts_size = std::min(header_size, pl_size) - offset;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the size of the PES header in the packet, if one is present.
//----------------------------------------------------------------------------

size_t ts::TSPacket::getPESHeaderSize() const
{
    const uint8_t* const pl = getPayload();
    const size_t plSize = getPayloadSize();

    if (!startPES() || plSize < 4) {
        // No start PES or PES header too short to get the stream type.
        return 0;
    }
    else if (!IsLongHeaderSID(pl[3])) {
        // Short fixed-size PES header for that stream type.
        return 6;
    }
    else if (plSize < 9) {
        // Long PES header, but not long enough to get actual size.
        return 0;
    }
    else {
        // Long header.
        return 9 + size_t(pl[8]);
    }
}

//----------------------------------------------------------------------------
// Static routines to extract / insert a PCR from / to a stream.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::GetPCR(const uint8_t* b)
{
    const uint32_t v32 = GetUInt32(b);
    const uint16_t v16 = GetUInt16(b + 4);
    const uint64_t pcr_base = (uint64_t(v32) << 1) | uint64_t(v16 >> 15);
    const uint64_t pcr_ext = uint64_t(v16 & 0x01FF);
    return pcr_base * SYSTEM_CLOCK_SUBFACTOR + pcr_ext;
}

void ts::TSPacket::PutPCR(uint8_t* b, const uint64_t& pcr)
{
    const uint64_t pcr_base = pcr / SYSTEM_CLOCK_SUBFACTOR;
    const uint64_t pcr_ext = pcr % SYSTEM_CLOCK_SUBFACTOR;
    PutUInt32(b, uint32_t(pcr_base >> 1));
    PutUInt16(b + 4, uint16_t(uint32_t((pcr_base << 15) | 0x7E00 | pcr_ext)));
}

//----------------------------------------------------------------------------
// Private methods to compute the offset of PCR or OPCR.
//----------------------------------------------------------------------------

size_t ts::TSPacket::PCROffset() const
{
    return hasPCR() && b[4] >= 7 ? 6 : 0;
}

size_t ts::TSPacket::OPCROffset() const
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
// Check presence and compute offset of private data in AF.
//----------------------------------------------------------------------------

size_t ts::TSPacket::privateDataOffset() const
{
    const size_t af = getAFSize();
    if (af < 2 || (b[5] & 0x02) == 0) {
        // No AF or no private data in it.
        return 0;
    }

    // Compute offset of private data.
    const size_t offset = 6 +                      // start of AF, after flags
        (((b[5] & 0x10) != 0) ? PCR_BYTES : 0) +   // skip PCR
        (((b[5] & 0x08) != 0) ? PCR_BYTES : 0) +   // skip OPCR
        (((b[5] & 0x04) != 0) ? 1 : 0);            // skip splicing countdown

    // Check that private data fit inside the AF.
    const size_t endAF = 4 + af;
    return offset < endAF && offset + 1 + b[offset] <= endAF ? offset : 0;
}

//----------------------------------------------------------------------------
// Set or delete private data in adaptation field.
//----------------------------------------------------------------------------

void ts::TSPacket::removePrivateData()
{
    const size_t offset = privateDataOffset();
    deleteFieldFromAF(offset, offset > 0 ? 1 + b[offset] : 0, 0x02);
}

size_t ts::TSPacket::getPrivateDataSize() const
{
    const size_t offset = privateDataOffset();
    return offset == 0 ? 0 : size_t(b[offset]);
}

const uint8_t* ts::TSPacket::getPrivateData() const
{
    const size_t offset = privateDataOffset();
    return offset == 0 ? nullptr : b + offset + 1;
}

uint8_t* ts::TSPacket::getPrivateData()
{
    const size_t offset = privateDataOffset();
    return offset == 0 ? nullptr : b + offset + 1;
}

void ts::TSPacket::getPrivateData(ByteBlock& data) const
{
    const size_t offset = privateDataOffset();
    if (offset == 0) {
        data.clear(); // No AF
    }
    else {
        data.copy(b + offset + 1, b[offset]);
    }
}

bool ts::TSPacket::setPrivateData(const ByteBlock& data, bool shift_payload)
{
    return setPrivateData(data.data(), data.size(), shift_payload);
}

bool ts::TSPacket::setPrivateData(const void* data, size_t size, bool shift_payload)
{
    // Filter incorrect data.
    if (data == nullptr || size > PKT_SIZE - 7) {
        // Min overhead outside private data: 4-byte packet header, 2-byte AF header, 1-byte data size = 7 bytes
        return false;
    }

    // Make sure an AF is created.
    if (!reserveStuffing(0, shift_payload, true)) {
        return false;
    }
    assert(hasAF());

    // Compute offset of private data.
    const size_t offset = 6 +                      // start of AF, after flags
        (((b[5] & 0x10) != 0) ? PCR_BYTES : 0) +   // skip PCR
        (((b[5] & 0x08) != 0) ? PCR_BYTES : 0) +   // skip OPCR
        (((b[5] & 0x04) != 0) ? 1 : 0);            // skip splicing countdown

    // Do we have valid private data already?
    const bool hasData = (b[5] & 0x02) != 0;
    size_t endAF = 5 + b[4];
    if (hasData && offset + 1 + b[offset] > endAF) {
        // Invalid previous private data, they extend beyond end of AF => invalid packet.
        return false;
    }

    // Make room for the new private data.
    const size_t endNewData = offset + 1 + size;
    if (!hasData) {
        // No previous private data, reserve space for size and data.
        if (!reserveStuffing(1 + size, shift_payload)) {
            return false;
        }
        // Shift rest of AF upward.
        endAF = 5 + b[4];
        std::memmove(b + endNewData, b + offset, endAF - endNewData);
    }
    else {
        const size_t endPreviousData = offset + 1 + b[offset];
        if (endNewData < endPreviousData) {
            // New private data are shorter.
            // Move rest of AF downward.
            endAF = 5 + b[4];
            const size_t remove = endPreviousData - endNewData;
            std::memmove(b + endNewData, b + endPreviousData, endAF - endPreviousData);
            // Erase freeed space (now stuffing).
            std::memset(b + endAF - remove, 0xFF, remove);
        }
        else if (endNewData > endPreviousData) {
            // New private data are larger.
            const size_t add = endNewData - endPreviousData;
            if (!reserveStuffing(add, shift_payload)) {
                return false; // cannot enlarge AF.
            }
            // Move rest of AF upward.
            endAF = 5 + b[4];
            std::memmove(b + endNewData, b + endPreviousData, endAF - endNewData);
        }
    }

    // Finally write private data.
    b[5] |= 0x02;
    b[offset] = uint8_t(size);
    std::memcpy(b + 1 + offset, data, size);
    return true;
}

//----------------------------------------------------------------------------
// Get PCR or OPCR - 42 bits
// Return 0 if not found.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::getPCR() const
{
    const size_t offset = PCROffset();
    return offset == 0 ? INVALID_PCR : GetPCR(b + offset);
}

uint64_t ts::TSPacket::getOPCR() const
{
    const size_t offset = OPCROffset();
    return offset == 0 ? INVALID_PCR : GetPCR(b + offset);
}

int8_t ts::TSPacket::getSpliceCountdown() const
{
    const size_t offset = spliceCountdownOffset();
    return offset == 0 ? 0 : int8_t(b[offset]);
}

//----------------------------------------------------------------------------
// Remove the PCR or OPCR.
//----------------------------------------------------------------------------

void ts::TSPacket::deleteFieldFromAF(size_t offset, size_t size, uint32_t flag)
{
    if (offset > 0) {
        // A field is present at the given offset.
        const size_t afSize = getAFSize();
        assert(4 + afSize >= offset + size); // assert end of AF >= end of field
        // Clear the field presence flag:
        b[5] &= ~flag;
        // Shift the adaptation field down. With memmove(), the memory regions may overlap.
        std::memmove(b + offset, b + offset + size, 4 + afSize - offset - size);
        // Overwrite the last part of the AF, becoming AF stuffing.
        std::memset(b + 4 + afSize - size, 0xFF, size);
    }
}

//----------------------------------------------------------------------------
// Create or replace the splicing point countdown - 8 bits.
//----------------------------------------------------------------------------

bool ts::TSPacket::setSpliceCountdown(int8_t count, bool shift_payload)
{
    size_t offset = spliceCountdownOffset();
    if (offset == 0) {
        // Currently no splicing point countdown is present, we need to create one.
        if (!reserveStuffing(1, shift_payload, false)) {
            // Could not create space for splicing point countdown.
            return false;
        }
        // Set splicing point countdown flag.
        b[5] |= 0x04;
        // Compute splicing point countdown offset.
        offset = 6 + (hasPCR() ? PCR_BYTES : 0) + (hasOPCR() ? PCR_BYTES : 0);
        // Shift the existing AF 1 byte ahead to make room for the splicing point countdown value.
        std::memmove(b + offset + 1, b + offset, 4 + getAFSize() - offset - 1);
    }

    // Finally write the splicing point countdown value.
    b[offset] = uint8_t(count);
    return true;
}

//----------------------------------------------------------------------------
// Set fields in the adaptation field.
//----------------------------------------------------------------------------

bool ts::TSPacket::setFlagsInAF(uint8_t flags, bool shift_payload)
{
    if (reserveStuffing(0, shift_payload, true)) {
        b[5] |= flags;
        return true;
    }
    else {
        return false;
    }
}

//----------------------------------------------------------------------------
// Enlarge adaptation field, make sure that there is at least 'size'
//----------------------------------------------------------------------------

bool ts::TSPacket::reserveStuffing(size_t size, bool shift_payload, bool enforce_af)
{
    const size_t af = getAFSize();
    const size_t stuff = getAFStuffingSize();
    const size_t payload = getPayloadSize();

    // How many bytes shall we add into the AF?
    size_t moreAF = size <= stuff ? 0 : size - stuff;
    if (moreAF > 0 || enforce_af) {
        // Make sure that the AF exists, with at least the flags field.
        if (af == 0) {
            moreAF += 2;  // no AF exists yet, need AF size byte and flags bytes
        }
        else if (af == 1) {
            moreAF += 1;  // no flags byte, need to create it
        }
    }

    if (moreAF == 0) {
        // Nothing to add, everything is there.
        return true;
    }
    else if (!shift_payload || moreAF > payload) {
        // We are not allowed to shift the payload and/or there is not enough room in the packet.
        return false;
    }
    else {
        // Shrink payload, enlarge or create AF.
        setPayloadSize(payload - moreAF, true);
        return true;
    }
}

//----------------------------------------------------------------------------
// Create or replace the PCR or OPCR value - 42 bits.
//----------------------------------------------------------------------------

bool ts::TSPacket::setPCR(const uint64_t &pcr, bool shift_payload)
{
    if (pcr == INVALID_PCR) {
        return false;
    }

    size_t offset = PCROffset();
    if (offset == 0) {
        // Currently no PCR is present, we need to create one.
        if (!reserveStuffing(PCR_BYTES, shift_payload, false)) {
            // Could not create space for PCR.
            return false;
        }
        // We will create a PCR:
        b[5] |= 0x10;  // set PCR flag
        offset = 6;    // PCR offset in packet
        // Shift the existing AF 6 bytes ahead to make room for the PCR value.
        std::memmove(b + offset + PCR_BYTES, b + offset, 4 + getAFSize() - offset - PCR_BYTES);
    }

    // Finally write the PCR value.
    PutPCR(b + offset, pcr);
    return true;
}

bool ts::TSPacket::setOPCR(const uint64_t &opcr, bool shift_payload)
{
    if (opcr == INVALID_PCR) {
        return false;
    }

    size_t offset = OPCROffset();
    if (offset == 0) {
        // Currently no OPCR is present, we need to create one.
        if (!reserveStuffing(PCR_BYTES, shift_payload, false)) {
            // Could not create space for PCR.
            return false;
        }
        // Set OPCR flag.
        b[5] |= 0x08;
        // Compute OPCR offset.
        offset = 6 + (hasPCR() ? PCR_BYTES : 0);
        // Shift the existing AF 6 bytes ahead to make room for the PCR value.
        std::memmove(b + offset + PCR_BYTES, b + offset, 4 + getAFSize() - offset - PCR_BYTES);
    }

    // Finally write the OPCR value.
    PutPCR(b + offset, opcr);
    return true;
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

size_t ts::TSPacket::DTSOffset() const
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
// Get PTS or DTS at specified offset.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::getPDTS(size_t offset) const
{
    if (offset == 0) {
        return INVALID_PTS; // same as INVALID_DTS
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
    if (offset != 0 && pdts != INVALID_PTS) {
        b[offset] = (b[offset] & 0xF1) | (uint8_t(pdts >> 29) & 0x0E);
        PutUInt16(b + offset + 1, (GetUInt16(b + offset + 1) & 0x0001) | (uint16_t(pdts >> 14) & 0xFFFE));
        PutUInt16(b + offset + 3, (GetUInt16(b + offset + 3) & 0x0001) | (uint16_t(pdts << 1) & 0xFFFE));
    }
}


//----------------------------------------------------------------------------
// Check if this packet has the same payload as another one.
//----------------------------------------------------------------------------

bool ts::TSPacket::samePayload(const TSPacket& other) const
{
    if (!hasPayload() || !other.hasPayload()) {
        return false;
    }
    else {
        const size_t plsize = getPayloadSize();
        return other.getPayloadSize() == plsize && std::memcmp(getPayload(), other.getPayload(), plsize) == 0;
    }
}


//----------------------------------------------------------------------------
// Check if this packet is a duplicate as another one.
//----------------------------------------------------------------------------

bool ts::TSPacket::isDuplicate(const TSPacket& other) const
{
    // - There is no duplicate packets in null packets.
    // - Duplicate packets must have a payload.
    // - The 4-byte TS header of duplicate packets are always identical
    //   (same PID, CC, presence or AF and payload).
    // - The next 2 bytes must also be identical: either there is an AF and
    //   these two bytes are AF size and flags, or there no AF and this is the
    //   start of the payload. In both cases, they be must identical.
    // - Specifically, if the AF flag are present and identical, the two
    //   packets have both a PCR or no PCR at all.
    // - The PCR, if present, can be different in duplicate packets.
    // - If the PCR is present, it must be at offset 6 and is 6 bytes long.

    const size_t offset = hasPCR() ? 12 : 6;
    return hasPayload() &&
        getPID() != PID_NULL &&
        std::memcmp(b, other.b, 6) == 0 &&
        std::memcmp(b + offset, other.b + offset, PKT_SIZE - offset) == 0;
}


//----------------------------------------------------------------------------
// Locate contiguous TS packets into a buffer.
//----------------------------------------------------------------------------

bool ts::TSPacket::Locate(const uint8_t* buffer, size_t buffer_size, size_t& start_index, size_t& packet_count)
{
    // Nothing found by default.
    start_index = 0;
    packet_count = 0;

    // Filter out invalid parameters.
    if (buffer == nullptr || buffer_size < PKT_SIZE) {
        return false;
    }

    // Look backward from the end of the message, looking for a 0x47 sync byte every 188 bytes, going backward.
    const uint8_t* buffer_end = buffer + buffer_size;
    const uint8_t* p;
    for (p = buffer_end; p >= buffer + PKT_SIZE && *(p - PKT_SIZE) == SYNC_BYTE; p -= PKT_SIZE) {}

    if (p < buffer_end) {
        // Some packets were found
        start_index = p - buffer;
        packet_count = (buffer_end - p) / PKT_SIZE;
        return true;
    }

    // No TS packet found using the first method. Restart from the beginning of the message.
    const uint8_t* const max = buffer_end - PKT_SIZE; // max address for a TS packet
    for (p = buffer; p <= max; p++) {
        if (*p == SYNC_BYTE) {
            // Found something that could be the start of a TS packet.
            // Verify that we get a 0x47 sync byte every 188 bytes up
            // to the end of message (not leaving more than one truncated
            // TS packet at the end of the message).
            const uint8_t* end;
            for (end = p; end <= max && *end == SYNC_BYTE; end += PKT_SIZE) {}
            if (end > max) {
                // End is after the start of the last possible packet.
                // So, there are less than 188 bytes left in buffer after last packet.
                // Consider that we have found a valid suite of packets.
                start_index = p - buffer;
                packet_count = (end - p) / PKT_SIZE;
                return true;
            }
        }
    }

    // Could not find a valid suite of TS packets.
    return false;
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
        report.error(u"I/O error while reading TS packet%s", {AfterPackets(position)});
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
// Formatting helpers
//----------------------------------------------------------------------------

namespace {
    ts::UString timeStampsString(uint64_t pcr, uint64_t opcr)
    {
        ts::UString str;
        if (pcr != ts::INVALID_PCR) {
            str.format(u"PCR: 0x%011X", {pcr});
            if (opcr != ts::INVALID_PCR) {
                str.append(u", ");
            }
        }
        if (opcr != ts::INVALID_PCR) {
            str.format(u"OPCR: 0x%011X", {opcr});
        }
        return str;
    }
}

//----------------------------------------------------------------------------
// This method displays the content of a transport packet.
//----------------------------------------------------------------------------

std::ostream& ts::TSPacket::display(std::ostream& strm, uint32_t flags, size_t indent, size_t max_size) const
{
    const UString margin(indent, ' ');

    // The 16 MSB contains flags specific to ts_dump_packet.
    // The 16 LSB contains flags for ts_hexa_dump.
    // Supply default dump option:
    if ((flags & 0xFFFF0000) == 0) {
        flags |= DUMP_RAW;
    }

    // Filter invalid packets
    if (!hasValidSync()) {
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

    // Timestamps information (can be INVALID_xxx values).
    const uint64_t pcr = getPCR();
    const uint64_t opcr = getOPCR();
    const uint64_t subpcr = pcr == INVALID_PCR ? INVALID_DTS : (pcr / SYSTEM_CLOCK_SUBFACTOR);
    const uint64_t dts = getDTS();
    const uint64_t pts = getPTS();

    // Display TS header
    if (flags & DUMP_TS_HEADER) {
        strm << margin << "---- TS Header ----" << std::endl
             << margin << UString::Format(u"PID: %d (0x%X), header size: %d, sync: 0x%X", {getPID(), getPID(), header_size, b[0]}) << std::endl
             << margin << "Error: " << getTEI() << ", unit start: " << getPUSI() << ", priority: " << getPriority() << std::endl
             << margin << "Scrambling: " << int(getScrambling()) << ", continuity counter: " << int(getCC()) << std::endl
             << margin << "Adaptation field: " << UString::YesNo(hasAF()) << " (" << getAFSize() << " bytes)"
             << ", payload: " << UString::YesNo(hasPayload()) << " (" << getPayloadSize() << " bytes)" << std::endl;

        // Without explicit adaptation field analysis, just display the most important info from AF.
        if (hasAF() && !(flags & DUMP_AF)) {
            strm << margin << "Discontinuity: " << getDiscontinuityIndicator()
                 << ", random access: " << getRandomAccessIndicator()
                 << ", ES priority: " << getESPI() << std::endl;
            if (hasSpliceCountdown()) {
                strm << margin << "Splice countdown: " << int(getSpliceCountdown()) << std::endl;
            }
            if (pcr != INVALID_PCR || opcr != INVALID_PCR) {
                strm << margin << timeStampsString(pcr, opcr) << std::endl;
            }
        }
    }

    // Display adaptation field.
    size_t afsize = getAFSize();
    if (hasAF() && (flags & DUMP_AF) && afsize > 1) {
        strm << margin << "---- Adaptation field (" << afsize << " bytes) ----" << std::endl;
        if (4 + afsize > PKT_SIZE) {
            strm << margin << "*** invalid adaptation field size" << std::endl;
            afsize = PKT_SIZE - 4;
        }
        // Deserialization buffer over AF payload (skip initial length field).
        Buffer buf(b + 5, afsize - 1);
        strm << margin << "Discontinuity: " << int(buf.getBit());
        strm << ", random access: " << int(buf.getBit());
        strm << ", ES priority: " << int(buf.getBit()) << std::endl;
        const bool PCR_flag = buf.getBool();
        const bool OPCR_flag = buf.getBool();
        const bool splicing_point_flag = buf.getBool();
        const bool transport_private_data_flag = buf.getBool();
        const bool adaptation_field_extension_flag = buf.getBool();
        if (pcr != INVALID_PCR || opcr != INVALID_PCR) {
            strm << margin << timeStampsString(pcr, opcr) << std::endl;
        }
        if (PCR_flag) {
            buf.skipBits(48);
        }
        if (OPCR_flag) {
            buf.skipBits(48);
        }
        if (splicing_point_flag && buf.canReadBits(8)) {
            strm << margin << "Splice countdown: " << int(buf.getUInt8()) << std::endl;
        }
        if (transport_private_data_flag && buf.canReadBits(8)) {
            buf.pushReadSizeFromLength(8);
            strm << margin << "Private data (" << buf.remainingReadBytes() << " bytes): " << std::endl;
            if (buf.canRead()) {
                strm << UString::Dump(buf.getBytes(), UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, margin.size() + 2, 16);
            }
            buf.popState();
        }
        if (adaptation_field_extension_flag && buf.canReadBits(8)) {
            buf.pushReadSizeFromLength(8);
            const bool ltw_flag = buf.getBool();
            const bool piecewise_rate_flag = buf.getBool();
            const bool seamless_splice_flag = buf.getBool();
            const bool af_descriptor_not_present_flag = buf.getBool();
            buf.skipBits(4);
            if (ltw_flag && buf.canReadBits(16)) {
                strm << margin << "LTW valid: " << int(buf.getBit());
                strm << ", offset: " << UString::Decimal(buf.getBits<uint16_t>(15)) << std::endl;
            }
            if (piecewise_rate_flag && buf.canReadBits(24)) {
                buf.skipBits(2);
                strm << margin << "Piecewise rate: " << UString::Decimal(buf.getBits<uint16_t>(22)) << std::endl;
            }
            if (seamless_splice_flag && buf.canReadBits(40)) {
                strm << margin << "Splice type: " << buf.getBits<int>(4) << std::endl;
                uint64_t dts_next_au = buf.getBits<uint64_t>(3) << 30;
                buf.skipBits(1);
                dts_next_au |= buf.getBits<uint64_t>(15) << 15;
                buf.skipBits(1);
                dts_next_au |= buf.getBits<uint64_t>(15);
                buf.skipBits(1);
                strm << UString::Format(u"DTS next AU: 0x%09X", {dts_next_au}) << std::endl;
            }
            if (!af_descriptor_not_present_flag) {
                strm << margin << "AF descriptors (" << buf.remainingReadBytes() << " bytes): " << std::endl;
                while (buf.canReadBytes(2)) {
                    strm << margin << "- Tag: " << NameFromDTV(u"ts.af_descriptor_tag", buf.getUInt8(), NamesFlags::FIRST) << std::endl;
                    const size_t len = buf.getUInt8();
                    strm << margin << "  Length: " << len << " bytes" << std::endl
                         << UString::Dump(buf.getBytes(len), UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, margin.size() + 2, 16);
                }
            }
            buf.popState();
        }
        if (buf.canRead()) {
            strm << margin << "Stuffing (" << buf.remainingReadBytes() << " bytes): " << std::endl
                 << UString::Dump(buf.getBytes(), UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, margin.size() + 2, 16);
        }
    }

    // Display PES header
    if (startPES() && (flags & DUMP_PES_HEADER)) {
        uint8_t sid = b[header_size + 3];
        uint16_t length = GetUInt16(b + header_size + 4);
        strm << margin << "---- PES Header ----" << std::endl
             << margin << "Stream id: " << NameFromDTV(u"pes.stream_id", sid, NamesFlags::FIRST) << std::endl
             << margin << "PES packet length: " << length;
        if (length == 0) {
            strm << " (unbounded)";
        }
        strm << std::endl;
        if (dts != INVALID_DTS || pts != INVALID_PTS) {
            strm << margin;
            if (dts != INVALID_DTS) {
                strm << UString::Format(u"DTS: 0x%09X", {dts});
                if (subpcr != INVALID_DTS) {
                    strm << UString::Format(u" (PCR%+'d ms)", {((SubSecond(dts) - SubSecond(subpcr)) * MilliSecPerSec) / SYSTEM_CLOCK_SUBFREQ});
                }
                if (pts != INVALID_PTS) {
                    strm << ", ";
                }
            }
            if (pts != INVALID_PTS) {
                strm << UString::Format(u"PTS: 0x%09X", {pts});
                if (dts != INVALID_DTS || subpcr != INVALID_DTS) {
                    strm << " (";
                }
                if (dts != INVALID_DTS) {
                    strm << UString::Format(u"DTS%+'d ms", {((SubSecond(pts) - SubSecond(dts)) * MilliSecPerSec) / SYSTEM_CLOCK_SUBFREQ});
                }
                if (dts != INVALID_DTS && subpcr != INVALID_DTS) {
                    strm << ", ";
                }
                if (subpcr != INVALID_DTS) {
                    strm << UString::Format(u"PCR%+'d ms", {((SubSecond(pts) - SubSecond(subpcr)) * MilliSecPerSec) / SYSTEM_CLOCK_SUBFREQ});
                }
                if (dts != INVALID_DTS || subpcr != INVALID_DTS) {
                    strm << ")";
                }
            }
            strm << std::endl;
        }
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
