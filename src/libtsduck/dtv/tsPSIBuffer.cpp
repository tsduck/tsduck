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
