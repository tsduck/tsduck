//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Binary Coded Decimal utilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Check if a byte is a valid Binary Coded Decimal (BCD) value.
    //! @param [in] b A byte containing a BCD-encoded value.
    //! @return True if the value is valid BCDn false otherwise.
    //!
    TSDUCKDLL inline bool IsValidBCD(uint8_t b)
    {
        return (b & 0xF0) < 0xA0 && (b & 0x0F) < 0x0A;
    }

    //!
    //! Return the decimal value of a Binary Coded Decimal (BCD) encoded byte.
    //! @param [in] b A byte containing a BCD-encoded value.
    //! @return The decoded value in the range 0 to 99.
    //!
    TSDUCKDLL inline int DecodeBCD(uint8_t b)
    {
        return 10 * (b >> 4) + (b & 0x0F);
    }

    //!
    //! Return a one-byte Binary Coded Decimal (BCD) representation of an integer
    //! @param [in] i The integer to encode (must be in 0..99).
    //! @return One byte containing the BCD-encoded value of @a i.
    //!
    TSDUCKDLL inline uint8_t EncodeBCD(int i)
    {
        return uint8_t(((i / 10) % 10) << 4) | uint8_t(i % 10);
    }

    //!
    //! Return the decimal value of a Binary Coded Decimal (BCD) encoded string.
    //! @param [in] bcd Address of an array of bytes.
    //! @param [in] bcd_count Number of BCD digits (@a bcd_count / 2 bytes).
    //! Note that @a bcd_count can be even.
    //! @param [in] left_justified When true (the default), the first BCD digit starts in
    //! the first half of the first byte. When false and @a bcd_count is odd, the first
    //! BCD digit starts in the second half of the first byte. Ignored when @a bcd_count is even.
    //! @return The decoded integer value.
    //!
    TSDUCKDLL uint32_t DecodeBCD(const uint8_t* bcd, size_t bcd_count, bool left_justified = true);

    //!
    //! Encode a Binary Coded Decimal (BCD) string.
    //! @param [out] bcd Address of an array of bytes.
    //! Its size must be at least (@a bcd_count + 1) / 2 bytes.
    //! @param [in] bcd_count Number of BCD digits.
    //! Note that @a bcd_count can be even.
    //! @param [in] value The value to encode.
    //! @param [in] left_justified When true (the default), the first BCD digit starts in
    //! the first half of the first byte. When false and @a bcd_count is odd, the first
    //! BCD digit starts in the second half of the first byte.
    //! This parameter is ignored when @a bcd_count is even.
    //! @param [in] pad_nibble A value in the range 0..15 to set in the unused nibble when
    //! @a bcd_count is odd. This is the first half of the first byte when @a left_justified
    //! is false. This is the second half of the last byte when @a left_justified is true.
    //! This parameter is ignored when @a bcd_count is even.
    //!
    TSDUCKDLL void EncodeBCD(uint8_t* bcd, size_t bcd_count, uint32_t value, bool left_justified = true, uint8_t pad_nibble = 0);

    //!
    //! Decode a string representation of a variable-length Binary Coded Decimal (BCD) encoded integer.
    //! @param [out] str Returned string representation.
    //! @param [in] bcd Address of an array of bytes.
    //! @param [in] bcd_count Number of BCD digits (@a bcd_count / 2 bytes).
    //! Note that @a bcd_count can be even.
    //! @param [in] decimal Indicates the position of the virtual decimal point
    //! (-1: none, 0: before first digit, 1: after first digit, etc.)
    //! @param [in] left_justified When true (the default), the first BCD digit starts in
    //! the first half of the first byte. When false and @a bcd_count is odd, the first
    //! BCD digit starts in the second half of the first byte. Ignored when @a bcd_count is even.
    //!
    TSDUCKDLL void BCDToString(std::string &str, const uint8_t* bcd, size_t bcd_count, int decimal, bool left_justified = true);
}
