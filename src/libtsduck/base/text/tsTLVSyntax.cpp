//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLVSyntax.h"
#include "tsIntegerUtils.h"
#include "tsUString.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::TLVSyntax::TLVSyntax(int start, int size, size_t tagSize, size_t lengthSize, bool msb, Report& report) :
    _msb(msb)
{
    set(start, size, tagSize, lengthSize, msb, report);
}


//----------------------------------------------------------------------------
// Set the values of a TLVSyntax object.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::set(int start, int size, size_t tagSize, size_t lengthSize, bool msb, Report& report)
{
    if (tagSize != 1 && tagSize != 2 && tagSize != 4) {
        report.error(u"invalid tag size %d", {tagSize});
        return false;
    }
    else if (lengthSize != 1 && lengthSize != 2 && lengthSize != 4) {
        report.error(u"invalid length size %d", {lengthSize});
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

bool ts::TLVSyntax::fromString(const UString& s, Report& report)
{
    // Reset default values in this object.
    set();

    // List of string fields.
    UStringVector fields;
    s.split(fields, u',', true);
    bool ok = fields.size() <= 5;

    // Decode each field. Empty or "auto" values means default value.
    if (ok && fields.size() > 0 && !fields[0].empty() && !fields[0].similar(u"auto")) {
        ok = fields[0].toInteger(_start, u",");
    }
    if (ok && fields.size() > 1 && !fields[1].empty() && !fields[1].similar(u"auto")) {
        ok = fields[1].toInteger(_size, u",");
    }
    if (ok && fields.size() > 2 && !fields[2].empty()) {
        ok = fields[2].toInteger(_tagSize) && (_tagSize == 1 || _tagSize == 2 || _tagSize == 4);
    }
    if (ok && fields.size() > 3 && !fields[3].empty()) {
        ok = fields[3].toInteger(_lengthSize) && (_lengthSize == 1 || _lengthSize == 2 || _lengthSize == 4);
    }
    if (ok && fields.size() > 4 && !fields[4].empty()) {
        _msb = fields[4].similar(u"msb");
        ok = _msb || fields[4].similar(u"lsb");
    }

    // Handle errors.
    if (!ok) {
        report.error(u"invalid TLV syntax specification \"%s\", use \"start,size,tagSize,lengthSize,msb|lsb\"", {s});
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
            return tlvSize > 0;
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
        const size_t next = bounded_add(index, bounded_add(headerSize, len));
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
