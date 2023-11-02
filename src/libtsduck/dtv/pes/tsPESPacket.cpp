//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPESPacket.h"
#include "tsPES.h"
#include "tsMPEG2.h"
#include "tsAVC.h"
#include "tsHEVC.h"
#include "tsVVC.h"
#include "tsAccessUnitIterator.h"
#include "tsAVCAccessUnitDelimiter.h"
#include "tsHEVCAccessUnitDelimiter.h"
#include "tsVVCAccessUnitDelimiter.h"
#include "tsSingleton.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PESPacket::PESPacket(PID source_pid) :
    SuperClass(source_pid)
{
}

ts::PESPacket::PESPacket(const PESPacket& pp, ShareMode mode) :
    SuperClass(pp, mode),
    _is_valid(pp._is_valid),
    _header_size(pp._header_size),
    _stream_type(pp._stream_type),
    _codec(pp._codec),
    _pcr(pp._pcr)
{
}

ts::PESPacket::PESPacket(PESPacket&& pp) noexcept :
    SuperClass(std::move(pp)),
    _is_valid(pp._is_valid),
    _header_size(pp._header_size),
    _stream_type(pp._stream_type),
    _codec(pp._codec),
    _pcr(pp._pcr)
{
}

ts::PESPacket::PESPacket(const void* content, size_t content_size, PID source_pid) :
    SuperClass(content, content_size, source_pid)
{
    validate();
}

ts::PESPacket::PESPacket(const ByteBlock& content, PID source_pid) :
    SuperClass(content, source_pid)
{
    validate();
}

