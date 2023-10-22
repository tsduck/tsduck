//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAccessUnitIterator.h"
#include "tsPESPacket.h"
#include "tsHEVC.h"
#include "tsVVC.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::AccessUnitIterator::AccessUnitIterator(const uint8_t* data, size_t size, uint8_t stream_type, CodecType default_format) :
    _data(data),
    _data_size(size),
    _valid(PESPacket::HasCommonVideoHeader(data, size)),
    _format(_valid ? default_format : CodecType::UNDEFINED)
{
    // If the area is valid, compute the actual format.
    if (_valid) {
        // Determine encoding from stream type in PMT.
        // If unspecified, keep default format.
        if (StreamTypeIsAVC(stream_type)) {
            _format = CodecType::AVC;
        }
        else if (StreamTypeIsHEVC(stream_type)) {
            _format = CodecType::HEVC;
        }
        else if (StreamTypeIsVVC(stream_type)) {
            _format = CodecType::VVC;
        }
        else if (stream_type != ST_NULL || (_format != CodecType::AVC && _format != CodecType::HEVC && _format != CodecType::VVC)) {
            // This is an explicit but unsupported stream or codec type.
            _format = CodecType::UNDEFINED;
            _valid = false;
        }
    }

    // Search the first access unit.
    reset();
}


//----------------------------------------------------------------------------
// Reset the exploration of the data area at the beginning.
//----------------------------------------------------------------------------

void ts::AccessUnitIterator::reset()
{
    if (_valid) {
        // Point to the beginning of area, before the first access unit.
        // Calling next() will find the first one (if any).
        _nalunit = _data;
        next();
        // Reset NALunit index since we point to the first one.
        _nalunit_index = 0;
    }
}


//----------------------------------------------------------------------------
// Check if the current access unit is a Supplemental Enhancement Information
//----------------------------------------------------------------------------

bool ts::AccessUnitIterator::currentAccessUnitIsSEI() const
{
    // Not all enum values used in switch, intentionally.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum)
    TS_MSC_NOWARNING(4061)

    switch (_format) {
        case CodecType::AVC: return _nalunit_type == AVC_AUT_SEI;
        case CodecType::HEVC: return _nalunit_type == HEVC_AUT_PREFIX_SEI_NUT || _nalunit_type == HEVC_AUT_SUFFIX_SEI_NUT;
        case CodecType::VVC: return _nalunit_type == VVC_AUT_PREFIX_SEI_NUT || _nalunit_type == VVC_AUT_SUFFIX_SEI_NUT;
        default: return false;
    }

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Iterate to the next access unit.
//----------------------------------------------------------------------------

bool ts::AccessUnitIterator::next()
{
    // Cannot iterate on invalid area ar after end of iteration..
    if (!_valid || _nalunit == nullptr) {
        return false;
    }

    // Memory patterns which are used between access units.
    static const uint8_t Zero3[] = {0x00, 0x00, 0x00};
    static const uint8_t StartCodePrefix[] = {0x00, 0x00, 0x01};

    // Remaining size in data area.
    assert(_nalunit >= _data);
    assert(_nalunit <= _data + _data_size);
    size_t remain = _data + _data_size - _nalunit;

    // Preset access unit type to an invalid value.
    // If the video format is undefined, we won't be able to extract a valid one.
    _nalunit_type = AVC_AUT_INVALID;
    _nalunit_size = 0;
    _nalunit_header_size = 0;

    // Locate next access unit: starts with 00 00 01.
    // The start code prefix 00 00 01 is not part of the NALunit.
    // The NALunit starts at the NALunit type byte (see H.264, 7.3.1).
    const uint8_t* const p1 = LocatePattern(_nalunit, remain, StartCodePrefix, sizeof(StartCodePrefix));
    if (p1 == nullptr) {
        // No next access unit.
        _nalunit = nullptr;
        _nalunit_index++;
        return false;
    }

    // Jump to first byte of NALunit.
    remain -= p1 - _nalunit + sizeof(StartCodePrefix);
    _nalunit = p1 + sizeof(StartCodePrefix);

    // Locate end of access unit: ends with 00 00 00, 00 00 01 or end of data.
    const uint8_t* const p2 = LocatePattern(_nalunit, remain, StartCodePrefix, sizeof(StartCodePrefix));
    const uint8_t* const p3 = LocatePattern(_nalunit, remain, Zero3, sizeof(Zero3));
    if (p2 == nullptr && p3 == nullptr) {
        // No 00 00 01, no 00 00 00, the NALunit extends up to the end of data.
        _nalunit_size = remain;
    }
    else if (p2 == nullptr || (p3 != nullptr && p3 < p2)) {
        // NALunit ends at 00 00 00.
        assert(p3 != nullptr);
        _nalunit_size = p3 - _nalunit;
    }
    else {
        // NALunit ends at 00 00 01.
        assert(p2 != nullptr);
        _nalunit_size = p2 - _nalunit;
    }

    // Extract NALunit type.
    if (_format == CodecType::AVC && _nalunit_size >= 1) {
        _nalunit_header_size = 1;
        _nalunit_type = _nalunit[0] & 0x1F;
    }
    else if (_format == CodecType::HEVC && _nalunit_size >= 1) {
        _nalunit_header_size = 2;
        _nalunit_type = (_nalunit[0] >> 1) & 0x3F;
    }
    else if (_format == CodecType::VVC && _nalunit_size >= 2) {
        _nalunit_header_size = 2;
        _nalunit_type = (_nalunit[1] >> 3) & 0x1F;
    }

    // Count NALunits.
    _nalunit_index++;
    return true;
}
