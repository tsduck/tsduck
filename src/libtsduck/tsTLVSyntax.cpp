//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of a Tag, Length, Value (TVL) syntax.
//
//----------------------------------------------------------------------------

#include "tsTLVSyntax.h"
#include "tsStringUtils.h"
#include "tsToInteger.h"
#include "tsIntegerUtils.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TLVSyntax::TLVSyntax(int start, int size, size_t tagSize, size_t lengthSize, bool msb, ReportInterface& report) :
    _start(0),
    _size(0),
    _tagSize(1),
    _lengthSize(1),
    _msb(msb)
{
    set(start, size, tagSize, lengthSize, msb, report);
}


//----------------------------------------------------------------------------
// Set the values of a TLVSyntax object.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::set(int start, int size, size_t tagSize, size_t lengthSize, bool msb, ReportInterface& report)
{
    if (tagSize != 1 && tagSize != 2 && tagSize != 4) {
        report.error("invalid tag size %d", tagSize);
        return false;
    }
    else if (lengthSize != 1 && lengthSize != 2 && lengthSize != 4) {
        report.error("invalid length size %d", lengthSize);
        return false;
    }
    else {
        _start = start;
        _size = size;
        _tagSize = tagSize;
        _lengthSize = lengthSize;
        _msb = msb;
        return true;
    }
}


//----------------------------------------------------------------------------
// Set the values of a TLVSyntax object from a string representation.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::fromString(const std::string& s, ReportInterface& report)
{
    // Reset default values in this object.
    set();

    // List of string fields.
    StringVector fields;
    SplitString(fields, s, ',', true);
    bool ok = fields.size() <= 5;

    // Decode each field. Empty or "auto" values means default value.
    if (ok && fields.size() > 0 && !fields[0].empty() && !SimilarStrings(fields[0], "auto")) {
        ok = ToInteger(_start, fields[0], ",");
    }
    if (ok && fields.size() > 1 && !fields[1].empty() && !SimilarStrings(fields[1], "auto")) {
        ok = ToInteger(_size, fields[1], ",");
    }
    if (ok && fields.size() > 2 && !fields[2].empty()) {
        ok = ToInteger(_tagSize, fields[2]) && (_tagSize == 1 || _tagSize == 2 || _tagSize == 4);
    }
    if (ok && fields.size() > 3 && !fields[3].empty()) {
        ok = ToInteger(_lengthSize, fields[3]) && (_lengthSize == 1 || _lengthSize == 2 || _lengthSize == 4);
    }
    if (ok && fields.size() > 4 && !fields[4].empty()) {
        _msb = SimilarStrings(fields[4], "msb");
        ok = _msb || SimilarStrings(fields[4], "lsb");
    }

    // Handle errors.
    if (!ok) {
        report.error("invalid TLV syntax specification \"%s\", use \"start,size,tagSize,lengthSize,msb|lsb\"", s.c_str());
    }
    return ok;
}


//----------------------------------------------------------------------------
// Extract a tag and length value from a data area.
//----------------------------------------------------------------------------

size_t ts::TLVSyntax::getTagAndLength(const uint8_t* data, size_t size, uint32_t& tag, size_t& length) const
{
    const size_t headerSize = _tagSize + _lengthSize;
    if (size < headerSize) {
        // Cannot fit TL fields.
        tag = 0;
        length = 0;
        return 0;
    }
    else {
        tag = getInt(data, _tagSize);
        length = getInt(data + _tagSize, _lengthSize);
        // Check if the V field fits (and prevent overflow).
        return size - headerSize < length ? 0 : headerSize;
    }
}


//----------------------------------------------------------------------------
// Locate the "TLV area" inside a data area.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::locateTLV(const uint8_t* data, size_t dataSize, size_t& tlvStart, size_t& tlvSize) const
{
    const size_t headerSize = _tagSize + _lengthSize;

    // Default values.
    tlvStart = tlvSize = 0;

    if (_start >= 0 && _size >= 0) {
        // Fixed TLV area.
        if (size_t(_start + _size) > dataSize) {
            return false; // Won't fit
        }
        else {
            tlvStart = _start;
            tlvSize = _size;
            return true;
        }
    }
    else if (_start >= 0) {
        // Fixed starting offset, auto size.
        if (size_t(_start) > dataSize) {
            return false; // Won't fit
        }
        else {
            tlvStart = _start;
            tlvSize = longestTLV(data, dataSize, _start);
            return true;
        }
    }
    else {
        // Auto start index, need to find the best matching TLV area.
        for (size_t index = 0; index < dataSize - tlvSize; index++) {
            const size_t size = longestTLV(data, dataSize, index);
            if (size > tlvSize) {
                // Found a longer match.
                tlvStart = index;
                tlvSize = size;
            }
        }
        return tlvSize > 0;
    }
}


//----------------------------------------------------------------------------
// Compute the size of the longest TLV area starting at tlvStart.
//----------------------------------------------------------------------------

size_t ts::TLVSyntax::longestTLV(const uint8_t * data, size_t dataSize, size_t tlvStart) const
{
    const size_t headerSize = _tagSize + _lengthSize;
    size_t index = tlvStart;
    while (index + headerSize <= dataSize) {
        const size_t len = getInt(data + index + _tagSize, _lengthSize);
        const size_t next = BoundedAdd(index, BoundedAdd(headerSize, len));
        if (next > dataSize) {
            break; // would overflow data area
        }
        index = next;
    }
    return index - tlvStart;
}


//----------------------------------------------------------------------------
// Get an integer in the right byte order.
//----------------------------------------------------------------------------

uint32_t ts::TLVSyntax::getInt(const uint8_t * data, size_t size) const
{
    if (_msb) {
        switch (size) {
            case 1: return GetUInt8(data);
            case 2: return GetUInt16BE(data);
            case 4: return GetUInt32BE(data);
            default: assert(false); return 0;
        }
    }
    else {
        switch (size) {
            case 1: return GetUInt8(data);
            case 2: return GetUInt16LE(data);
            case 4: return GetUInt32LE(data);
            default: assert(false); return 0;
        }
    }
}
