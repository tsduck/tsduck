//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  AC-3 (DD) and Enhanced-AC-3 (DD+) audio attributes
//
//----------------------------------------------------------------------------

#include "tsAC3Attributes.h"
#include "tsBitStream.h"
#include "tsDecimal.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::AC3Attributes::AC3Attributes() :
    _eac3(false),
    _surround(false),
    _bsid(0),
    _bsmod(0),
    _acmod(0),
    _sampling_freq(0)
{
}


//----------------------------------------------------------------------------
// Provide an audio frame.
//----------------------------------------------------------------------------

bool ts::AC3Attributes::moreBinaryData (const void* vdata, size_t size)
{
    const uint8_t* const data = reinterpret_cast<const uint8_t*> (vdata);

    // Minimum size for AC-3 header, check AC-3 syncword
    if (size < 7 || data[0] != 0x0B || data[1] != 0x77) {
        return false;
    }

    // New attribute values, same as private members
    int  bsid = data[5] >> 3;
    bool eac3 = bsid > 10;
    int  sampling_freq;
    bool surround;
    int  bsmod;
    int  acmod;

    // Sampling frequency
    uint8_t fscod = data[4] >> 6;
    uint8_t fscod2 = eac3 && fscod == 3 ? ((data[4] >> 4) & 0x03) : 3; // 3=reserved
    switch (fscod) {
        case 0:  sampling_freq = 48000; break;  // 48 kHz
        case 1:  sampling_freq = 44100; break;  // 44.1 kHz
        case 2:  sampling_freq = 32000; break;  // 32 kHz
        case 3:
            switch (fscod2) {
                case 0:  sampling_freq = 24000; break;  // 24 kHz
                case 1:  sampling_freq = 22050; break;  // 22.05 kHz
                case 2:  sampling_freq = 16000; break;  // 16 kHz
                default: sampling_freq = 0;     break;  // unknown
            }
            break;
        default: sampling_freq = 0; break; // unknown
    }

    // bsmod is far away in E-AC-3 (in metadata info)
    if (eac3) {
        // Enhanced-AC-3
        bsmod = extractEAC3bsmod (data, size);
        acmod = (data[4] >> 1) & 0x07;
        surround = false;
    }
    else {
        // AC-3
        bsmod = data[5] & 0x07;
        acmod = data[6] >> 5;
        surround = acmod == 0x02 && ((data[6] >> 3) & 0x03) == 2;
    }

    // Check if new values were found
    const bool changed = !_is_valid ||
        _eac3 != eac3 ||
        _surround != surround ||
        _bsid != bsid ||
        _bsmod != bsmod ||
        _acmod != acmod ||
        _sampling_freq != sampling_freq;

    // Commit new values
    if (changed) {
        _is_valid = true;
        _eac3 = eac3;
        _surround = surround;
        _bsid = bsid;
        _bsmod = bsmod;
        _acmod = acmod;
        _sampling_freq = sampling_freq;
    }

    return changed;
}


//----------------------------------------------------------------------------
// Extract 'bsmod' from an Enhanced-AC-3 frame. Return 0 if not found.
//----------------------------------------------------------------------------

