//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFECPayloadId.h"
#include "tsmcast.h"


//----------------------------------------------------------------------------
// Clear the content of a structure.
//----------------------------------------------------------------------------

void ts::mcast::FECPayloadId::clear()
{
    valid = false;
    fec_encoding_id = 0;
    source_block_number = encoding_symbol_id = 0;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::FECPayloadId::deserialize(uint8_t fei, const uint8_t*& addr, size_t& size)
{
    clear();
    fec_encoding_id = fei;
    if (addr != nullptr && (fei == FEI_COMPACT_NOCODE || fei == FEI_COMPACT) && size >= 4) {
        valid = true;
        source_block_number = GetUInt16(addr);
        encoding_symbol_id = GetUInt16(addr + 2);
        addr += 4;
        size -= 4;
    }
    return valid;
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FECPayloadId::toString() const
{
    UString str;
    if (valid && (fec_encoding_id == FEI_COMPACT_NOCODE || fec_encoding_id == FEI_COMPACT)) {
        str.format(u"sbn: %d, symbol id: %d", source_block_number, encoding_symbol_id);
    }
    return str;
}
