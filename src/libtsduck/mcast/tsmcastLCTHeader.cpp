//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastLCTHeader.h"
#include "tsmcast.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Clear the content of a binary LCT header.
//----------------------------------------------------------------------------

void ts::mcast::LCTHeader::clear()
{
    protocol = FT_UNKNOWN;
    repair_packet = close_session = close_object = false;
    lct_version = psi = codepoint = fec_encoding_id = 0;
    tsi = toi = toi_high = 0;
    cci.clear();
    time.reset();
    cenc.reset();
    tol.reset();
    naci.reset();
    fdt.reset();
    fti.reset();
    ext.clear();
    fpi.clear();
}


//----------------------------------------------------------------------------
// Deserialize a binary LCT header.
//----------------------------------------------------------------------------

bool ts::mcast::LCTHeader::deserialize(const uint8_t*& addr, size_t& size, FileTransport proto)
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
    protocol = proto;
    lct_version = addr[0] >> 4;
    const size_t c = (addr[0] >> 2) & 0x03;
    psi = addr[0] & 0x03;
    const size_t s = addr[1] >> 7;
    const size_t o = (addr[1] >> 5) & 0x03;
    const size_t h = (addr[1] >> 4) & 0x01;
    close_session = (addr[1] & 0x02) != 0;
    close_object = (addr[1] & 0x01) != 0;
    codepoint = addr[3];

    // With ROUTE, FEC repair packets are indicated in PSI (RFC 9223, 2.1).
    repair_packet = proto == FT_ROUTE && (psi & 2) == 0;

    // FEC Encoding ID:
    // - FLUTE: contained in codepoint (RFC 3926, 5.1).
    // - ROUTE source packets: always Compact No-Code Scheme in source packet (RFC 9223, 5.2).
    // - ROUTE FEC repair packets: probably RaptorQ FEC Scheme (RFC 9223, 2.4), to be confirmed.
    if (proto == FT_FLUTE) {
        fec_encoding_id = codepoint;
    }
    else if (proto == FT_ROUTE && repair_packet) {
        fec_encoding_id = FEI_RAPTORQ;
    }
    else {
        fec_encoding_id = FEI_COMPACT_NOCODE;
    }

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

        // Extract extension type and size.
        const uint8_t het = addr[0];
        const uint8_t* headdr = nullptr;
        size_t hel = 0;
        if (het >= HET_MIN_FIXED_SIZE && het <= HET_MAX_FIXED_SIZE) {
            // Fixed size extension.
            headdr = addr + 1;
            hel = 3;
            addr += 4;
            size -= 4;
            hdr_len -= 4;
        }
        else {
            // Variable size extension.
            hel = 4 * size_t(addr[1]);
            if (hel == 0 || hdr_len < hel) {
                break;
            }
            headdr = addr + 2;
            addr += hel;
            size -= hel;
            hdr_len -= hel;
            hel -= 2;
        }

        // Process header.
        switch (het) {
            case HET_TIME:
                if (hel >= 6 && (GetUInt16(headdr) & 0x8000) != 0) {
                    // The "SCT-High" bit is set in the "Use" field (RFC 5651, section 5.2.2).
                    static const Time origin(1900, 1, 1, 0, 0);
                    time = origin + cn::seconds(GetUInt32(headdr + 2));
                }
                break;
            case HET_CENC:
                if (hel >= 1) {
                    cenc = *headdr;
                }
                break;
            case HET_TOL24:
                if (hel >= 3) {
                    tol = GetUInt24(headdr);
                }
                break;
            case HET_TOL48:
                if (hel >= 6) {
                    tol = GetUInt48(headdr);
                }
                break;
            case HET_NACI:
                naci.emplace();
                if (!naci.value().deserialize(headdr, hel)) {
                    naci.reset();
                }
                break;
            case HET_FDT:
                fdt.emplace();
                if (!fdt.value().deserialize(headdr, hel)) {
                    fdt.reset();
                }
                break;
            case HET_FTI:
                fti.emplace();
                if (!fti.value().deserialize(fec_encoding_id, headdr, hel)) {
                    fti.reset();
                }
                break;
            default:
                // Other extension.
                ext[het].copy(headdr, hel);
                break;
        }
    }

    // Check that HDR_LEN matches the header length.
    if (hdr_len > 0) {
        addr += hdr_len;
        size -= hdr_len;
        return false;
    }

    // Decode FEC Payload ID following the header.
    // The FEC Encoding ID is stored in LCT header codepoint (RFC 3926, section 5.1).
    return fpi.deserialize(fec_encoding_id, addr, size);
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::LCTHeader::toString() const
{
    UString str;
    // Fixed part.
    str.format(u"version: %d, psi: %d, cci: %d bytes, tsi: %d (%d bytes), toi: %d (%d bytes), codepoint: %d\n"
               u"    close sess: %s, close obj: %s, unknown extensions: ",
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
    if (time.has_value()) {
        str.format(u"\n    sender time: %s", time.value());
    }
    if (cenc.has_value()) {
        str.format(u"\n    cenc: content encoding: %d", cenc.value());
    }
    if (tol.has_value()) {
        str.format(u"\n    tol: %d", tol.value());
    }
    if (fdt.has_value()) {
        str.format(u"\n    fdt: %s", fdt.value());
    }
    if (fti.has_value()) {
        str.format(u"\n    fti: %s", fti.value());
    }
    if (fpi.valid) {
        str.format(u"\n    fpi: %s", fpi);
    }
    if (naci.has_value()) {
        str.format(u"\n    naci: %s", naci.value());
    }
    return str;
}
