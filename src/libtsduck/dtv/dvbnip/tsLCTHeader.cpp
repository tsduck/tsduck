//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLCTHeader.h"
#include "tsFlute.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Clear the content of a binary LCT header.
//----------------------------------------------------------------------------

void ts::LCTHeader::clear()
{
    valid = close_session = close_object = false;
    lct_version = psi = codepoint = 0;
    tsi = toi = toi_high = 0;
    cci.clear();
    ext.clear();
    sender_current_time.clear();
    naci.clear();
    fdt.clear();
    fti.clear();
    fpi.clear();
}


//----------------------------------------------------------------------------
// Deserialize a binary LCT header.
//----------------------------------------------------------------------------

bool ts::LCTHeader::deserialize(const uint8_t*& addr, size_t& size)
{
    clear();
    size_t hdr_len = 0; // in bytes

    if (addr == nullptr) {
        size = 0;
        return false;
    }
    if (size < 4 || size < (hdr_len = 4 * size_t(addr[2]))) {
        addr += size;
        size = 0;
        return false;
    }

    // Decode first 32-bit word.
    lct_version = addr[0] >> 4;
    const size_t c = (addr[0] >> 2) & 0x03;
    psi = addr[0] & 0x03;
    const size_t s = addr[1] >> 7;
    const size_t o = (addr[1] >> 5) & 0x03;
    const size_t h = (addr[1] >> 4) & 0x01;
    close_session = (addr[1] & 0x02) != 0;
    close_object = (addr[1] & 0x01) != 0;
    codepoint = addr[3];

    const size_t cci_length = 4*(c+1);
    tsi_length = 4*s + 2*h;
    toi_length = 4*o + 2*h;

    addr += 4;
    size -= 4;

    // Check if all variable-size fields fit in header.
    if (hdr_len < 4 + cci_length + tsi_length + toi_length) {
        addr += size;
        size = 0;
        return false;
    }

    // Read variable-size fields.
    cci.copy(addr, cci_length);
    GetIntVar(addr + cci_length, tsi_length, tsi);
    if (toi_length <= 8) {
        GetIntVar(addr + cci_length + tsi_length, toi_length, toi);
    }
    else {
        GetIntVar(addr + cci_length + tsi_length, toi_length - 8, toi_high);
        toi = GetUInt64(addr + cci_length + tsi_length + toi_length - 8);
    }

    addr += cci_length + tsi_length + toi_length;
    size -= cci_length + tsi_length + toi_length;
    hdr_len -= 4 + cci_length + tsi_length + toi_length;
    assert(size >= hdr_len);

    // Read header extensions. All extensions are multiple of 32-bit words.
    while (hdr_len >= 4) {
        const uint8_t het = addr[0];
        if (het >= HET_MIN_FIXED_SIZE && het <= HET_MAX_FIXED_SIZE) {
            // Fixed size extension.
            ext[het].copy(addr + 1, 3);
            addr += 4;
            size -= 4;
            hdr_len -= 4;
        }
        else {
            const size_t hel = 4 * size_t(addr[1]);
            if (hdr_len < hel) {
                break;
            }
            ext[het].copy(addr + 2, hel - 2);
            addr += hel;
            size -= hel;
            hdr_len -= hel;
        }
    }

    // Check that HDR_LEN matches the header length.
    if (hdr_len > 0) {
        addr += hdr_len;
        size -= hdr_len;
        return false;
    }

    // Decode optional headers.
    valid = true;
    fti.deserialize(*this);
    fdt.deserialize(*this);
    naci.deserialize(*this);

    // Decode optional HET_TIME header.
    const auto etime = ext.find(HET_TIME);
    if (etime != ext.end() && etime->second.size() >= 6 && (GetUInt16(etime->second.data()) & 0x8000) != 0) {
        // The "SCT-High" bit is set in the "Use" field (RFC 5651, section 5.2.2).
        static const Time origin(1900, 1, 1, 0, 0);
        sender_current_time = origin + cn::seconds(GetUInt32(etime->second.data() + 2));
    }

    // Decode FEC Payload ID following the header.
    // The FEC Encoding ID is stored in LCT header codepoint (RFC 3926, section 5.1).
    valid = fpi.deserialize(codepoint, addr, size);
    return valid;
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::LCTHeader::toString() const
{
    UString str;
    if (valid) {
        // Fixed part.
        str.format(u"version: %d, psi: %d, cci: %d bytes, tsi: %d (%d bytes), toi: %d (%d bytes), codepoint: %d\n"
                    u"    close sess: %s, close obj: %s, extensions: ",
                    lct_version, psi, cci.size(), tsi, tsi_length, toi, toi_length, codepoint, close_session, close_object);

        // List of extensions.
        bool got_ext = false;
        for (const auto& e : ext) {
            if (got_ext) {
                str += u", ";
            }
            got_ext = true;
            str.format(u"%d (%s, %d bytes)", e.first, NameFromSection(u"dtv", u"lct_het", e.first), e.second.size());
        }
        if (!got_ext) {
            str += u"none";
        }
        if (sender_current_time != Time::Epoch) {
            str.format(u"\n    sender time: %s", sender_current_time);
        }
        if (fdt.valid) {
            str.format(u"\n    fdt: %s", fdt);
        }
        if (fti.valid) {
            str.format(u"\n    fti: %s", fti);
        }
        if (fpi.valid) {
            str.format(u"\n    fpi: %s", fpi);
        }
        if (naci.valid) {
            str.format(u"\n    naci: %s", naci);
        }
    }
    return str;
}
