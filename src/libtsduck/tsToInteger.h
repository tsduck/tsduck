//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Utilities to convert strings to integer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Convert a character representing a multi-base integer digit
    //! into the corresponding integer value.
    //!
    //! Characters '0'..'9' are converted to 0..9.
    //! Characters 'a'..'z' and 'A'..'Z' are converted to 10..35.
    //! This function can be used to convert decimal digits, hexadecimal and
    //! any other base up to base 36.
    //!
    //! @param [in] c A character to convert.
    //! @param [in] base The base of the integer representation, must be in the range 2 to 36.
    //! @param [in] defaultValue The value to return on invalid character.
    //! @return The corresponding integer value or the default value in case of error.
    //!
    TSDUCKDLL int ToIntegerDigit(char c, int base = 10, int defaultValue = -1);

    //!
    //! Convert a string into an integer.
    //!
    //! The input string must contain the representation of
    //! an integer value in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! The ToInteger function decodes integer values of this type.
    //! @param [out] value The returned decoded value.
    //! On error (invalid string), @a value contains what could be decoded
    //! up to the first invalid character.
    //! @param [in] from The C-string to decode, not nul-terminated.
    //! @param [in] length The number of character in the C-string.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <typename INT>
    bool ToInteger(INT& value,
                   const char* from,
                   size_t length,
                   const char* thousandSeparators = "");

    //!
    //! Convert a string into an integer.
    //!
    //! The input string must contain the representation of
    //! an integer value in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! The ToInteger function decodes integer values of this type.
    //! @param [out] value The returned decoded value.
    //! On error (invalid string), @a value contains what could be decoded
    //! up to the first invalid character.
    //! @param [in] from The C-string to decode. Must be nul-terminated.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <typename INT>
    inline bool ToInteger(INT& value,
                         const char* from,
                         const char* thousandSeparators = "")
    {
        // Flawfinder: ignore: strlen()
        return from != 0 && ToInteger<INT>(value, from, ::strlen(from), thousandSeparators);
    }

    //!
    //! Convert a string into an integer.
    //!
    //! The input string must contain the representation of
    //! an integer value in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! @tparam INT An integer type, any size, signed or unsigned.
    //! The ToInteger function decodes integer values of this type.
    //! @param [out] value The returned decoded value.
    //! On error (invalid string), @a value contains what could be decoded
    //! up to the first invalid character.
    //! @param [in] from The string to decode.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <typename INT>
    inline bool ToInteger(INT& value,
                          const std::string& from,
                          const char* thousandSeparators = "")
    {
        return ToInteger<INT>(value, from.c_str(), from.length(), thousandSeparators);
    }

    //!
    //! Convert a string containing a list of integers into a container of integers.
    //!
    //! The input string must contain the representation of
    //! integer values in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! The various integer values in the string are separated using
    //! list delimiters.
    //!
    //! @tparam CONTAINER A container class of any integer type as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container The returned decoded values. The previous content
    //! of the container is discarded. The integer values are added in the
    //! container in the order of the decoded string.
    //! On error (invalid string), @a container contains what could be decoded
    //! up to the first invalid character.
    //! @param [in] from The C-string to decode, not nul-terminated.
    //! @param [in] length The number of character in the C-string.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @param [in] listSeparators A string of characters which
    //! are interpreted as list separators. Distinct integer values
    //! must be separated by one or more of these separators.
    //! <i>Any character</i> from the @a listSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the list separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <class CONTAINER>
    bool ToIntegers(CONTAINER& container,
                    const char* from,
                    size_t length,
                    const char* thousandSeparators = "",
                    const char* listSeparators = ", ");

    //!
    //! Convert a string containing a list of integers into a container of integers.
    //!
    //! The input string must contain the representation of
    //! integer values in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! The various integer values in the string are separated using
    //! list delimiters.
    //!
    //! @tparam CONTAINER A container class of any integer type as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container The returned decoded values. The previous content
    //! of the container is discarded. The integer values are added in the
    //! container in the order of the decoded string.
    //! On error (invalid string), @a container contains what could be decoded
    //! up to the first invalid character.
    //! @param [in] from The C-string to decode. Must be nul-terminated.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @param [in] listSeparators A string of characters which
    //! are interpreted as list separators. Distinct integer values
    //! must be separated by one or more of these separators.
    //! <i>Any character</i> from the @a listSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the list separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <class CONTAINER>
    inline bool ToIntegers(CONTAINER& container,
                           const char* from,
                           const char* thousandSeparators = "",
                           const char* listSeparators = ", ")
    {
        // Flawfinder: ignore: strlen()
        return from != 0 && ToIntegers<CONTAINER>(container, from, ::strlen(from), thousandSeparators, listSeparators);
    }

    //!
    //! Convert a string containing a list of integers into a container of integers.
    //!
    //! The input string must contain the representation of
    //! integer values in decimal or hexadecimal (prefix @c 0x).
    //! Hexadecimal values are case-insensitive, including the @c 0x prefix.
    //! Leading and trailing spaces are ignored.
    //! Optional thousands separators are ignored.
    //!
    //! The various integer values in the string are separated using
    //! list delimiters.
    //!
    //! @tparam CONTAINER A container class of any integer type as defined by the
    //! C++ Standard Template Library (STL).
    //! @param [out] container The returned decoded values. The previous content
    //! of the container is discarded. The integer values are added in the
    //! container in the order of the decoded string.
    //! @param [in] from The string to decode.
    //! @param [in] thousandSeparators A string of characters which
    //! are interpreted as thousands separators and are ignored.
    //! <i>Any character</i> from the @a thousandSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the optional thousands separators may have one character only.
    //! @param [in] listSeparators A string of characters which
    //! are interpreted as list separators. Distinct integer values
    //! must be separated by one or more of these separators.
    //! <i>Any character</i> from the @a listSeparators string
    //! is interpreted as a separator. Note that this implies that
    //! the list separators may have one character only.
    //! @return True on success, false on error (invalid string).
    //!
    template <class CONTAINER>
    inline bool ToIntegers(CONTAINER& container,
                           const std::string& from,
                           const char* thousandSeparators = "",
                           const char* listSeparators = ", ")
    {
        return ToIntegers<CONTAINER>(container, from.c_str(), from.length(), thousandSeparators, listSeparators);
    }
}

#include "tsToIntegerTemplate.h"
