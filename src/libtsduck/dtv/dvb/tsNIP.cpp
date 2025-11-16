//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIP.h"


//----------------------------------------------------------------------------
// Get the DVB-NIP signalling IPv4 address.
//----------------------------------------------------------------------------

const ts::IPSocketAddress& ts::NIPSignallingAddress4()
{
    // IPv4: 224.0.23.14, UDP port: 3937, see ETSI TS 103 876, section 8.2.2
    static const IPSocketAddress data {224, 0, 23, 14, NIP_SIGNALLING_PORT};
    return data;
}

const ts::IPSocketAddress& ts::NIPSignallingAddress6()
{
    // IPv6: FF0X:0:0:0:0:0:0:12D, UDP port: 3937, see ETSI TS 103 876, section 8.2.2
    static const IPSocketAddress data {0xFF00, 0, 0, 0, 0, 0, 0, 0x012D, NIP_SIGNALLING_PORT};
    return data;
}


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

    if (hdr_len > 0) {
        addr += hdr_len;
        size -= hdr_len;
        return false;
    }

    valid = true;
    return valid;
}


//----------------------------------------------------------------------------
// Clear the content of a DVB-NIP Actual Carrier Information.
//----------------------------------------------------------------------------

void ts::NIPActualCarrierInformation::clear()
{
    valid = false;
    nip_network_id = nip_carrier_id = nip_link_id = nip_service_id = 0;
    nip_stream_provider_name.clear();
}


//----------------------------------------------------------------------------
// Deserialize a DVB-NIP Actual Carrier Information from a binary area.
//----------------------------------------------------------------------------

bool ts::NIPActualCarrierInformation::deserialize(const uint8_t* addr, size_t size)
{
    clear();
    if (size >= 10 && size >= 10 + addr[9]) {
        nip_network_id = GetUInt16(addr);
        nip_carrier_id = GetUInt16(addr + 2);
        nip_link_id    = GetUInt16(addr + 4);
        nip_service_id = GetUInt16(addr + 6);
        nip_stream_provider_name.assignFromUTF8(reinterpret_cast<const char*>(addr + 10), addr[9]);
        valid = true;
    }
    return valid;
}


//----------------------------------------------------------------------------
// Deserialize a DVB-NIP Actual Carrier Information from a HET_NACI.
//----------------------------------------------------------------------------

bool ts::NIPActualCarrierInformation::deserialize(const LCTHeader& lct)
{
    const auto it = lct.ext.find(HET_NACI);
    if (!lct.valid || it == lct.ext.end()) {
        clear();
        return false;
    }
    else {
        return deserialize(it->second.data(), it->second.size());
    }
}
