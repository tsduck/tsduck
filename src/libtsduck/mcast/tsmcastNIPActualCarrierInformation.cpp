//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPActualCarrierInformation.h"
#include "tsmcastLCTHeader.h"
#include "tsmcast.h"


//----------------------------------------------------------------------------
// Clear the content of a DVB-NIP Actual Carrier Information.
//----------------------------------------------------------------------------

void ts::mcast::NIPActualCarrierInformation::clear()
{
    valid = false;
    stream_id.clear();
    stream_provider_name.clear();
}


//----------------------------------------------------------------------------
// Deserialize a DVB-NIP Actual Carrier Information from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::NIPActualCarrierInformation::deserialize(const uint8_t* addr, size_t size)
{
    clear();
    if (addr != nullptr && size >= 10 && size >= 10 + size_t(addr[9])) {
        stream_id.network_id = GetUInt16(addr);
        stream_id.carrier_id = GetUInt16(addr + 2);
        stream_id.link_id    = GetUInt16(addr + 4);
        stream_id.service_id = GetUInt16(addr + 6);
        stream_provider_name.assignFromUTF8(reinterpret_cast<const char*>(addr + 10), addr[9]);
        valid = true;
    }
    return valid;
}


//----------------------------------------------------------------------------
// Deserialize a DVB-NIP Actual Carrier Information from a HET_NACI.
//----------------------------------------------------------------------------

bool ts::mcast::NIPActualCarrierInformation::deserialize(const LCTHeader& lct)
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

bool ts::mcast::NIPActualCarrierInformation::operator<(const NIPActualCarrierInformation& other) const
{
    if (stream_provider_name != other.stream_provider_name) {
        return stream_provider_name < other.stream_provider_name;
    }
    else {
        return stream_id < other.stream_id;
    }
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::NIPActualCarrierInformation::toString() const
{
    UString str;
    if (valid) {
        str.format(u"%s, provider: \"%s\"", stream_id, stream_provider_name);
    }
    return str;
}
