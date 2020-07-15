//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsDescriptorList.h"
#include "tsSection.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PSIBuffer::PSIBuffer(DuckContext& duck, size_t size) :
    Buffer(size),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, void* data, size_t size, bool read_only) :
    Buffer(data, size, read_only),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, const void* data, size_t size) :
    Buffer(data, size),
    _duck(duck)
{
}

ts::PSIBuffer::PSIBuffer(DuckContext& duck, const Section& section) :
    Buffer(section.payload(), section.payloadSize()),
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Serialize / deserialize a 13-bit PID value.
//----------------------------------------------------------------------------

ts::PID ts::PSIBuffer::getPID()
{
    if (readIsByteAligned()) {
        return getUInt16() & 0x1FFF;
    }
    else if (currentReadBitOffset() % 8 == 3) {
        return getBits<PID>(13);
    }
    else {
        setReadError();
        return PID_NULL;
    }
}

bool ts::PSIBuffer::putPID(PID pid)
{
    if (writeIsByteAligned()) {
        return putUInt16(0xE000 | pid);
    }
    else if (currentWriteBitOffset() % 8 == 3) {
        return putBits(pid, 13);
    }
    else {
        setWriteError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Serialize a 3-byte language or country code.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putLanguageCode(const UString& str, bool allow_empty)
{
    // Process empty strings as zeroes when allowed.
    if (allow_empty && str.empty()) {
        putUInt24(0);
        return !writeError();
    }

    // Generate an error if the provided code is not 3 characters long or not ASCII-only.
    // All country codes are encoded in ASCII, no exception allowed.
    bool ok = str.size() == 3;
    for (size_t i = 0; ok && i < 3; ++i) {
        ok = int(str[i]) >= 0x20 && int(str[i]) <= 0x7F;
    }
    if (ok) {
        for (size_t i = 0; i < 3; ++i) {
            putUInt8(uint8_t(str[i]));
        }
        return !writeError();
    }
    else {
        setWriteError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Deserialize a 3-byte language or country code.
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getLanguageCode()
{
    if (readError() || remainingReadBytes() < 3 || !readIsByteAligned()) {
        // No partial string read if not enough bytes are present.
        // Cannot read unaligned character codes.
        setReadError();
        return UString();
    }
    else {
        // Read 3 characters. Ignore non-ASCII characters.
        UString str;
        for (size_t i = 0; i < 3; ++i) {
            const uint8_t c = getUInt8();
            if (c >= 0x20 && c <= 0x7F) {
                str.push_back(UChar(c));
            }
        }
        return str;
    }
}


//----------------------------------------------------------------------------
// Common code the various putString functions.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putStringCommon(const UString& str, size_t start, size_t count, EncodeMethod em, bool partial, size_t min_req_size)
{
    // Make sure we can write in the buffer and has the minimum required free size.
    if (readOnly() || writeError() || remainingWriteBytes() < min_req_size) {
        setWriteError();
        return 0;
    }

    // Adjust index and size to allowed bounds.
    start = std::min(start, str.size());
    count = std::min(count, str.size() - start);

    // Encode the string.
    uint8_t* data = currentWriteAddress();
    const size_t prev_size = remainingWriteBytes();
    size_t size = prev_size;
    const size_t nchars = (_duck.charsetOut()->*em)(data, size, str, start, count);

    if (partial || nchars >= count) {
        // Some or all characters were serialized.
        // Include the serialized bytes in the written part.
        writeSeek(currentWriteByteOffset() + prev_size - size);
        return partial ? nchars : size_t(!writeError());
    }
    else {
        // Failed to serialize the whole string.
        setWriteError();
        return 0;
    }
}


//----------------------------------------------------------------------------
// Deserialize a string
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getString(size_t size)
{
    UString str;
    getString(str, size);
    return str;
}

bool ts::PSIBuffer::getString(ts::UString& str, size_t size)
{
    // NPOS means exact size of the buffer.
    if (size == NPOS) {
        size = remainingReadBytes();
    }
    if (readError() || size > remainingReadBytes()) {
        str.clear();
        setReadError();
        return false;
    }

    // Decode characters.
    if (_duck.charsetIn()->decode(str, currentReadAddress(), size)) {
        // Include the deserialized bytes in the read part.
        readSeek(currentReadByteOffset() + size);
        return true;
    }
    else {
        // Set read error and let bytes as unread.
        setReadError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Deserialize a string with byte length.
//----------------------------------------------------------------------------

ts::UString ts::PSIBuffer::getStringWithByteLength()
{
    UString str;
    getStringWithByteLength(str);
    return str;
}

bool ts::PSIBuffer::getStringWithByteLength(ts::UString& str)
{
    const uint8_t* data = currentReadAddress();
    const size_t prev_size = remainingReadBytes();
    size_t size = prev_size;

    if (!readError() && _duck.charsetIn()->decodeWithByteLength(str, data, size)) {
        // Include the deserialized bytes in the read part.
        readSeek(currentReadByteOffset() + prev_size - size);
        return true;
    }
    else {
        // Set read error and let bytes as unread.
        setReadError();
        return false;
    }
}


//----------------------------------------------------------------------------
// Put (serialize) a complete descriptor list.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putDescriptorList(const DescriptorList& descs, size_t start, size_t count)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);

    if (readOnly() || !writeIsByteAligned() || descs.binarySize(start, count) > remainingWriteBytes()) {
        // Write is not byte-aligned or there is not enough room to serialize the descriptors.
        setWriteError();
        return false;
    }
    else {
        // Write all descriptors (they should fit).
        const size_t next = putPartialDescriptorList(descs, start, count);
        assert(next == start + count);
        return true;
    }
}


//----------------------------------------------------------------------------
// Put (serialize) as many descriptors as possible from a descriptor list.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putPartialDescriptorList(const DescriptorList& descs, size_t start, size_t count)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);
    const size_t last = start + count;

    // Write error if not byte-aligned.
    if (readOnly() || !writeIsByteAligned()) {
        setWriteError();
        return start;
    }

    // Serialize as many descriptors as we can.
    while (start < last && descs[start]->size() <= remainingWriteBytes()) {
        const size_t written = putBytes(descs[start]->content(), descs[start]->size());
        assert(written == descs[start]->size());
        start++;
    }

    return start;
}


//----------------------------------------------------------------------------
// Put (serialize) a complete descriptor list with a 2-byte length field.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::putDescriptorListWithLength(const DescriptorList& descs, size_t start, size_t count, size_t length_bits)
{
    // Normalize start and count.
    start = std::min(start, descs.size());
    count = std::min(count, descs.size() - start);

    if (2 + descs.binarySize(start, count) > remainingWriteBytes()) {
        // Not enough room to serialize the descriptors.
        setWriteError();
        return false;
    }
    else {
        // Write all descriptors (they should fit unless there is an alignment error).
        return putPartialDescriptorListWithLength(descs, start, count, length_bits) == start + count;
    }
}


//----------------------------------------------------------------------------
// Put (serialize) as many descriptors as possible with a 2-byte length field.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::putPartialDescriptorListWithLength(const DescriptorList& descs, size_t start, size_t count, size_t length_bits)
{
    // Normalize start.
    start = std::min(start, descs.size());

    // Filter incorrect length or length alignment.
    if (readOnly() || remainingWriteBytes() < 2 || length_bits == 0 || length_bits > 16 || (!writeIsByteAligned() && currentWriteBitOffset() % 8 != 16 - length_bits)) {
        setWriteError();
        return start;
    }

    // Write stuffing bits if byte aligned.
    if (writeIsByteAligned()) {
        putBits(0xFFFF, 16 - length_bits);
    }

    // Save state where the length will be written later.
    pushReadWriteState();

    // Write a zero as place-holder for length.
    putBits(0, length_bits);
    assert(writeIsByteAligned());

    // Serialize as many descriptors as we can. Compute written size.
    size_t size_in_bytes = currentWriteByteOffset();
    start = putPartialDescriptorList(descs, start, count);
    size_in_bytes = currentWriteByteOffset() - size_in_bytes;

    // Update the length field.
    swapReadWriteState();
    putBits(size_in_bytes, length_bits);
    popReadWriteState();

    return start;
}


//----------------------------------------------------------------------------
// Get (deserialize) a descriptor list.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getDescriptorList(DescriptorList& descs, size_t length)
{
    // Normalize and check length.
    if (length == NPOS) {
        length = remainingReadBytes();
    }
    if (!readIsByteAligned() || length > remainingReadBytes()) {
        setReadError();
        return false;
    }

    // Read descriptors.
    const bool ok = descs.add(currentReadAddress(), length);
    skipBytes(length);

    if (!ok) {
        setReadError();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get (deserialize) a descriptor list with a 2-byte length field.
//----------------------------------------------------------------------------

bool ts::PSIBuffer::getDescriptorListWithLength(DescriptorList& descs, size_t length_bits)
{
    // Read the length field.
    const size_t length = getUnalignedLength(length_bits);
    bool ok = !readError();

    // Read descriptors.
    if (ok) {
        ok = descs.add(currentReadAddress(), length);
        skipBytes(length);
    }

    if (!ok) {
        setReadError();
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get a 2-byte integer field, typically a length before a descriptor list.
//----------------------------------------------------------------------------

size_t ts::PSIBuffer::getUnalignedLength(size_t length_bits)
{
    if (readError() || remainingReadBytes() < 2 || length_bits == 0 || length_bits > 16 || (!readIsByteAligned() && currentReadBitOffset() % 8 != 16 - length_bits)) {
        setReadError();
        return 0;
    }
    if (readIsByteAligned()) {
        skipBits(16 - length_bits);
    }
    const size_t length = getBits<size_t>(length_bits);
    const size_t actual_length = std::min(length, remainingReadBytes());
    assert(readIsByteAligned());
    if (length > actual_length) {
        setReadError();
    }
    return actual_length;
}
