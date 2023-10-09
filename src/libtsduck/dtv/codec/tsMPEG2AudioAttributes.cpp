//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG2AudioAttributes.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Provides an audio frame.
//----------------------------------------------------------------------------

bool ts::MPEG2AudioAttributes::moreBinaryData(const uint8_t* data, size_t size)
{
    // Audio header is 4 bytes, starting with FFF
    uint32_t header = 0;
    if (size < 4 || ((header = GetUInt32 (data)) & 0xFFF00000) != 0xFFF00000) {
        return false;
    }

    // Mask of fields we use in the header
    constexpr uint32_t HEADER_MASK = 0xFFFEFCF0;

    // If content has not changed, nothing to do
    if (_is_valid && (_header & HEADER_MASK) == (header & HEADER_MASK)) {
        return false;
    }

    // Extract fields (ISO 11172-3, section 2.4.1.3)
    uint8_t id = (header >> 19) & 0x01;
    uint8_t layer = (header >> 17) & 0x03;
    uint8_t bitrate_index = (header >> 12) & 0x0F;
    uint8_t sampling_frequency = (header >> 10) & 0x03;
    _mode = (header >> 6) & 0x03;
    _mode_extension = (header >> 4) & 0x03;
    _header = header;
    _is_valid = true;

    // Audio layer
    switch (layer) {
        case 3:  _layer = 1; break;
        case 2:  _layer = 2; break;
        case 1:  _layer = 3; break;
        default: _layer = 0; // reserved
    }

    // Bitrate in kb/s
    _bitrate = 0;
    if (id == 0) {
        // ISO 13818-3 "lower sampling frequencies" extension
        switch (_layer) {
            case 1: // Layer I, lower sampling frequencies extension
                switch (bitrate_index) {
                    case  1: _bitrate = 32; break;
                    case  2: _bitrate = 48; break;
                    case  3: _bitrate = 56; break;
                    case  4: _bitrate = 64; break;
                    case  5: _bitrate = 80; break;
                    case  6: _bitrate = 96; break;
                    case  7: _bitrate = 112; break;
                    case  8: _bitrate = 128; break;
                    case  9: _bitrate = 144; break;
                    case 10: _bitrate = 160; break;
                    case 11: _bitrate = 176; break;
                    case 12: _bitrate = 192; break;
                    case 13: _bitrate = 224; break;
                    case 14: _bitrate = 256; break;
                    default: _bitrate = 0; break; // reserved
                }
                break;
            case 2: // Layer II, lower sampling frequencies extension
            case 3: // Layer III, lower sampling frequencies extension
                switch (bitrate_index) {
                    case  1: _bitrate = 8; break;
                    case  2: _bitrate = 16; break;
                    case  3: _bitrate = 24; break;
                    case  4: _bitrate = 32; break;
                    case  5: _bitrate = 40; break;
                    case  6: _bitrate = 48; break;
                    case  7: _bitrate = 56; break;
                    case  8: _bitrate = 64; break;
                    case  9: _bitrate = 80; break;
                    case 10: _bitrate = 96; break;
                    case 11: _bitrate = 112; break;
                    case 12: _bitrate = 128; break;
                    case 13: _bitrate = 144; break;
                    case 14: _bitrate = 160; break;
                    default: _bitrate = 0; break; // reserved
                }
                break;
            default: // reserved
                _bitrate = 0;
                break;
        }
    }
    else {
        // No sampling extension
        switch (_layer) {
            case 1: // Layer I
                switch (bitrate_index) {
                    case  1: _bitrate = 32; break;
                    case  2: _bitrate = 64; break;
                    case  3: _bitrate = 96; break;
                    case  4: _bitrate = 128; break;
                    case  5: _bitrate = 160; break;
                    case  6: _bitrate = 192; break;
                    case  7: _bitrate = 224; break;
                    case  8: _bitrate = 256; break;
                    case  9: _bitrate = 288; break;
                    case 10: _bitrate = 320; break;
                    case 11: _bitrate = 352; break;
                    case 12: _bitrate = 384; break;
                    case 13: _bitrate = 416; break;
                    case 14: _bitrate = 448; break;
                    default: _bitrate = 0; break; // reserved
                }
                break;
            case 2: // Layer II
                switch (bitrate_index) {
                    case  1: _bitrate = 32; break;
                    case  2: _bitrate = 48; break;
                    case  3: _bitrate = 56; break;
                    case  4: _bitrate = 64; break;
                    case  5: _bitrate = 80; break;
                    case  6: _bitrate = 96; break;
                    case  7: _bitrate = 112; break;
                    case  8: _bitrate = 128; break;
                    case  9: _bitrate = 160; break;
                    case 10: _bitrate = 192; break;
                    case 11: _bitrate = 224; break;
                    case 12: _bitrate = 256; break;
                    case 13: _bitrate = 320; break;
                    case 14: _bitrate = 384; break;
                    default: _bitrate = 0; break; // reserved
                }
                break;
            case 3: // Layer III
                switch (bitrate_index) {
                    case  1: _bitrate = 32; break;
                    case  2: _bitrate = 40; break;
                    case  3: _bitrate = 48; break;
                    case  4: _bitrate = 56; break;
                    case  5: _bitrate = 64; break;
                    case  6: _bitrate = 80; break;
                    case  7: _bitrate = 96; break;
                    case  8: _bitrate = 112; break;
                    case  9: _bitrate = 128; break;
                    case 10: _bitrate = 160; break;
                    case 11: _bitrate = 192; break;
                    case 12: _bitrate = 224; break;
                    case 13: _bitrate = 256; break;
                    case 14: _bitrate = 320; break;
                    default: _bitrate = 0; break; // reserved
                }
                break;
            default: // reserved
                _bitrate = 0;
                break;
        }
    }

    // Sampling frequency
    _sampling_freq = 0;
    if (id == 0) {
        // ISO 13818-3 "lower sampling frequencies" extension
        switch (sampling_frequency) {
            case 0: _sampling_freq = 22050; break;
            case 1: _sampling_freq = 24000; break;
            case 2: _sampling_freq = 16000; break;
            default: _sampling_freq = 0; break; // reserved
        }
    }
    else {
        switch (sampling_frequency) {
            case 0: _sampling_freq = 44100; break;
            case 1: _sampling_freq = 48000; break;
            case 2: _sampling_freq = 32000; break;
            default: _sampling_freq = 0; break; // reserved
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::MPEG2AudioAttributes::toString() const
{
    if (!_is_valid) {
        return UString();
    }

    UString desc(u"Audio ");
    desc += layerName();

    if (_bitrate != 0) {
        desc += u", ";
        desc += _bitrate.toString();
        desc += u" kb/s";
    }

    if (_sampling_freq != 0) {
        desc += u", @";
        desc += UString::Decimal(_sampling_freq);
        desc += u" Hz";
    }

    const UString stereo(stereoDescription());
    if (!stereo.empty()) {
        desc += u", ";
        desc += stereo;
    }

    return desc;
}


//----------------------------------------------------------------------------
// Layer name
//----------------------------------------------------------------------------

ts::UString ts::MPEG2AudioAttributes::layerName() const
{
    if (!_is_valid) {
        return UString();
    }

    switch (_layer) {
        case 1:  return u"layer I";
        case 2:  return u"layer II";
        case 3:  return u"layer III";
        default: return UString::Format(u"layer %d", {_layer});
    }
}


//----------------------------------------------------------------------------
// Mono/stereo modes
//----------------------------------------------------------------------------

ts::UString ts::MPEG2AudioAttributes::stereoDescription() const
{
    if (!_is_valid) {
        return UString();
    }

    switch (_mode) {
        case 0:
            return u"stereo";
        case 1: // joint stereo
            if (_layer == 1 || _layer == 2) {
                switch (_mode_extension) {
                    case 0:  return u"subbands 4-31 in intensity stereo";
                    case 1:  return u"subbands 8-31 in intensity stereo";
                    case 2:  return u"subbands 12-31 in intensity stereo";
                    case 3:  return u"subbands 16-31 in intensity stereo";
                    default: return UString();
                }
            }
            else {
                switch (_mode_extension) {
                    case 0:  return UString();
                    case 1:  return u"intensity stereo";
                    case 2:  return u"ms stereo";
                    case 3:  return u"intensity & ms stereo";
                    default: return UString();
                }
            }
        case 2:
            return u"dual channel";
        case 3:
            return u"single channel";
        default:
            return UString();
    }
}