int ts::AC3Attributes::extractEAC3bsmod (const uint8_t* data, size_t size)
{
    // Analyse Enhanced-AC-3 bitstream until bsmod is found.
    // See ETSI TS 102 366 V1.1.1, annex E.1, for the dirty details.

    BitStream bs (data, 8 * size);
    bs.skip (16); // syncword
    const uint8_t strmtyp = bs.read<uint8_t> (2);
    bs.skip (14); // substreamid, frmsiz
    const uint8_t fscod = bs.read<uint8_t> (2);
    uint8_t numblkscod;
    if (fscod == 3) {
        bs.skip (2); // fscod2
        numblkscod = 3;
    }
    else {
        numblkscod = bs.read<uint8_t> (2);
    }
    uint8_t number_of_blocks_per_sync_frame = 0;
    switch (numblkscod) {
        case 0:  number_of_blocks_per_sync_frame = 1; break;
        case 1:  number_of_blocks_per_sync_frame = 2; break;
        case 2:  number_of_blocks_per_sync_frame = 3; break;
        case 3:  number_of_blocks_per_sync_frame = 6; break;
        default: assert (false);
    }
    const uint8_t acmod = bs.read<uint8_t> (3);
    const uint8_t lfeon = bs.read<uint8_t> (1);
    bs.skip (10); // bsid, dialnorm
    const uint8_t compre = bs.read<uint8_t> (1);
    if (compre) {
        bs.skip (8); // compr
    }
    if (acmod == 0) {
        bs.skip (5); // dialnorm2
        const uint8_t compr2e = bs.read<uint8_t> (1);
        if (compr2e) {
            bs.skip (8); // compr2
        }
    }
    if (strmtyp == 1) {
        const uint8_t chanmape = bs.read<uint8_t> (1);
        if (chanmape) {
            bs.skip (16); // chanmap
        }
    }
    const uint8_t mixmdate = bs.read<uint8_t> (1);
    if (mixmdate) {
        if (acmod > 2) {
            bs.skip (2); // dmixmod
        }
        if ((acmod & 0x1) && (acmod > 2)) {
            bs.skip (6); // ltrtcmixlev, lorocmixlev
        }
        if (acmod & 0x4) {
            bs.skip (6); // ltrtsurmixlev, lorosurmixlev
        }
        if (lfeon) {
            const uint8_t lfemixlevcode = bs.read<uint8_t> (1);
            if (lfemixlevcode) {
                bs.skip (5); // lfemixlevcod
            }
        }
        if (strmtyp == 0) {
            const uint8_t pgmscle = bs.read<uint8_t> (1);
            if (pgmscle) {
                bs.skip (6); // pgmscl
            }
            if (acmod == 0) {
                const uint8_t pgmscl2e = bs.read<uint8_t> (1);
                if (pgmscl2e) {
                    bs.skip (6); // pgmscl2
                }
            }
            const uint8_t extpgmscle = bs.read<uint8_t> (1);
            if (extpgmscle) {
                bs.skip (6); // extpgmscl
            }
            const uint8_t mixdef = bs.read<uint8_t> (2);
            if (mixdef == 1) {
                bs.skip (5); // premixcompsel, drcsrc, premixcompscl
            }
            else if (mixdef == 2) {
                bs.skip (12); // mixdata
            }
            else if (mixdef == 3) {
                const size_t mixdeflen = bs.read<size_t> (5);
                bs.skip (8 * (mixdeflen + 2)); // mixdata
            }
            if (acmod < 2) {
                const uint8_t paninfoe = bs.read<uint8_t> (1);
                if (paninfoe) {
                    bs.skip (14); // panmean, paninfo
                }
                if (acmod == 0) {
                    const uint8_t paninfo2e = bs.read<uint8_t> (1);
                    if (paninfo2e) {
                        bs.skip (14); // panmean2, paninfo2
                    }
                }
            }
            const uint8_t frmmixcfginfoe = bs.read<uint8_t> (1);
            if (frmmixcfginfoe) {
                if (numblkscod == 0) {
                    bs.skip (5); // blkmixcfginfo[0]
                }
                else {
                    for (uint8_t blk = 0; blk < number_of_blocks_per_sync_frame; blk++) {
                        const uint8_t blkmixcfginfoe = bs.read<uint8_t> (1);
                        if (blkmixcfginfoe) {
                            bs.skip (5); // blkmixcfginfo[blk]
                        }
                    }
                }
            }
        }
    }
    const uint8_t infomdate = bs.read<uint8_t> (1);
    if (infomdate) {
        int bsmod;
        if (bs.remainingBitCount() < 3) {
            bsmod = 0;
        }
        else {
            bsmod = bs.read<int>(3); // bsmod, at last !
        }
        return bsmod;
    }

    return 0; // not found
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::AC3Attributes::toString() const
{
    if (!_is_valid) {
        return "";
    }

    UString desc(_eac3 ? u"E-" : u"");
    desc += u"AC-3";

    UString name(audioCodingDescription());
    if (!name.empty()) {
        desc += u", ";
        desc += name;
    }

    if (_surround) {
        desc += u", Dolby surround";
    }

    if (_sampling_freq != 0) {
        desc += u", @";
        desc += Decimal(_sampling_freq);
        desc += u" Hz";
    }

    name = bitstreamModeDescription();
    if (!name.empty()) {
        desc += u", ";
        desc += name;
    }

    desc += Format(", bsid %d", _bsid);
    return desc;
}


//----------------------------------------------------------------------------
// Bitstream mode ("bsmod", metadata info), see ETSI TS 102 366
//----------------------------------------------------------------------------

ts::UString ts::AC3Attributes::bitstreamModeDescription() const
{
    if (!_is_valid) {
        return UString();
    }
    switch (_bsmod) {
        case 0:  return u"complete main";
        case 1:  return u"music and effects";
        case 2:  return u"visually impaired";
        case 3:  return u"hearing impaired";
        case 4:  return u"dialogue";
        case 5:  return u"commentary";
        case 6:  return u"emergency";
        case 7:  return _acmod == 1 ? u"voice over" : u"karaoke";
        default: return Format("bsmod=%d", _bsmod);
    }
}


//----------------------------------------------------------------------------
// Audio coding mode ("acmod"), see ETSI TS 102 366
//----------------------------------------------------------------------------

ts::UString ts::AC3Attributes::audioCodingDescription() const
{
    if (!_is_valid) {
        return UString();
    }
    switch (_acmod) {
        case 0:  return u"1+1 (Ch1,Ch2)";
        case 1:  return u"mono";
        case 2:  return u"stereo (L,R)";
        case 3:  return u"3/0 (L,C,R)";
        case 4:  return u"2/1 (L,R,S)";
        case 5:  return u"3/1 (L,C,R,S)";
        case 6:  return u"2/2 (L,R,SL,SR)";
        case 7:  return u"3/2 (L,C,R,SL,SR)";
        default: return Format("acmod=%d", _acmod);
    }
}


//----------------------------------------------------------------------------
// Rebuild a component_type for AC-3 descriptors
//----------------------------------------------------------------------------

uint8_t ts::AC3Attributes::componentType() const
{
    // See ETSI 300 468 V1.9.1, annex D.1.

    // - 1 bit: AC-3 vs. Enhanced-AC-3
    uint8_t ctype = _eac3 ? 0x80 : 0x00;

    // - 1 bit: full service flag
    if (_bsmod < 2 || (_bsmod == 7 && _acmod > 1)) {
        // Main audio service
        ctype |= 0x40;
    }

    // - 3 bits: service type flags
    ctype |= (_bsmod & 0x07) << 3;

    // - 3 bits: number of channels flags
    if (_acmod == 0) {
        ctype |= 0x01; // 1+1 mode
    }
    else if (_acmod == 2) { // 2 channels (stereo)
        if (_surround) {
            ctype |= 0x03; // stereo, Dolby surround encoded
        }
        else {
            ctype |= 0x02; // stereo
        }
    }
    else if (_acmod > 2) {
        ctype |= 0x04; // more than 2 channels
    }

    return ctype;
}
