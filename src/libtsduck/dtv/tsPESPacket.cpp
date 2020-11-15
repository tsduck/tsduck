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
#include "tsPES.h"
#include "tsAVC.h"
#include "tsAVCAccessUnitDelimiter.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PESPacket::PESPacket(PID source_pid) :
    _is_valid(false),
    _header_size(0),
    _source_pid(source_pid),
    _stream_type(ST_NULL),
    _pcr(INVALID_PCR),
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
    _pcr(pp._pcr),
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
    _pcr(pp._pcr),
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
// Get the header size of the start of a PES packet. Return 0 on error.
//----------------------------------------------------------------------------

size_t ts::PESPacket::HeaderSize(const uint8_t* data, size_t size)
{
    // Fixed minimum common PES header size is 6 bytes.
    if (data == nullptr || size < 6) {
        return 0;
    }

    // Check start code prefix: 00 00 01
    if (data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x01) {
        return 0;
    }

    // Packet structure depends on stream_id nn: 00 00 01 nn
    if (IsLongHeaderSID(data[3])) {
        // Header size
        if (size < 9) {
            return 0;
        }
        const size_t header_size = 9 + size_t(data[8]);
        return header_size > size ? 0 : header_size;
    }
    else {
        // No additional header fields, common PES header size.
        return 6;
    }
}


//----------------------------------------------------------------------------
// Initialize from a binary content.
//----------------------------------------------------------------------------

