//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPActualCarrierInformation.h"


//----------------------------------------------------------------------------
// Clear the content of a DVB-NIP Actual Carrier Information.
//----------------------------------------------------------------------------

void ts::mcast::NIPActualCarrierInformation::clear()
{
    stream_id.clear();
    stream_provider_name.clear();
}


//----------------------------------------------------------------------------
// Deserialize a DVB-NIP Actual Carrier Information from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::NIPActualCarrierInformation::deserialize(const uint8_t* addr, size_t size)
{
    clear();
    if (addr == nullptr || size < 10 || size < 10 + size_t(addr[9])) {
        return false;
    }
    stream_id.network_id = GetUInt16(addr);
    stream_id.carrier_id = GetUInt16(addr + 2);
    stream_id.link_id    = GetUInt16(addr + 4);
    stream_id.service_id = GetUInt16(addr + 6);
    stream_provider_name.assignFromUTF8(reinterpret_cast<const char*>(addr + 10), addr[9]);
    return true;
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
    return UString::Format(u"%s, provider: \"%s\"", stream_id, stream_provider_name);
}
