//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tstlvAnalyzer.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor: associate the analyzer object with the address
// and size of the binary message. The corresponding memory area
// must remain alive as long as the object exists.
// Also pre-analyze the first TLV field.
//----------------------------------------------------------------------------

ts::tlv::Analyzer::Analyzer (const void* addr, size_t size) :
    _base       (static_cast<const char*> (addr)),
    _end        (_base + size),
    _eom        (size == 0),
    _valid      (true),
    _tlv_addr   (_base),
    _tlv_size   (0),
    _tag        (0),
    _value_addr (_base),
    _length     (0)
{
    next();
}


//----------------------------------------------------------------------------
//  This method analyzes the next TLV field in the stream
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
