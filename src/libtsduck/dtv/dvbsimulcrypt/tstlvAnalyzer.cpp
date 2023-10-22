//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvAnalyzer.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tlv::Analyzer::Analyzer(const void* addr, size_t size) :
    _base(static_cast<const char*>(addr)),
    _end(_base + size),
    _eom(size == 0),
    _valid(true),
    _tlv_addr(_base),
    _value_addr(_base)
{
    next();
}


//----------------------------------------------------------------------------
// This method analyzes the next TLV field in the stream
//----------------------------------------------------------------------------

void ts::tlv::Analyzer::next()
{
    // Don't change if already at end of message or structure error found
    if (_eom || !_valid) {
        return;
    }

    // Compute address of next TLV
    _tlv_addr = _value_addr + _length;

    // Detect end of message
    if (_tlv_addr == _end) {
        _eom = true;
        return;
    }

    // Check if there is enough space for tag and length fields
    if (_tlv_addr + sizeof(TAG) + sizeof(LENGTH) > _end) {
        _eom = true;
        _valid = false;
        return;
    }

    // Get tag, length and value
    _tag = GetUInt16 (_tlv_addr);
    _length = GetUInt16 (_tlv_addr + sizeof(TAG));
    _value_addr = _tlv_addr + sizeof(TAG) + sizeof(LENGTH);
    _tlv_size = _value_addr + _length - _tlv_addr;

    // Check that the value fit in the message
    if (_value_addr + _length > _end) {
        _eom = true;
        _valid = false;
    }
}
