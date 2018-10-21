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

#include "tsT2MIPacket.h"
#include "tsCRC32.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::T2MIPacket::T2MIPacket() :
    _is_valid(false),
    _source_pid(PID_NULL),
    _data()
{
}


//----------------------------------------------------------------------------
// Copy constructor. The packet content is either shared or copied.
//----------------------------------------------------------------------------

ts::T2MIPacket::T2MIPacket(const T2MIPacket& pp, CopyShare mode) :
    _is_valid(pp._is_valid),
    _source_pid(pp._source_pid),
    _data()
{
    switch (mode) {
        case SHARE:
            _data = pp._data;
            break;
        case COPY:
            _data = pp._is_valid ? new ByteBlock(*pp._data) : nullptr;
            break;
        default:
            // should not get there
            assert(false);
    }
}


//----------------------------------------------------------------------------
// Constructors from binary content.
//----------------------------------------------------------------------------

ts::T2MIPacket::T2MIPacket(const void* content, size_t content_size, PID source_pid) :
    _is_valid(false),
    _source_pid(source_pid),
    _data()
{
    initialize(new ByteBlock(content, content_size));
}

ts::T2MIPacket::T2MIPacket(const ByteBlock& content, PID source_pid) :
    _is_valid(false),
    _source_pid(source_pid),
    _data()
{
    initialize(new ByteBlock(content));
}

ts::T2MIPacket::T2MIPacket(const ByteBlockPtr& content_ptr, PID source_pid) :
    _is_valid(false),
    _source_pid(source_pid),
    _data()
{
    initialize(content_ptr);
}


//----------------------------------------------------------------------------
// Initialize from a binary content.
//----------------------------------------------------------------------------

void ts::T2MIPacket::initialize(const ByteBlockPtr& bbp)
{
    _is_valid = false;
    _data.clear();

    if (bbp.isNull()) {
        return;
    }

    // Check fixed header size
    const uint8_t* data = bbp->data();
    size_t size = bbp->size();
    if (size < T2MI_HEADER_SIZE) {
        return;
    }

    // Check packet size.
    const uint16_t payload_bytes = (GetUInt16(data + 4) + 7) / 8;
    if (T2MI_HEADER_SIZE + payload_bytes + SECTION_CRC32_SIZE != size) {
        return;
    }

    // Get CRC from packet and recompute CRC from header + payload.
    const uint32_t pktCRC = GetUInt32(data + T2MI_HEADER_SIZE + payload_bytes);
    const uint32_t compCRC = CRC32(data, T2MI_HEADER_SIZE + payload_bytes);
    if (pktCRC != compCRC) {
        // Invalid CRC in M2-TI packet.
        return;
    }

    // Passed all checks
    _is_valid = true;
    _data = bbp;
}


//----------------------------------------------------------------------------
// Clear packet content.
//----------------------------------------------------------------------------

void ts::T2MIPacket::clear()
{
    _is_valid = false;
    _source_pid = PID_NULL;
    _data.clear();
}


//----------------------------------------------------------------------------
// Assignment. The packet content is referenced, and thus shared
// between the two packet objects.
//----------------------------------------------------------------------------

ts::T2MIPacket& ts::T2MIPacket::operator=(const T2MIPacket& pp)
{
    if (&pp != this) {
        _is_valid = pp._is_valid;
        _source_pid = pp._source_pid;
        _data = pp._data;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the packet
// is duplicated.
//----------------------------------------------------------------------------

ts::T2MIPacket& ts::T2MIPacket::copy(const T2MIPacket& pp)
{
    if (&pp != this) {
        _is_valid = pp._is_valid;
        _source_pid = pp._source_pid;
        _data = pp._is_valid ? new ByteBlock(*pp._data) : nullptr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
// The source PID are ignored, only the packet contents are compared.
// Note: Invalid packets are never identical
//----------------------------------------------------------------------------

bool ts::T2MIPacket::operator==(const T2MIPacket& pp) const
{
    return _is_valid && pp._is_valid && (_data == pp._data || *_data == *pp._data);
}
