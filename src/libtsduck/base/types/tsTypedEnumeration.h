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
//!  Representation of enumeration as typed enum / string pairs.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Enumeration of typed enum/string pairs.
    //! @ingroup cpp
    //!
    //! This class inherits from Enumeration with typed enums.
    //! It is essentially useful for enum classes (ie. not old enum types).
    //!
    template<typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value>::type* = nullptr>
    class TypedEnumeration: public Enumeration
    {
    public:
        //!
        //! A structure used in the constructor of a TypedEnumeration.
        //!
        struct TypedNameValue
        {
            UString name;   //!< Name for the value.
            ENUM    value;  //!< Value for the name.
        };

        //!
        //! Default constructor
        //!
        TypedEnumeration();

        //!
        //! Constructor from a variable list of string/int pairs.
        //!
        //! @param [in] values Variable list of name/value pairs.
        //!
        TypedEnumeration(const std::initializer_list<TypedNameValue> values);

        //!
        //! Add a new enumeration value.
        //!
        //! @param [in] name A string for a symbol.
        //! @param [in] value The corresponding integer value.
        //!
        void add(const UString& name, ENUM value)
        {
            Enumeration::add(name, static_cast<int>(value));
        }

        //!
        //! Get the value from a name, abbreviation allowed.
        //!
        //! @param [in] name The string to search. This string may also
        //! contain an integer value in decimal or hexadecimal representation
        //! in which case this integer value is returned.
        //! @param [in] caseSensitive If false, the search is not case
        //! sensitive and @a name may match an equivalent string with
        //! distinct letter case. If true (the default), an exact match is required.
        //! @return The integer value corresponding to @a name or @c UNKNOWN
        //! if not found or ambiguous, unless @a name can be interpreted as
        //! an integer value. If multiple integer values were registered
        //! with the same name, one of them is returned but which one is
        //! returned is unspecified.
        //!
        ENUM value(const UString& name, bool caseSensitive = true) const
        {
            return static_cast<ENUM>(Enumeration::value(name, caseSensitive));
        }

        //!
        //! Get the name from a value.
        //!
        //! @param [in] value An integer value to search.
        //! @param [in] hexa If true and no name exists for @a value, return the value
        //! as an hexadecimal string with "0x" prefix instead of decimal.
        //! @param [in] hexDigitCount When an hexadecimal value is returned, specify the
        //! minimum number of digits.
        //! @return The corresponding string or a numeric representation of @a value if not found.
        //! If several names were registered with the same value, one of them is returned but which
        //! one is returned is unspecified.
        //!
        UString name(ENUM value, bool hexa = false, size_t hexDigitCount = 0) const
        {
            return Enumeration::name(static_cast<int>(value), hexa, hexDigitCount);
        }

        //!
        //! Get the names from a bit-mask value.
        //! The method is useful only when the integer values in the enumeration are bit-masks.
        //!
        //! @param [in] value A bit-mask, built from integer values in the Enumeration object.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @param [in] hexa If true and no name exists for a value, insert the value
        //! as an hexadecimal string with "0x" prefix instead of decimal.
        //! @param [in] hexDigitCount When an hexadecimal value is returned, specify the
        //! minimum number of digits.
        //! @return The corresponding string containing a list of names. If several names were
        //! registered with the same value, all of them are inserted in the string.
        //!
        UString bitMaskNames(ENUM value, const UString& separator = u", ", bool hexa = false, size_t hexDigitCount = 0) const
        {
            return Enumeration::bitMaskNames(static_cast<int>(value), separator, hexa, hexDigitCount);
        }
    };
}

#include "tsTypedEnumerationTemplate.h"
