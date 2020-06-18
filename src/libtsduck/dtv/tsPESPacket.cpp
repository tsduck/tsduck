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

#include "tsPESPacket.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PESPacket::PESPacket(PID source_pid) :
    _is_valid(false),
    _header_size(0),
    _source_pid(source_pid),
    _stream_type(ST_NULL),
    _first_pkt(0),
    _last_pkt(0),
    _data()
{
}

ts::PESPacket::PESPacket(const PESPacket& pp, ShareMode mode) :
    _is_valid(pp._is_valid),
    _header_size(pp._header_size),
    _source_pid(pp._source_pid),
    _stream_type(pp._stream_type),
    _first_pkt(pp._first_pkt),
    _last_pkt(pp._last_pkt),
    _data()
{
    switch (mode) {
        case ShareMode::SHARE:
            _data = pp._data;
            break;
        case ShareMode::COPY:
            _data = pp._is_valid ? new ByteBlock(*pp._data) : nullptr;
            break;
        default:
            // should not get there
            assert(false);
    }
}

ts::PESPacket::PESPacket(PESPacket&& pp) noexcept :
    _is_valid(pp._is_valid),
    _header_size(pp._header_size),
    _source_pid(pp._source_pid),
    _stream_type(pp._stream_type),
    _first_pkt(pp._first_pkt),
    _last_pkt(pp._last_pkt),
    _data(std::move(pp._data))
{
}

ts::PESPacket::PESPacket(const void* content, size_t content_size, PID source_pid) :
    PESPacket(source_pid)
{
    initialize(new ByteBlock(content, content_size));
}

ts::PESPacket::PESPacket(const ByteBlock& content, PID source_pid) :
    PESPacket(source_pid)
{
    initialize(new ByteBlock(content));
}

ts::PESPacket::PESPacket(const ByteBlockPtr& content_ptr, PID source_pid) :
    PESPacket(source_pid)
{
    initialize(content_ptr);
}


//----------------------------------------------------------------------------
// Initialize from a binary content.
//----------------------------------------------------------------------------

void ts::PESPacket::initialize(const ByteBlockPtr& bbp)
{
    _is_valid = false;
    _header_size = 0;
    _first_pkt = 0;
    _last_pkt = 0;
    _data.clear();

    if (bbp.isNull()) {
        return;
    }

    // Fixed common header size
    const uint8_t* data = bbp->data();
    size_t size = bbp->size();
    if (size < 6) {
        return;
    }

    // Check start code prefix: 00 00 01
    if (data[0] != 0 || data[1] != 0 || data[2] != 1) {
        return;
    }

    // Packet structure depends on stream_id
    if (IsLongHeaderSID(data[3])) {
        // Header size
        if (size < 9) {
            return;
        }
        _header_size = 9 + size_t (data[8]);
        if (size < _header_size) {
            return;
        }
    }
    else {
        // No additional header fields
        _header_size = 6;
    }

    // Passed all checks
    _is_valid = true;
    _data = bbp;
}


//----------------------------------------------------------------------------
// Clear packet content.
//----------------------------------------------------------------------------

void ts::PESPacket::clear()
{
    _is_valid = false;
    _header_size = 0;
    _source_pid = PID_NULL;
    _stream_type = ST_NULL;
    _data.clear();
}


//----------------------------------------------------------------------------
// Reload from full binary content.
//----------------------------------------------------------------------------

void ts::PESPacket::reload(const void* content, size_t content_size, PID source_pid)
{
    _source_pid = source_pid;
    initialize(new ByteBlock(content, content_size));
}

void ts::PESPacket::reload(const ByteBlock& content, PID source_pid)
{
    _source_pid = source_pid;
    initialize(new ByteBlock(content));
}

void ts::PESPacket::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    _source_pid = source_pid;
    initialize(content_ptr);
}


//----------------------------------------------------------------------------
// Size of the binary content of the packet.
//----------------------------------------------------------------------------

size_t ts::PESPacket::size() const
{
    if (_is_valid) {
        // Check if an actual size is specified.
        const size_t psize = GetUInt16(_data->data() + 4);
        // When the specified size is zero, get the complete binary data.
        return psize == 0 ? _data->size() : std::min(psize + 6, _data->size());
    }
    else {
        // Invalid PES packet.
        return 0;
    }
}


//----------------------------------------------------------------------------
// Set the stream id of the PES packet.
//----------------------------------------------------------------------------

void ts::PESPacket::setStreamId(uint8_t sid)
{
    if (_is_valid) {
        (*_data)[3] = sid;
    }
}


//----------------------------------------------------------------------------
// Assignment.
//----------------------------------------------------------------------------