void ts::PESPacket::initialize(const ByteBlockPtr& bbp)
{
    _is_valid = false;
    _header_size = 0;
    _pcr = INVALID_PCR;
    _first_pkt = 0;
    _last_pkt = 0;
    _data.clear();

    if (bbp.isNull()) {
        return;
    }

    // PES header size
    const uint8_t* const data = bbp->data();
    const size_t size = bbp->size();
    _header_size = HeaderSize(data, size);
    if (_header_size == 0) {
        return;
    }

    // Check that the embedded size is either zero (unbounded) or within actual data size.
    // This field indicates the packet length _after_ that field (ie. after offset 6).
    const size_t psize = 6 + size_t(GetUInt16(data + 4));
    if (psize != 6 && (psize < _header_size || psize > size)) {
        return;
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
    _pcr = INVALID_PCR;
    _first_pkt = 0;
    _last_pkt = 0;
    _data.clear();
}


//----------------------------------------------------------------------------
// Set the PCR value for this PES packet.
//----------------------------------------------------------------------------

void ts::PESPacket::setPCR(uint64_t pcr)
{
    // Make sure that all invalid PCR values are represented by the same value.
    _pcr = pcr <= MAX_PCR ? pcr : INVALID_PCR;
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
// Stream id of the PES packet.
//----------------------------------------------------------------------------

uint8_t ts::PESPacket::getStreamId() const
{
    return _is_valid ? (*_data)[3] : 0;
}

void ts::PESPacket::setStreamId(uint8_t sid)
{
    if (_is_valid) {
        (*_data)[3] = sid;
    }
}


//----------------------------------------------------------------------------
// Check if the packet has a long header.
//----------------------------------------------------------------------------

bool ts::PESPacket::hasLongHeader() const
{
    return _is_valid && IsLongHeaderSID((*_data)[3]);
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
    _pcr = pp._pcr;
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
    _pcr = pp._pcr;
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
    _pcr = pp._pcr;
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
    return _is_valid && IsMPEG2Video(content(), size(), _stream_type);
}

bool ts::PESPacket::IsMPEG2Video(const uint8_t* data, size_t size, uint8_t stream_type)
{
    // Must have a video stream_id and payload must start with 00 00 01.
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0 || size < header_size + 3) {
        return false;
    }
    else if (stream_type == ST_MPEG1_VIDEO || stream_type == ST_MPEG2_VIDEO || stream_type == ST_MPEG2_3D_VIEW) {
        return true;
    }
    else if (stream_type != ST_NULL || !IsVideoSID(data[3])) {
        return false;
    }
    else {
        return data[header_size] == 0x00 && data[header_size + 1] == 0x00 && data[header_size + 2] == 0x01;
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains AVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAVC() const
{
    return _is_valid && IsAVC(content(), size(), _stream_type);
}

bool ts::PESPacket::IsAVC(const uint8_t* data, size_t size, uint8_t stream_type)
{
    // Must have a video stream_id and payload must start with 00 00 00 [00...] 01
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0 || size < header_size + 4) {
        return false;
    }
    else if (StreamTypeIsAVC(stream_type)) {
        return true;
    }
    else if (stream_type != ST_NULL || !IsVideoSID(data[3])) {
        return false;
    }
    else {
        // Check that the payload starts with 00 00 00 [00...] 01
        const uint8_t* pl = data + header_size;
        size_t pl_size = size - header_size;
        while (pl_size > 0 && *pl == 0x00) {
            ++pl;
            --pl_size;
        }
        return pl_size > 0 && *pl == 0x01 && pl > data + header_size + 2;
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains HEVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::isHEVC() const
{
    return _is_valid && IsHEVC(content(), size(), _stream_type);
}

bool ts::PESPacket::IsHEVC(const uint8_t* data, size_t size, uint8_t stream_type)
{
    // Currently, only test the stream type from the PMT.
    // Can we use additional non-ambiguous test on the PES payload?
    const size_t header_size = HeaderSize(data, size);
    return header_size > 0 && StreamTypeIsHEVC(stream_type);
}


//----------------------------------------------------------------------------
// Check if the PES packet contains AC-3 or Enhanced-AC-3.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAC3() const
{
    return _is_valid && IsAC3(content(), size(), _stream_type);
}

bool ts::PESPacket::IsAC3(const uint8_t* data, size_t size, uint8_t stream_type)
{
    // Payload must start with 0B 77
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0 || size < header_size + 2) {
        return false;
    }
    else if (stream_type == ST_AC3_AUDIO || stream_type == ST_EAC3_AUDIO) {
        // ATSC defined stream type.
        return true;
    }
    else if (stream_type != ST_NULL && stream_type != ST_PES_PRIV) {
        // In DVB systems, there is no stream type for AC-3. AC-3 streams are
        // defined by "PES private data" and an AC-3 descriptor.
        return false;
    }
    else {
        return data[header_size] == 0x0B && data[header_size + 1] == 0x77;
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains an intra-coded image.
//----------------------------------------------------------------------------

size_t ts::PESPacket::findIntraImage() const
{
    return _is_valid ? FindIntraImage(content(), size(), _stream_type) : NPOS;
}

size_t ts::PESPacket::FindIntraImage(const uint8_t* data, size_t size, uint8_t stream_type)
{
    // Check PES structure, we need as least a valid PES header.
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0) {
        return NPOS;
    }

    // Packet payload content, possibly truncated.
    const uint8_t* pdata = data + header_size;
    size_t psize = size - header_size;

    // Start code prefix for ISO 11172-2 (MPEG-1 video) and ISO 13818-2 (MPEG-2 video)
    static const uint8_t StartCodePrefix[] = {0x00, 0x00, 0x01};

    // End of AVC NALunit delimiter
    static const uint8_t Zero3[] = {0x00, 0x00, 0x00};

    // Process MPEG-1 (ISO 11172-2) and MPEG-2 (ISO 13818-2) video start codes
    if (IsMPEG2Video(data, size, stream_type)) {
        // Locate all start codes and detect start of Group of Pictures (GOP).
        // The beginning of the PES payload is already a start code prefix in MPEG-1/2.
        while (psize > 0) {
            // Look for next start code
            const uint8_t* pnext = LocatePattern(pdata + 1, psize - 1, StartCodePrefix, sizeof(StartCodePrefix));
            if (pnext == nullptr) {
                // No next start code, current one extends up to the end of the payload.
                pnext = pdata + psize;
            }
            // The start code is after the start code prefix: 00 00 01 xx
            if (pdata + 3 < pnext && pdata[3] == PST_GROUP) {
                // Found a start of GOP. This must be an intra-image in MPEG-1/2.
                return pdata - data;
            }
            // Move to next start code
            psize -= pnext - pdata;
            pdata = pnext;
        }
    }

    // Process AVC (ISO 14496-10, ITU H.264) access units (aka "NALunits")
    else if (IsAVC(data, size, stream_type)) {
        // With AVC, we use two types of intra-frame detection:
        // 1) Start of a NALunit of type 5 (AVC_AUT_IDR).
        // 2) Access unit delimiter (AUD) with primary_pic_type 0, 3 or 5 (AVC_PIC_TYPE_I, AVC_PIC_TYPE_SI, AVC_PIC_TYPE_I_SI).
        // The beginning of the PES payload is not a start code prefix in AVC (at least three 00 before 01).
        while (psize > 0) {
            // Locate next access unit: starts with 00 00 01.
            // The start code prefix 00 00 01 is not part of the NALunit.
            // The NALunit starts at the NALunit type byte (see H.264, 7.3.1).
            const uint8_t* p1 = LocatePattern(pdata, psize, StartCodePrefix, sizeof(StartCodePrefix));
            if (p1 == nullptr) {
                // No next access unit.
                break;
            }

            // Jump to first byte of NALunit.
            psize -= p1 + sizeof(StartCodePrefix) - pdata;
            pdata = p1 + sizeof(StartCodePrefix);

            // Locate end of access unit: ends with 00 00 00, 00 00 01 or end of data.
            const uint8_t* p2 = LocatePattern(pdata, psize, StartCodePrefix, sizeof(StartCodePrefix));
            const uint8_t* p3 = LocatePattern(pdata, psize, Zero3, sizeof(Zero3));
            size_t nalunit_size = 0;
            if (p2 == nullptr && p3 == nullptr) {
                // No 00 00 01, no 00 00 00, the NALunit extends up to the end of data.
                nalunit_size = psize;
            }
            else if (p2 == nullptr || (p3 != nullptr && p3 < p2)) {
                // NALunit ends at 00 00 00.
                assert(p3 != nullptr);
                nalunit_size = p3 - pdata;
            }
            else {
                // NALunit ends at 00 00 01.
                assert(p2 != nullptr);
                nalunit_size = p2 - pdata;
            }

            // Compute and process NALunit type.
            const uint8_t nalunit_type = nalunit_size == 0 ? 0 : (pdata[0] & 0x1F);
            if (nalunit_type == AVC_AUT_IDR) {
                // Found an explicit IDR picture.
                return pdata - data;
            }
            else if (nalunit_type == AVC_AUT_DELIMITER) {
                // Found an access unit delimiter, analyze it.
                const AVCAccessUnitDelimiter aud(pdata, nalunit_size);
                if (aud.valid && (aud.primary_pic_type == AVC_PIC_TYPE_I || aud.primary_pic_type == AVC_PIC_TYPE_SI || aud.primary_pic_type == AVC_PIC_TYPE_I_SI)) {
                    // Found an access unit delimiter which contains intra slices only.
                    return pdata - data;
                }
            }

            // Move to next start code prefix.
            pdata += nalunit_size;
            psize -= nalunit_size;
        }
    }

    // No intra-image found.
    return NPOS;
}
