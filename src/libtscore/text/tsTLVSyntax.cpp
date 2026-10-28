//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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

ts::TLVSyntax::TLVSyntax(int start, int size, size_t tag_size, size_t length_size, bool msb, Report& report) :
    _msb(msb)
{
    set(start, size, tag_size, length_size, msb, report);
}


//----------------------------------------------------------------------------
// Set the values of a TLVSyntax object.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::set(int start, int size, size_t tag_size, size_t length_size, bool msb, Report& report)
{
    if (tag_size != 1 && tag_size != 2 && tag_size != 4) {
        report.error(u"invalid tag size %d", tag_size);
        return false;
    }
    else if (length_size != 1 && length_size != 2 && length_size != 4) {
        report.error(u"invalid length size %d", length_size);
        return false;
    }
    else {
        _start = start;
        _size = size;
        _tag_size = tag_size;
        _length_size = length_size;
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
        ok = fields[2].toInteger(_tag_size) && (_tag_size == 1 || _tag_size == 2 || _tag_size == 4);
    }
    if (ok && fields.size() > 3 && !fields[3].empty()) {
        ok = fields[3].toInteger(_length_size) && (_length_size == 1 || _length_size == 2 || _length_size == 4);
    }
    if (ok && fields.size() > 4 && !fields[4].empty()) {
        _msb = fields[4].similar(u"msb");
        ok = _msb || fields[4].similar(u"lsb");
    }

    // Handle errors.
    if (!ok) {
        report.error(u"invalid TLV syntax specification \"%s\", use \"start,size,tagSize,lengthSize,msb|lsb\"", s);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Extract a tag and length value from a data area.
//----------------------------------------------------------------------------

size_t ts::TLVSyntax::getTagAndLength(const uint8_t* data, size_t size, uint32_t& tag, size_t& length) const
{
    const size_t headerSize = _tag_size + _length_size;
    if (size < headerSize) {
        // Cannot fit TL fields.
        tag = 0;
        length = 0;
        return 0;
    }
    else {
        tag = getInt(data, _tag_size);
        length = getInt(data + _tag_size, _length_size);
        // Check if the V field fits (and prevent overflow).
        return size - headerSize < length ? 0 : headerSize;
    }
}


//----------------------------------------------------------------------------
// Locate the "TLV area" inside a data area.
//----------------------------------------------------------------------------

bool ts::TLVSyntax::locateTLV(const uint8_t* data, size_t data_size, size_t& tlv_start, size_t& tlv_size) const
{
    // Default values.
    tlv_start = tlv_size = 0;

    if (_start >= 0 && _size >= 0) {
        // Fixed TLV area.
        if (size_t(_start + _size) > data_size) {
            return false; // Won't fit
        }
        else {
            tlv_start = _start;
            tlv_size = _size;
            return true;
        }
    }
    else if (_start >= 0) {
        // Fixed starting offset, auto size.
        if (size_t(_start) > data_size) {
            return false; // Won't fit
        }
        else {
            tlv_start = _start;
            tlv_size = longestTLV(data, data_size, _start);
            return tlv_size > 0;
        }
    }
    else {
        // Auto start index, need to find the best matching TLV area.
        for (size_t index = 0; index < data_size - tlv_size; index++) {
            const size_t size = longestTLV(data, data_size, index);
            if (size > tlv_size) {
                // Found a longer match.
                tlv_start = index;
                tlv_size = size;
            }
        }
        return tlv_size > 0;
    }
}


//----------------------------------------------------------------------------
// Compute the size of the longest TLV area starting at tlv_start.
//----------------------------------------------------------------------------

size_t ts::TLVSyntax::longestTLV(const uint8_t * data, size_t data_size, size_t tlv_start) const
{
    const size_t headerSize = _tag_size + _length_size;
    size_t index = tlv_start;
    while (index + headerSize <= data_size) {
        const size_t len = getInt(data + index + _tag_size, _length_size);
        const size_t next = bounded_add(index, bounded_add(headerSize, len));
        if (next > data_size) {
            break; // would overflow data area
        }
        index = next;
    }
    return index - tlv_start;
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
