//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFECTransmissionInformation.h"
#include "tsLCTHeader.h"
#include "tsFlute.h"


//----------------------------------------------------------------------------
// Clear the content of a structure.
//----------------------------------------------------------------------------

void ts::FECTransmissionInformation::clear()
{
    valid = false;
    fec_encoding_id = 0;
    transfer_length = 0;
    max_source_block_length = 0;
    fec_instance_id = encoding_symbol_length = max_encoding_symbols = 0;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a binary area.
//----------------------------------------------------------------------------

bool ts::FECTransmissionInformation::deserialize(uint8_t fei, const uint8_t* addr, size_t size)
{
    clear();
    fec_encoding_id = fei;
    if (addr != nullptr && size >= 10) {
        valid = true;
        transfer_length = GetUInt48(addr);
        fec_instance_id = GetUInt16(addr + 6);
        if (fei == FEI_COMPACT_NOCODE || fei == FEI_EXPANDABLE || fei == FEI_SMALL_BLOCK || fei == FEI_COMPACT) {
            valid = size >= 14;
            if (valid) {
                encoding_symbol_length = GetUInt16(addr + 8);
                if (fei == FEI_SMALL_BLOCK) {
                    max_source_block_length = GetUInt16(addr + 10);
                    max_encoding_symbols = GetUInt16(addr + 12);
                }
                else {
                    max_source_block_length = GetUInt32(addr + 10);
                }
            }
        }
    }
    return valid;
}


//----------------------------------------------------------------------------
// Deserialize the structure from a HET_FTI LCT header extension.
//----------------------------------------------------------------------------

bool ts::FECTransmissionInformation::deserialize(const LCTHeader& lct)
{
    const auto it = lct.ext.find(HET_FTI);
    if (!lct.valid || it == lct.ext.end()) {
        clear();
        return false;
    }
    else {
        // The FEC Encoding ID is stored in LCT header codepoint (RFC 3926, section 5.1).
        return deserialize(lct.codepoint, it->second.data(), it->second.size());
    }
}


//----------------------------------------------------------------------------
// Convert to string. Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::FECTransmissionInformation::toString() const
{
    UString str;
    if (valid) {
        str.format(u"transf len: %d, fec inst id: %d", transfer_length, fec_instance_id);
        if (fec_encoding_id == FEI_COMPACT_NOCODE || fec_encoding_id == FEI_EXPANDABLE || fec_encoding_id == FEI_COMPACT) {
            str.format(u", max src blk len: %d", max_source_block_length);
        }
        else if (fec_encoding_id == FEI_SMALL_BLOCK) {
            str.format(u", max src blk len: %d, max num enc sym: %d", max_source_block_length, max_encoding_symbols);
        }
    }
    return str;
}
