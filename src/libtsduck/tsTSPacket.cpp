//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsByteBlock.h"
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
// Initialize a TS packet.
//----------------------------------------------------------------------------

void ts::TSPacket::init(PID pid, uint8_t cc, uint8_t data)
{
    b[0] = 0x47;
    b[1] = uint8_t(pid >> 8) & 0x1F;
    b[2] = uint8_t(pid);
    b[3] = 0x10 | (cc & 0x0F); // no adaptation field, payload only.
    ::memset(b + 4, data, PKT_SIZE - 4);
}

//----------------------------------------------------------------------------
// Assigment operator.
//----------------------------------------------------------------------------

ts::TSPacket& ts::TSPacket::operator=(const TSPacket& p) noexcept
{
    if (&p != this) {
        ::memcpy(b, p.b, PKT_SIZE);
    }
    return *this;
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
            ::memmove(b + PKT_SIZE - size, b + PKT_SIZE - plSize, size);
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
        ::memset(b + 5 + b[4], pad, plSize - size);
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
            ::memmove(b + PKT_SIZE - size, b + PKT_SIZE - plSize, plSize);
            // Fill the top part of the payload with the pad byte.
            ::memset(b + PKT_SIZE - add , pad, add);
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
    const uint8_t* const pl = getPayload();
    return hasValidSync() && getPUSI() && isClear() && hasPayload() &&
        getPayloadSize() >= 3 && pl[0] == 0x00 && pl[1] == 0x00 && pl[2] == 0x01;
}

//----------------------------------------------------------------------------
// These private methods compute the offset of PCR, OPCR.
// Return 0 if there is none.
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
    const size_t offset = 6 +                     // start of AF, after flags
        (((b[5] & 0x10) != 0) ? PCR_SIZE : 0) +   // skip PCR
        (((b[5] & 0x08) != 0) ? PCR_SIZE : 0) +   // skip OPCR
        (((b[5] & 0x04) != 0) ? 1 : 0);           // skip splicing countdown

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
    const size_t offset = 6 +                     // start of AF, after flags
        (((b[5] & 0x10) != 0) ? PCR_SIZE : 0) +   // skip PCR
        (((b[5] & 0x08) != 0) ? PCR_SIZE : 0) +   // skip OPCR
        (((b[5] & 0x04) != 0) ? 1 : 0);           // skip splicing countdown

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
        ::memmove(b + endNewData, b + offset, endAF - endNewData);
    }
    else {
        const size_t endPreviousData = offset + 1 + b[offset];
        if (endNewData < endPreviousData) {
            // New private data are shorter.
            // Move rest of AF downward.
            endAF = 5 + b[4];
            const size_t remove = endPreviousData - endNewData;
            ::memmove(b + endNewData, b + endPreviousData, endAF - endPreviousData);
            // Erase freeed space (now stuffing).
            ::memset(b + endAF - remove, 0xFF, remove);
        }
        else if (endNewData > endPreviousData) {
            // New private data are larger.
            const size_t add = endNewData - endPreviousData;
            if (!reserveStuffing(add, shift_payload)) {
                return false; // cannot enlarge AF.
            }
            // Move rest of AF upward.
            endAF = 5 + b[4];
            ::memmove(b + endNewData, b + endPreviousData, endAF - endNewData);
        }
    }

    // Finally write private data.
    b[5] |= 0x02;
    b[offset] = uint8_t(size);
    ::memcpy(b + 1 + offset, data, size);
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

uint64_t ts::TSPacket::getOPCR () const
{
    const size_t offset = OPCROffset ();
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
        ::memmove(b + offset, b + offset + size, 4 + afSize - offset - size);
        // Overwrite the last part of the AF, becoming AF stuffing.
        ::memset(b + 4 + afSize - size, 0xFF, size);
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
        offset = 6 + (hasPCR() ? PCR_SIZE : 0) + (hasOPCR() ? PCR_SIZE : 0);
        // Shift the existing AF 1 byte ahead to make room for the splicing point countdown value.
        ::memmove(b + offset + 1, b + offset, 4 + getAFSize() - offset - 1);
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
        // Shrink payload, enlargs or create AF.
        setPayloadSize(payload - moreAF, true);
        return true;
    }
}

//----------------------------------------------------------------------------
// Create or replace the PCR or OPCR value - 42 bits.
//----------------------------------------------------------------------------

bool ts::TSPacket::setPCR(const uint64_t &pcr, bool shift_payload)
{
    size_t offset = PCROffset();
    if (offset == 0) {
        // Currently no PCR is present, we need to create one.
        if (!reserveStuffing(PCR_SIZE, shift_payload, false)) {
            // Could not create space for PCR.
            return false;
        }
        // We will create a PCR:
        b[5] |= 0x10;  // set PCR flag
        offset = 6;    // PCR offset in packet
        // Shift the existing AF 6 bytes ahead to make room for the PCR value.
        ::memmove(b + offset + PCR_SIZE, b + offset, 4 + getAFSize() - offset - PCR_SIZE);
    }

    // Finally write the PCR value.
    PutPCR(b + offset, pcr);
    return true;
}

bool ts::TSPacket::setOPCR(const uint64_t &opcr, bool shift_payload)
{
    size_t offset = OPCROffset();
    if (offset == 0) {
        // Currently no OPCR is present, we need to create one.
        if (!reserveStuffing(PCR_SIZE, shift_payload, false)) {
            // Could not create space for PCR.
            return false;
        }
        // Set OPCR flag.
        b[5] |= 0x08;
        // Compute OPCR offset.
        offset = 6 + (hasPCR() ? PCR_SIZE : 0);
        // Shift the existing AF 6 bytes ahead to make room for the PCR value.
        ::memmove(b + offset + PCR_SIZE, b + offset, 4 + getAFSize() - offset - PCR_SIZE);
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
// Get PTS or DTS at specified offset. Return 0 if offset is zero.
//----------------------------------------------------------------------------

uint64_t ts::TSPacket::getPDTS(size_t offset) const
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
