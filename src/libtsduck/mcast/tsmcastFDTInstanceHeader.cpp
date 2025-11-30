//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFDTInstanceHeader.h"
#include "tsmcastLCTHeader.h"
#include "tsmcast.h"


//----------------------------------------------------------------------------
// Clear the content of a structure.
//----------------------------------------------------------------------------

void ts::mcast::FDTInstanceHeader::clear()
{
    valid = false;
    flute_version = 0;
    fdt_instance_id = 0;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::FDTInstanceHeader::deserialize(const uint8_t* addr, size_t size)
{
    clear();
    if (addr != nullptr && size >= 3) {
        valid = true;
        const uint32_t value = GetUInt24(addr);
        flute_version = uint8_t(value >> 20);
        fdt_instance_id = value & 0x000FFFFF;
    }
    return valid;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a HET_FTI LCT header extension.
//----------------------------------------------------------------------------

bool ts::mcast::FDTInstanceHeader::deserialize(const LCTHeader& lct)
{
    const auto it = lct.ext.find(HET_FDT);
    if (!lct.valid || it == lct.ext.end()) {
        clear();
        return false;
    }
    else {
        return deserialize(it->second.data(), it->second.size());
    }
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FDTInstanceHeader::toString() const
{
    UString str;
    if (valid) {
        str.format(u"version: %d, fdt inst id: %d", flute_version, fdt_instance_id);
    }
    return str;
}
