//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastFECTransmissionInformation.h"
#include "tsmcast.h"


//----------------------------------------------------------------------------
// Clear the content of a structure.
//----------------------------------------------------------------------------

void ts::mcast::FECTransmissionInformation::clear()
{
    fec_encoding_id = 0;
    transfer_length = 0;
    max_source_block_length = 0;
    fec_instance_id = encoding_symbol_length = max_encoding_symbols = 0;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a binary area.
//----------------------------------------------------------------------------

bool ts::mcast::FECTransmissionInformation::deserialize(uint8_t fei, const uint8_t* addr, size_t size)
{
    clear();
    fec_encoding_id = fei;
    if (addr == nullptr | size < 10) {
        return false;
    }
    transfer_length = GetUInt48(addr);
    fec_instance_id = GetUInt16(addr + 6);
    if (fei == FEI_COMPACT_NOCODE || fei == FEI_EXPANDABLE || fei == FEI_SMALL_BLOCK || fei == FEI_COMPACT) {
        if (size < 14) {
            return false;
        }
        encoding_symbol_length = GetUInt16(addr + 8);
        if (fei == FEI_SMALL_BLOCK) {
            max_source_block_length = GetUInt16(addr + 10);
            max_encoding_symbols = GetUInt16(addr + 12);
        }
        else {
            max_source_block_length = GetUInt32(addr + 10);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::mcast::FECTransmissionInformation::toString() const
{
    UString str;
    str.format(u"transfer len: %d, fec inst id: %d", transfer_length, fec_instance_id);
    if (fec_encoding_id == FEI_COMPACT_NOCODE || fec_encoding_id == FEI_EXPANDABLE || fec_encoding_id == FEI_COMPACT) {
        str.format(u", max src blk len: %d", max_source_block_length);
    }
    else if (fec_encoding_id == FEI_SMALL_BLOCK) {
        str.format(u", max src blk len: %d, max num enc sym: %d", max_source_block_length, max_encoding_symbols);
    }
    return str;
}
