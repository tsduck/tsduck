//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIPActualCarrierInformation.h"
#include "tsLCTHeader.h"
#include "tsFlute.h"


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
    if (addr != nullptr && size >= 10 && size >= 10 + size_t(addr[9])) {
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


//----------------------------------------------------------------------------
// Comparison operator for use as index in maps.
//----------------------------------------------------------------------------

bool ts::NIPActualCarrierInformation::operator<(const NIPActualCarrierInformation& other) const
{
    if (nip_stream_provider_name != other.nip_stream_provider_name) {
        return nip_stream_provider_name < other.nip_stream_provider_name;
    }
    else {
        return index() < other.index();
    }
}

// Index of all intergers.
uint64_t ts::NIPActualCarrierInformation::index() const
{
    return (uint64_t(nip_network_id) << 48) | (uint64_t(nip_carrier_id) << 32) | (uint64_t(nip_link_id) << 16) | nip_network_id;
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::NIPActualCarrierInformation::toString() const
{
    UString str;
    if (valid) {
        str.format(u"network: %n, carrier: %n, link: %n, service: %n, provider: \"%s\"",
                   nip_network_id, nip_carrier_id, nip_link_id, nip_service_id, nip_stream_provider_name);
    }
    return str;
}