ts::PESPacket::PESPacket(const ByteBlockPtr& content_ptr, PID source_pid) :
    SuperClass(content_ptr, source_pid)
{
    validate();
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
// Validate binary content.
//----------------------------------------------------------------------------

void ts::PESPacket::validate()
{
    _is_valid = false;
    _header_size = 0;
    _pcr = INVALID_PCR;

    // PES header size
    const uint8_t* const data = content();
    const size_t dsize = SuperClass::size();
    _header_size = HeaderSize(data, dsize);
    if (_header_size == 0) {
        clear();
        return;
    }

    // Check that the embedded size is either zero (unbounded) or within actual data size.
    // This field indicates the packet length _after_ that field (ie. after offset 6).
    const size_t psize = 6 + size_t(GetUInt16(data + 4));
    if (psize != 6 && (psize < _header_size || psize > dsize)) {
        clear();
        return;
    }

    // Passed all checks
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Clear packet content.
//----------------------------------------------------------------------------

void ts::PESPacket::clear()
{
    SuperClass::clear();
    _is_valid = false;
    _header_size = 0;
    _stream_type = ST_NULL;
    _codec = CodecType::UNDEFINED;
    _pcr = INVALID_PCR;
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
    SuperClass::reload(content, content_size, source_pid);
    validate();
}

void ts::PESPacket::reload(const ByteBlock& content, PID source_pid)
{
    SuperClass::reload(content, source_pid);
    validate();
}

void ts::PESPacket::reload(const ByteBlockPtr& content_ptr, PID source_pid)
{
    SuperClass::reload(content_ptr, source_pid);
    validate();
}


//----------------------------------------------------------------------------
// Size of the binary content of the packet.
//----------------------------------------------------------------------------

size_t ts::PESPacket::size() const
{
    if (_is_valid) {
        // Check if an actual size is specified.
        const size_t psize = GetUInt16(content() + 4);
        // When the specified size is zero, get the complete binary data.
        return psize == 0 ? SuperClass::size() : std::min(psize + 6, SuperClass::size());
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
    return _is_valid ? content()[3] : 0;
}

void ts::PESPacket::setStreamId(uint8_t sid)
{
    if (_is_valid) {
        rwContent()[3] = sid;
    }
}


//----------------------------------------------------------------------------
// Check if the packet has a long header.
//----------------------------------------------------------------------------

bool ts::PESPacket::hasLongHeader() const
{
    return _is_valid && IsLongHeaderSID(content()[3]);
}


//----------------------------------------------------------------------------
// Assignment.
//----------------------------------------------------------------------------

ts::PESPacket& ts::PESPacket::operator=(const PESPacket& pp)
{
    if (&pp != this) {
        SuperClass::operator=(pp);
        _is_valid = pp._is_valid;
        _header_size = pp._header_size;
        _stream_type = pp._stream_type;
        _codec = pp._codec;
        _pcr = pp._pcr;
    }
    return *this;
}

ts::PESPacket& ts::PESPacket::operator=(PESPacket&& pp) noexcept
{
    if (&pp != this) {
        SuperClass::operator=(std::move(pp));
        _is_valid = pp._is_valid;
        _header_size = pp._header_size;
        _stream_type = pp._stream_type;
        _codec = pp._codec;
        _pcr = pp._pcr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Duplication.
//----------------------------------------------------------------------------

ts::PESPacket& ts::PESPacket::copy(const PESPacket& pp)
{
    if (&pp != this) {
        SuperClass::copy(pp);
        _is_valid = pp._is_valid;
        _header_size = pp._header_size;
        _stream_type = pp._stream_type;
        _codec = pp._codec;
        _pcr = pp._pcr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Comparison.
//----------------------------------------------------------------------------

bool ts::PESPacket::operator==(const PESPacket& pp) const
{
    return _is_valid && pp._is_valid && SuperClass::operator==(pp);
}


//----------------------------------------------------------------------------
// List of functions to check the compatibility of PES content and codec.
//----------------------------------------------------------------------------

typedef bool (*ContentCheckFunction)(const uint8_t* data, size_t size, uint8_t stream_type);
typedef std::map<ts::CodecType, ContentCheckFunction> CodecCheckMap;
TS_STATIC_INSTANCE(CodecCheckMap, ({
    std::make_pair(ts::CodecType::MPEG1_VIDEO, ts::PESPacket::IsMPEG2Video),
    std::make_pair(ts::CodecType::MPEG2_VIDEO, ts::PESPacket::IsMPEG2Video),
    std::make_pair(ts::CodecType::AC3, ts::PESPacket::IsAC3),
    std::make_pair(ts::CodecType::EAC3, ts::PESPacket::IsAC3),
    std::make_pair(ts::CodecType::AVC, ts::PESPacket::IsAVC),
    std::make_pair(ts::CodecType::HEVC, ts::PESPacket::IsHEVC),
    std::make_pair(ts::CodecType::VVC, ts::PESPacket::IsVVC),
}), StaticCodecCheckMap);


//----------------------------------------------------------------------------
// Set a default codec type.
//----------------------------------------------------------------------------

void ts::PESPacket::setDefaultCodec(CodecType default_codec)
{
    // If the codec if already set or the new one is undefined, nothing to do.
    if (_is_valid && _codec == CodecType::UNDEFINED && default_codec != CodecType::UNDEFINED) {
        // Check if the specified default codec has a PES content checking function.
        const auto it = StaticCodecCheckMap::Instance().find(default_codec);
        if (it == StaticCodecCheckMap::Instance().end() || it->second(content(), size(), _stream_type)) {
            _codec = default_codec;
        }
    }
}


//----------------------------------------------------------------------------
// Check if the PES packet contains MPEG-2 video (also applies to MPEG-1 video)
//----------------------------------------------------------------------------

bool ts::PESPacket::isMPEG2Video() const
{
    return _is_valid && (_codec == CodecType::MPEG1_VIDEO || _codec == CodecType::MPEG2_VIDEO || IsMPEG2Video(content(), size(), _stream_type));
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
// Check if the specified area starts with 00 00 00 [00...] 01,
// common header for AVC, HEVC and VVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::HasCommonVideoHeader(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    else {
        const uint8_t* pl = data;
        while (size > 0 && *pl == 0x00) {
            ++pl;
            --size;
        }
        return size > 0 && *pl == 0x01 && pl > data + 2;
    }
}


//----------------------------------------------------------------------------
// Check if a truncated PES packet may contain AVC, HEVC or VVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::IsXVC(bool (*StreamTypeCheck)(uint8_t), const uint8_t* data, size_t size, uint8_t stream_type)
{
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0) {
        return false;
    }
    else if (StreamTypeCheck(stream_type)) {
        return true;
    }
    else if (stream_type != ST_NULL || !IsVideoSID(data[3])) {
        return false;
    }
    else {
        return HasCommonVideoHeader(data + header_size, size - header_size);
    }
}


//----------------------------------------------------------------------------
// Check if a truncated PES packet may contain AVC, HEVC or VVC.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAVC() const
{
    return _is_valid && (_codec == CodecType::AVC || IsAVC(content(), size(), _stream_type));
}

bool ts::PESPacket::isHEVC() const
{
    return _is_valid && (_codec == CodecType::HEVC || IsHEVC(content(), size(), _stream_type));
}

bool ts::PESPacket::isVVC() const
{
    return _is_valid && (_codec == CodecType::VVC || IsVVC(content(), size(), _stream_type));
}


//----------------------------------------------------------------------------
// Check if the PES packet contains AC-3 or Enhanced-AC-3.
//----------------------------------------------------------------------------

bool ts::PESPacket::isAC3() const
{
    return _is_valid && (_codec == CodecType::AC3 || _codec == CodecType::EAC3 || IsAC3(content(), size(), _stream_type));
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
    return _is_valid ? FindIntraImage(content(), size(), _stream_type, _codec) : NPOS;
}

size_t ts::PESPacket::FindIntraImage(const uint8_t* data, size_t size, uint8_t stream_type, CodecType default_format)
{
    // Check PES structure, we need as least a valid PES header.
    const size_t header_size = HeaderSize(data, size);
    if (header_size == 0) {
        return NPOS;
    }

    // Packet payload content, possibly truncated.
    const uint8_t* pl_data = data + header_size;
    size_t pl_size = size - header_size;

    // Iterator on AVC/HEVC/VVC access units.
    AccessUnitIterator au_iter(pl_data, pl_size, stream_type, default_format);
    const CodecType codec = au_iter.videoFormat();

    // Process AVC/HEVC/VVC access units (aka "NALunits")
    if (au_iter.isValid()) {
        // Loop on all access units.
        for (; !au_iter.atEnd(); au_iter.next()) {
            const uint8_t au_type = au_iter.currentAccessUnitType();
            if (codec == CodecType::AVC) {
                if (au_type == AVC_AUT_IDR) {
                    // Found an explicit IDR picture.
                    // IDR = Instantaneous Decoding Refresh.
                    return au_iter.currentAccessUnitOffset();
                }
                else if (au_type == AVC_AUT_DELIMITER) {
                    // Found an access unit delimiter, analyze it.
                    const AVCAccessUnitDelimiter aud(au_iter.currentAccessUnit(), au_iter.currentAccessUnitSize());
                    if (aud.valid && (aud.primary_pic_type == AVC_PIC_TYPE_I || aud.primary_pic_type == AVC_PIC_TYPE_SI || aud.primary_pic_type == AVC_PIC_TYPE_I_SI)) {
                        // Found an access unit delimiter which contains intra slices only.
                        return au_iter.currentAccessUnitOffset();
                    }
                }
            }
            else if (codec == CodecType::HEVC) {
                if (au_type == HEVC_AUT_CRA_NUT || au_type == HEVC_AUT_IDR_N_LP || au_type == HEVC_AUT_IDR_W_RADL || au_type == HEVC_AUT_RADL_N || au_type == HEVC_AUT_RADL_R) {
                    // Found an explicit intra picture.
                    // CRA = Clear Random Access.
                    // RADL = Random Access Decodable Leading.
                    return au_iter.currentAccessUnitOffset();
                }
                else if (au_type == HEVC_AUT_AUD_NUT) {
                    // Found an access unit delimiter, analyze it.
                    const HEVCAccessUnitDelimiter aud(au_iter.currentAccessUnit(), au_iter.currentAccessUnitSize());
                    if (aud.valid && aud.pic_type == HEVC_PIC_TYPE_I) {
                        // Found an access unit delimiter which contains intra slices only.
                        return au_iter.currentAccessUnitOffset();
                    }
                }
            }
            else if (codec == CodecType::VVC) {
                if (au_type == VVC_AUT_CRA_NUT || au_type == VVC_AUT_RADL_NUT || au_type == VVC_AUT_IDR_N_LP || au_type == VVC_AUT_IDR_W_RADL) {
                    // Found an explicit intra picture.
                    // CRA = Clear Random Access.
                    // RADL = Random Access Decodable Leading.
                    return au_iter.currentAccessUnitOffset();
                }
                else if (au_type == VVC_AUT_AUD_NUT) {
                    // Found an access unit delimiter, analyze it.
                    const VVCAccessUnitDelimiter aud(au_iter.currentAccessUnit(), au_iter.currentAccessUnitSize());
                    if (aud.valid && aud.aud_pic_type == VVC_PIC_TYPE_I) {
                        // Found an access unit delimiter which contains intra slices only.
                        return au_iter.currentAccessUnitOffset();
                    }
                }
            }
        }
    }

    // Process MPEG-1 (ISO 11172-2) and MPEG-2 (ISO 13818-2) video start codes
    else if (IsMPEG2Video(data, size, stream_type)) {
        // Locate all start codes and detect start of Group of Pictures (GOP).
        // The beginning of the PES payload is already a start code prefix in MPEG-1/2.
        while (pl_size > 0) {
            // Look for next start code
            static const uint8_t StartCodePrefix[] = {0x00, 0x00, 0x01};
            const uint8_t* pl_next = LocatePattern(pl_data + 1, pl_size - 1, StartCodePrefix, sizeof(StartCodePrefix));
            if (pl_next == nullptr) {
                // No next start code, current one extends up to the end of the payload.
                pl_next = pl_data + pl_size;
            }
            // The start code is after the start code prefix: 00 00 01 xx
            if (pl_data + 3 < pl_next && (pl_data[3] == PST_SEQUENCE_HEADER || pl_data[3] == PST_GROUP)) {
                // Found a start of GOP. This must be an intra-image in MPEG-1/2.
                return pl_data - data;
            }
            // Move to next start code
            pl_size -= pl_next - pl_data;
            pl_data = pl_next;
        }
    }

    // No intra-image found.
    return NPOS;
}
