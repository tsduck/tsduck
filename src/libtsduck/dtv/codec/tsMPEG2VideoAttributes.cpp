//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG2VideoAttributes.h"
#include "tsMemory.h"
#include "tsMPEG2.h"
#include "tsNamesFile.h"


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::MPEG2VideoAttributes::toString() const
{
    if (!_is_valid) {
        return UString();
    }

    UString desc(UString::Format(u"%dx%d", {_hsize, _vsize}));
    if (_progressive) {
        desc += u'p';
    }
    if (_interlaced) {
        desc += u'i';
    }
    desc += u", ";
    desc += frameRateName();
    desc += u", ";
    desc += aspectRatioName();
    desc += u", ";
    desc += chromaFormatName();

    return desc;
}


//----------------------------------------------------------------------------
// Get frame rate as a string
//----------------------------------------------------------------------------

ts::UString ts::MPEG2VideoAttributes::frameRateName() const
{
    size_t fr100;

    if (!_is_valid || _fr_div == 0) {
        return UString();
    }
    else if ((fr100 = frameRate100()) % 100 == 0) {
        return UString::Format(u"@%d Hz", {fr100 / 100});
    }
    else {
        return UString::Format(u"@%d.%02d Hz", {fr100 / 100, fr100 % 100});
    }
}


//----------------------------------------------------------------------------
// Get display aspect ratio name.
//----------------------------------------------------------------------------

ts::UString ts::MPEG2VideoAttributes::aspectRatioName() const
{
    return _is_valid ? NameFromDTV(u"mpeg2.aspect_ratio", _ar_code) : UString();
}


//----------------------------------------------------------------------------
// Get chroma format name.
//----------------------------------------------------------------------------

ts::UString ts::MPEG2VideoAttributes::chromaFormatName() const
{
    return _is_valid ? NameFromDTV(u"mpeg2.chroma_format", _cf_code) : UString();
}


//----------------------------------------------------------------------------
// Refresh mode (both can be false if unspecifed)
//----------------------------------------------------------------------------

ts::UString ts::MPEG2VideoAttributes::refreshModeName() const
{
    if (!_is_valid) {
        return UString();
    }
    else if (_progressive) {
        return u"progressive";
    }
    else if (_interlaced) {
        return u"interlaced";
    }
    else {
        return UString();
    }
}


//----------------------------------------------------------------------------
// Extract frame rate fields from frame rate code
//----------------------------------------------------------------------------

size_t ts::MPEG2VideoAttributes::FRNum(uint8_t code)
{
    switch (code) {
        case 1:  return 24000;
        case 2:  return 24;
        case 3:  return 25;
        case 4:  return 30000;
        case 5:  return 30;
        case 6:  return 50;
        case 7:  return 60000;
        case 8:  return 60;
        default: return 0;
    }
}

size_t ts::MPEG2VideoAttributes::FRDiv(uint8_t code)
{
    switch (code) {
        case 1:  return 1001;
        case 2:  return 1;
        case 3:  return 1;
        case 4:  return 1001;
        case 5:  return 1;
        case 6:  return 1;
        case 7:  return 1001;
        case 8:  return 1;
        default: return 1;
    }
}


//----------------------------------------------------------------------------
// Provides a video unit, starting with a 00 00 01 xx start code.
// Return true if the VideoAttributes object becomes valid or
// has new values.
//----------------------------------------------------------------------------

bool ts::MPEG2VideoAttributes::moreBinaryData(const uint8_t* udata, size_t size)
{
    const uint8_t* data = reinterpret_cast <const uint8_t*> (udata);

    // Check start code
    if (size < 4 || data[0] != 0 || data[1] != 0 || data[2] != 1) {
        // Not a valid start code
        return false;
    }
    else if (data[3] == PST_SEQUENCE_HEADER && size >= 12) {
        // First set of value
        _sh_hsize = (GetUInt16(data + 4) >> 4) & 0x0FFF;
        _sh_vsize = GetUInt16(data + 5) & 0x0FFF;
        _sh_ar_code = (data[7] >> 4) & 0x0F;
        _sh_fr_code = data[7] & 0x0F;
        uint32_t fields = GetUInt32 (data + 8);
        _sh_bitrate = (fields >> 14) & 0x0003FFFF;
        _sh_vbv_size = (fields >> 3) & 0x000003FF;

        // Not yet complete, wait for next unit
        _waiting = true;
        return false;
    }
    else if (!_waiting) {
        // Not an interesting unit
        return false;
    }
    else if (data[3] == PST_EXTENSION && size >= 10) {
        // Extension data for MPEG-2
        // Extract fields:
        bool progressive = (data[5] & 0x08) != 0;
        bool interlaced = !progressive;
        uint8_t cf_code = (data[5] >> 1) & 0x03;
        size_t hsize_ext = (GetUInt16(data + 5) >> 7) & 0x0003;
        size_t vsize_ext = (data[6] >> 5) & 0x03;
        uint32_t bitrate_ext = (GetUInt16(data + 6) >> 1) & 0x0FFF;
        size_t vbv_ext = data[8];
        size_t fr_ext_n = (data[9] >> 5) & 0x03;
        size_t fr_ext_d = data[9] & 0x1F;

        // Compute final values:
        size_t hsize = _sh_hsize | (hsize_ext << 12);
        size_t vsize = _sh_vsize | (vsize_ext << 12);
        size_t fr_num = FRNum(_sh_ar_code);
        size_t fr_div = FRDiv(_sh_ar_code);
        if (fr_num == 0) {
            // Not a valid aspect ratio code
            fr_num = size_t(_sh_ar_code) * (fr_ext_n + 1);
            fr_div = fr_ext_d + 1;
        }
        uint32_t bitrate = _sh_bitrate | (bitrate_ext << 18);
        size_t vbv_size = _sh_vbv_size | (vbv_ext << 10);

        // Check modification
        bool changed = !_is_valid || _hsize != hsize || _vsize != vsize ||
            _ar_code != _sh_ar_code || _progressive != progressive ||
            _interlaced != interlaced || _cf_code != cf_code ||
            _fr_num != fr_num || _fr_div != fr_div || _bitrate != bitrate ||
            _vbv_size != vbv_size;

        // Commit final values
        _hsize = hsize;
        _vsize = vsize;
        _ar_code = _sh_ar_code;
        _progressive = progressive;
        _interlaced = interlaced;
        _cf_code = cf_code;
        _fr_num = fr_num;
        _fr_div = fr_div;
        _bitrate = bitrate;
        _vbv_size = vbv_size;

        _waiting = false;
        _is_valid = true;
        return changed;
    }
    else {
        // No extension data after sequence header => MPEG-1
        size_t fr_num = FRNum(_sh_ar_code);
        size_t fr_div = FRDiv(_sh_ar_code);
        bool changed = !_is_valid || _hsize != _sh_hsize || _vsize != _sh_vsize ||
            _ar_code != _sh_ar_code || _progressive || _interlaced || _cf_code != 0 ||
            _fr_num != fr_num || _fr_div != fr_div || _bitrate != _sh_bitrate ||
            _vbv_size != _sh_vbv_size;

        _hsize = _sh_hsize;
        _vsize = _sh_vsize;
        _ar_code = _sh_ar_code;
        _progressive = false;
        _interlaced = false;
        _cf_code = 0;
        _fr_num = fr_num;
        _fr_div = fr_div;
        _bitrate = _sh_bitrate;
        _vbv_size = _sh_bitrate;

        _waiting = false;
        _is_valid = true;
        return changed;
    }
}
