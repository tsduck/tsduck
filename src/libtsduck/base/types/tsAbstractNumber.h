//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Abstract base class for different representations of "numbers".
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"

namespace ts {

    class UString;

    //!
    //! Abstract base class for different representations of "numbers".
    //! @ingroup cpp
    //!
    //! All arithmetic and comparison operators shall be defined by concrete subclasses between
    //! number instances and between a number instance and an integer value, both directions.
    //! @see FixedPoint
    //! @see Fraction
    //! @see Double
    //!
    class TSDUCKDLL AbstractNumber
    {
    public:
        //!
        //! Convert the number to a string object.
        //! @param [in] min_width Minimum width of the returned string.
        //! Padded with @a pad characters if larger than the number of characters in the formatted number.
        //! @param [in] right_justified If true (the default), return a right-justified string.
        //! When false, return a left-justified string. Ignored if @a min_width is lower than
        //! the number of characters in the formatted number.
        //! @param [in] separator Separator character for groups of thousands, a comma by default.
        //! @a CHAR_NULL means no separator.
        //! @param [in] force_sign If true, force a '+' sign for positive values.
        //! @param [in] decimals Precision to use after the decimal point. NPOS means default.
        //! @param [in] force_decimals If true, with positive precision, force a decimal dot
        //! and the number of decimal digits of the precision. By default, skip non
        //! significant decimal digits.
        //! @param [in] decimal_dot The decimal separator, a dot by default.
        //! @param [in] pad The padding character to adjust the width.
        //! @return The formatted string.
        //!
        virtual UString toString(size_t min_width = 0,
                                 bool right_justified = true,
                                 UChar separator = COMMA,
                                 bool force_sign = false,
                                 size_t decimals = NPOS,
                                 bool force_decimals = false,
                                 UChar decimal_dot = FULL_STOP,
                                 UChar pad = SPACE) const = 0;

        //!
        //! Parse a string and interpret it as a number.
        //! The content of this object is updated from the parsed string.
        //! @param [in] str A string to parse, representing this object.
        //! @param [in] separator Separator character for groups of thousands, a comma by default.
        //! @param [in] decimal_dot The decimal separator, a dot by default.
        //! @return True if the @a str is valid, false otherwise.
        //! In case of parsing error, the content of this object is undefined.
        //!
        virtual bool fromString(const UString& str, UChar separator = COMMA, UChar decimal_dot = FULL_STOP) = 0;

        //!
        //! Conversion to a 64-bit signed integer value.
        //! @return The value in integral units. Underflow or overflow rounding is applied when necessary.
        //!
        virtual int64_t toInt64() const = 0;

        //!
        //! Conversion to double value.
        //! @return The value as a double.
        //!
        virtual double toDouble() const = 0;

        //!
        //! Check if the value of the number is within a range of integer value.
        //! @param [in] min Minimum integer value (inclusive).
        //! @param [in] max Maximum integer value (inclusive).
        //! @return True if this number is within the range @a min to @a max, inclusive.
        //!
        virtual bool inRange(int64_t min, int64_t max) const = 0;

        //!
        //! Get a textual description of the values of that type.
        //! This is typically used in help texts.
        //! @return A textual description of the values of that type.
        //!
        virtual UString description() const = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractNumber();

        //!
        //! Pointer to an invalid instance of AbstractNumber.
        //! Its toString() method always return an empty string.
        //!
        static const AbstractNumber* const INVALID;

    protected:
        //!
        //! Static common utility to format a string containing a number with or without decimal part.
        //! @param [in,out] str String to format.
        //! @param [in] min_width Minimum width of the string.
        //! @param [in] right_justified If true (the default), return a right-justified string.
        //! @param [in] separator Separator character for groups of thousands.
        //! @param [in] add_plus_sign If true, force a '+' sign before the number.
        //! @param [in] decimals Precision to use after the decimal point. NPOS not allowed.
        //! @param [in] force_decimals If true, force a decimal dot and the number of decimal digits.
        //! For integer types, use @a force_decimals == true and @a decimals == 0.
        //! @param [in] decimal_dot The decimal separator.
        //! @param [in] pad The padding character to adjust the width.
        //!
        static void Format(UString& str,
                           size_t min_width,
                           bool right_justified,
                           UChar separator,
                           bool add_plus_sign,
                           size_t decimals,
                           bool force_decimals,
                           UChar decimal_dot,
                           UChar pad);

        //!
        //! Static common utility to deformat a string containing a number.
        //! @param [in,out] str String to deformat.
        //! @param [in] separator Separator character for groups of thousands.
        //! @param [in] decimal_dot The decimal separator.
        //!
        static void Deformat(UString& str, UChar separator, UChar decimal_dot);
    };
}
