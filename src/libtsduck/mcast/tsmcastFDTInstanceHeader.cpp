//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFDTInstanceHeader.h"


//----------------------------------------------------------------------------
// Clear the content of a structure.
//----------------------------------------------------------------------------

void ts::mcast::FDTInstanceHeader::clear()
{
    flute_version = 0;
    fdt_instance_id = 0;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::FDTInstanceHeader::deserialize(const uint8_t* addr, size_t size)
{
    clear();
    if (addr == nullptr || size < 3) {
        return false;
    }
    const uint32_t value = GetUInt24(addr);
    flute_version = uint8_t(value >> 20);
    fdt_instance_id = value & 0x000FFFFF;
    return true;
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FDTInstanceHeader::toString() const
{
    return UString::Format(u"version: %d, fdt inst id: %d", flute_version, fdt_instance_id);
}
