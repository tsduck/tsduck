//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAC3Attributes.h"
#include "tsBuffer.h"


//----------------------------------------------------------------------------
// Provide an audio frame.
//----------------------------------------------------------------------------

bool ts::AC3Attributes::moreBinaryData(const uint8_t* vdata, size_t size)
{
    const uint8_t* const data = reinterpret_cast<const uint8_t*>(vdata);

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
        bsmod = extractEAC3bsmod(data, size);
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

int ts::AC3Attributes::extractEAC3bsmod(const uint8_t* data, size_t size)
{
    // Analyse Enhanced-AC-3 bitstream until bsmod is found.
    // See ETSI TS 102 366 V1.1.1, annex E.1, for the dirty details.

    Buffer bs(data, size);
    bs.skipBits(16); // syncword
    const uint8_t strmtyp = bs.getBits<uint8_t>(2);
    bs.skipBits(14); // substreamid, frmsiz
    const uint8_t fscod = bs.getBits<uint8_t>(2);
    uint8_t numblkscod;
    if (fscod == 3) {
        bs.skipBits(2); // fscod2
        numblkscod = 3;
    }
    else {
        numblkscod = bs.getBits<uint8_t>(2);
    }
    uint8_t number_of_blocks_per_sync_frame = 0;
    switch (numblkscod) {
        case 0:  number_of_blocks_per_sync_frame = 1; break;
        case 1:  number_of_blocks_per_sync_frame = 2; break;
        case 2:  number_of_blocks_per_sync_frame = 3; break;
        case 3:  number_of_blocks_per_sync_frame = 6; break;
        default: assert(false);
    }
    const uint8_t acmod = bs.getBits<uint8_t>(3);
    const uint8_t lfeon = bs.getBits<uint8_t>(1);
    bs.skipBits(10); // bsid, dialnorm
    const uint8_t compre = bs.getBits<uint8_t>(1);
    if (compre) {
        bs.skipBits(8); // compr
    }
    if (acmod == 0) {
        bs.skipBits(5); // dialnorm2
        const uint8_t compr2e = bs.getBits<uint8_t>(1);
        if (compr2e) {
            bs.skipBits(8); // compr2
        }
    }
    if (strmtyp == 1) {
        const uint8_t chanmape = bs.getBits<uint8_t>(1);
        if (chanmape) {
            bs.skipBits(16); // chanmap
        }
    }
    const uint8_t mixmdate = bs.getBits<uint8_t>(1);
    if (mixmdate) {
        if (acmod > 2) {
            bs.skipBits(2); // dmixmod
        }
        if ((acmod & 0x1) && (acmod > 2)) {
            bs.skipBits(6); // ltrtcmixlev, lorocmixlev
        }
        if (acmod & 0x4) {
            bs.skipBits(6); // ltrtsurmixlev, lorosurmixlev
        }
        if (lfeon) {
            const uint8_t lfemixlevcode = bs.getBits<uint8_t>(1);
            if (lfemixlevcode) {
                bs.skipBits(5); // lfemixlevcod
            }
        }
        if (strmtyp == 0) {
            const uint8_t pgmscle = bs.getBits<uint8_t>(1);
            if (pgmscle) {
                bs.skipBits(6); // pgmscl
            }
            if (acmod == 0) {
                const uint8_t pgmscl2e = bs.getBits<uint8_t>(1);
                if (pgmscl2e) {
                    bs.skipBits(6); // pgmscl2
                }
            }
            const uint8_t extpgmscle = bs.getBits<uint8_t>(1);
            if (extpgmscle) {
                bs.skipBits(6); // extpgmscl
            }
            const uint8_t mixdef = bs.getBits<uint8_t>(2);
            if (mixdef == 1) {
                bs.skipBits(5); // premixcompsel, drcsrc, premixcompscl
            }
            else if (mixdef == 2) {
                bs.skipBits(12); // mixdata
            }
            else if (mixdef == 3) {
                const size_t mixdeflen = bs.getBits<size_t>(5);
                bs.skipBits(8 * (mixdeflen + 2)); // mixdata
            }
            if (acmod < 2) {
                const uint8_t paninfoe = bs.getBits<uint8_t>(1);
                if (paninfoe) {
                    bs.skipBits(14); // panmean, paninfo
                }
                if (acmod == 0) {
                    const uint8_t paninfo2e = bs.getBits<uint8_t>(1);
                    if (paninfo2e) {
                        bs.skipBits(14); // panmean2, paninfo2
                    }
                }
            }
            const uint8_t frmmixcfginfoe = bs.getBits<uint8_t>(1);
            if (frmmixcfginfoe) {
                if (numblkscod == 0) {
                    bs.skipBits(5); // blkmixcfginfo[0]
                }
                else {
                    for (uint8_t blk = 0; blk < number_of_blocks_per_sync_frame; blk++) {
                        const uint8_t blkmixcfginfoe = bs.getBits<uint8_t>(1);
                        if (blkmixcfginfoe) {
                            bs.skipBits(5); // blkmixcfginfo[blk]
                        }
                    }
                }
            }
        }
    }
    const uint8_t infomdate = bs.getBits<uint8_t>(1);
    if (infomdate) {
        int bsmod;
        if (bs.remainingReadBits() < 3) {
            bsmod = 0;
        }
        else {
            bsmod = bs.getBits<int>(3); // bsmod, at last !
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
        return UString();
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
        desc += UString::Decimal(_sampling_freq);
        desc += u" Hz";
    }

    name = bitstreamModeDescription();
    if (!name.empty()) {
        desc += u", ";
        desc += name;
    }

    desc += UString::Format(u", bsid %d", {_bsid});
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
        default: return UString::Format(u"bsmod=%d", {_bsmod});
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
        default: return UString::Format(u"acmod=%d", {_acmod});
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