ts::PESPacket& ts::PESPacket::operator=(const PESPacket& pp)
{
    _is_valid = pp._is_valid;
    _header_size = pp._header_size;
    _source_pid = pp._source_pid;
    _stream_type = pp._stream_type;
    _first_pkt = pp._first_pkt;
    _last_pkt = pp._last_pkt;
    _data = pp._data;
    return *this;
}

ts::PESPacket& ts::PESPacket::operator=(PESPacket&& pp) noexcept
{
    _is_valid = pp._is_valid;
    _header_size = pp._header_size;
    _source_pid = pp._source_pid;
    _stream_type = pp._stream_type;
    _first_pkt = pp._first_pkt;
    _last_pkt = pp._last_pkt;
    _data = std::move(pp._data);
    return *this;
}


//----------------------------------------------------------------------------
// Duplication. Similar to assignment but the content of the packet
// is duplicated.
//----------------------------------------------------------------------------

ts::PESPacket& ts::PESPacket::copy(const PESPacket& pp)
{
    _is_valid = pp._is_valid;
    _header_size = pp._header_size;
    _source_pid = pp._source_pid;
    _stream_type = pp._stream_type;
    _first_pkt = pp._first_pkt;
    _last_pkt = pp._last_pkt;
    _data = pp._is_valid ? new ByteBlock(*pp._data) : nullptr;
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
// The source PID are ignored, only the packet contents are compared.
// Note: Invalid packets are never identical
//----------------------------------------------------------------------------

bool ts::PESPacket::operator==(const PESPacket& pp) const
{
    return _is_valid && pp._is_valid && (_data == pp._data || *_data == *pp._data);
}


//----------------------------------------------------------------------------
// Check if the PES packet contains MPEG-2 video (also applies to MPEG-1 video)
//----------------------------------------------------------------------------

bool ts::PESPacket::isMPEG2Video() const
{
    // Must have a video stream_id and payload must start with 00 00 01.
    if (_stream_type == ST_MPEG1_VIDEO || _stream_type == ST_MPEG2_VIDEO || _stream_type == ST_MPEG2_3D_VIEW) {
        return true;
    }
    else if (_stream_type != ST_NULL || !IsVideoSID(getStreamId())) {
        return false;
    }
    else {
        const uint8_t* pl = payload();
        size_t pl_size = payloadSize();
        return pl_size >= 3 && pl[0] == 0x00 && pl[1] == 0x00 && pl[2] == 0x01;
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains AVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAVC() const
{
    // Must have a video stream_id and payload must start with 00 00 00 [00...] 01
    if (_stream_type == ST_AVC_VIDEO ||
        _stream_type == ST_AVC_3D_VIEW ||
        _stream_type == ST_AVC_SUBVIDEO_G ||
        _stream_type == ST_AVC_SUBVIDEO_H ||
        _stream_type == ST_AVC_SUBVIDEO_I)
    {
        return true;
    }
    else if (_stream_type != ST_NULL || !IsVideoSID(getStreamId())) {
        return false;
    }
    else {
        const uint8_t* pl = payload();
        size_t pl_size = payloadSize();
        while (pl_size > 0 && *pl == 0x00) {
            ++pl;
            --pl_size;
        }
        return pl_size > 0 && *pl == 0x01 && pl > payload() + 2;
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains HEVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::isHEVC() const
{
    // Currently, only test the stream type from the PMT.
    // Can we use additional non-ambiguous test on the PES payload?
    return
        _stream_type == ST_HEVC_VIDEO ||
        _stream_type == ST_HEVC_SUBVIDEO ||
        _stream_type == ST_HEVC_SUBVIDEO_G ||
        _stream_type == ST_HEVC_SUBVIDEO_TG ||
        _stream_type == ST_HEVC_SUBVIDEO_H ||
        _stream_type == ST_HEVC_SUBVIDEO_TH;
}


//----------------------------------------------------------------------------
// Check if the PES packet contains AC-3 or Enhanced-AC-3.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAC3() const
{
    // Payload must start with 0B 77
    if (_stream_type == ST_AC3_AUDIO || _stream_type == ST_EAC3_AUDIO) {
        // ATSC defined stream type.
        return true;
    }
    else if (_stream_type != ST_NULL && _stream_type != ST_PES_PRIV) {
        // In DVB systems, there is no stream type for AC-3. AC-3 streams are
        // defined by "PES private data" and an AC-3 descriptor.
        return false;
    }
    else {
        const uint8_t* pl = payload();
        size_t pl_size = payloadSize();
        return pl_size > 2 && pl[0] == 0x0B && pl[1] == 0x77;
    }
}
