//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBCD.h"


//----------------------------------------------------------------------------
// Return the decimal value of a BCD-encoded string, on bcd_count digits
//----------------------------------------------------------------------------

uint32_t ts::DecodeBCD(const uint8_t* bcd, size_t bcd_count, bool left_justified)
{
    uint32_t result = 0;

    if (bcd != nullptr) {
        const size_t offset = (bcd_count % 2) == 0 || left_justified ? 0 : 1;
        for (size_t index = 0; index < bcd_count; index++) {
            if (index % 2 == offset) {
                result = 10 * result + (*bcd >> 4);
            }
            else {
                result = 10 * result + (*bcd++ & 0x0F);
            }
        }
    }

    return result;
}


//----------------------------------------------------------------------------
// Encode a Binary Coded Decimal (BCD) string.
//----------------------------------------------------------------------------

void ts::EncodeBCD(uint8_t* bcd, size_t bcd_count, uint32_t value, bool left_justified, uint8_t pad_nibble)
{
    if (bcd_count > 0 && bcd != nullptr) {
        const size_t offset = (bcd_count % 2) == 0 || left_justified ? 0 : 1;

        // Preset first nibble with right-justified even number of digits.
        if (offset == 1) {
            *bcd = uint8_t(pad_nibble << 4);
        }

        // Point to last byte to write.
        bcd += (bcd_count - 1) / 2;

        // Preset last nibble with left-justified even number of digits.
        if ((bcd_count % 2) == 1 && left_justified) {
            *bcd = pad_nibble & 0x0F;
        }

        for (size_t index = bcd_count; index > 0; --index) {
            if (index % 2 == offset) {
                *bcd = (*bcd & 0xF0) | (value % 10);
            }
            else {
                *bcd = (*bcd & 0x0F) | uint8_t((value % 10) << 4);
                bcd--;
            }
            value = value / 10;
        }
    }
}


//----------------------------------------------------------------------------
// Decode a variable-length BCD-encoded integer.
//----------------------------------------------------------------------------

void ts::BCDToString(std::string &str, const uint8_t* bcd, size_t bcd_count, int decimal, bool left_justified)
{
    // Cleanup result string and over-pre-allocate
    str.clear();
    str.reserve(bcd_count + 2);

    // Decode the BCD.
    if (bcd != nullptr) {
        const size_t offset = (bcd_count % 2) == 0 || left_justified ? 0 : 1;
        for (size_t i = 0; i < bcd_count; i++) {
            if (decimal == int(i)) {
                if (str.empty()) {
                    str.push_back('0');
                }
                str.push_back('.');
            }
            const int digit = (i % 2 == offset) ? (*bcd >> 4) : (*bcd++ & 0x0F);
            if (digit != 0 || !str.empty()) {
                str.push_back(char('0' + digit));
            }
        }
    }
}
