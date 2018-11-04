//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Representation of enumeration as int/string pairs.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    //!
    //! Enumeration of int/string pairs.
    //! @ingroup cpp
    //!
    //! This class is used to manage enumeration values. Here, by enumeration,
    //! we mean an association between strings and integers. The strings are
    //! manipulated as external values (command line parameters, report output, etc.)
    //! and the integers are manipulated as internal values. This class performs
    //! the association between these internal and external values.
    //!
    //! Some interesting features are:
    //! - When provided as input, the string values can be abbreviated up to the
    //!   shortest unambiguous string.
    //! - The strings can be case sensitive or not.
    //! - Several strings may have the same value.
    //!
    class TSDUCKDLL Enumeration
    {
    public:
        //!
        //! This value means "not found".
        //!
        //! It is returned by methods which search an integer value.
        //!
        static const int UNKNOWN;

        //!
        //! A structure used in the constructor of an Enumeration.
        //!
        struct NameValue
        {
            UString name;   //!< Name for the value.
            int     value;  //!< Value for the name.
        };

        //!
        //! Default constructor
        //!
        Enumeration();

        //!
        //! Constructor from a variable list of string/int pairs.
        //!
        //! @param [in] values Variable list of name/value pairs.
        //!
        Enumeration(const std::initializer_list<NameValue> values);

        //!
        //! Virtual destructor
        //!
        virtual ~Enumeration() {}

        //!
        //! Equality operator.
        //!
        //! @param [in] other Another instance to compare with.
        //! @return True if this object has the same content as @a other,
        //! false otherwise.
        //!
        bool operator==(const Enumeration& other) const
        {
            return _map == other._map;
        }

        //!
        //! Unequality operator.
        //!
        //! @param [in] other Another instance to compare with.
        //! @return True if this object does not have the same content as @a other,
        //! false otherwise.
        //!
        bool operator!=(const Enumeration& other) const
        {
            return _map != other._map;
        }

        //!
        //! Get the number of entries in the enumeration.
        //!
        //! @return The number of entries in the enumeration.
        //!
        size_t size() const
        {
            return _map.size();
        }

        //!
        //! Check if the enumeration is empty.
        //!
        //! @return True if the enumeration is empty.
        //!
        bool empty() const
        {
            return _map.empty();
        }

        //!
        //! Add a new enumeration value.
        //!
        //! @param [in] name A string for a symbol.
        //! @param [in] value The corresponding integer value.
        //!
        void add(const UString& name, int value)
        {
            _map.insert(std::make_pair(value, name));
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
        int value(const UString& name, bool caseSensitive = true) const;

        //!
        //! Get the enumeration value from a name, abbreviation allowed.
        //!
        //! @tparam ENUM An enumeration type.
        //! @param [out] e The enumeration value. Unmodified if @a name is not valid.
        //! If multiple integer values were registered with the same name, one of them
        //! is returned but which one is returned is unspecified.
        //! @param [in] name The string to search. This string may also
        //! contain an integer value in decimal or hexadecimal representation
        //! in which case this integer value is returned.
        //! @param [in] caseSensitive If false, the search is not case
        //! sensitive and @a name may match an equivalent string with
        //! distinct letter case. If true (the default), an exact match is required.
        //! @return True on success, false if @a name is not found or ambiguous, unless
        //! @a name can be interpreted as an integer value.
        //!
        template <typename ENUM, typename std::enable_if<std::is_enum<ENUM>::value>::type* = nullptr>
        bool getValue(ENUM& e, const UString& name, bool caseSensitive = true) const
        {
            const int i = value(name, caseSensitive);
            if (i == UNKNOWN) {
                return false;
            }
            else {
                e = static_cast<ENUM>(i);
                return true;
            }
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
        UString name(int value, bool hexa = false, size_t hexDigitCount = 0) const;

        //!
        //! Return a comma-separated list of all names for a list of integer values.
        //!
        //! @tparam CONTAINER A container class of integer values as defined by the C++ Standard Template Library (STL).
        //! @param [in] container A container of integer values.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @return A comma-separated list of the names for the integer values in
        //! @a container. Each value is formatted according to name().
        //!
        template <class CONTAINER>
        UString names(const CONTAINER& container, const UString& separator = u", ") const
        {
            return names<typename CONTAINER::const_iterator>(container.begin(), container.end(), separator);
        }

        //!
        //! Return a comma-separated list of all names for a list of integer values.
        //! The values are accessed through iterators in a container.
        //!
        //! @tparam ITERATOR An iterator class over integer values as defined by the C++ Standard Template Library (STL).
        //! @param [in] begin An iterator pointing to the first value.
        //! @param [in] end An iterator pointing @em after the last value.
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @return A comma-separated list of the names for the integer values in
        //! @a container. Each value is formatted according to name().
        //!
        template <class ITERATOR>
        UString names(ITERATOR begin, ITERATOR end, const UString& separator = u", ") const
        {
            UString res;
            while (begin != end) {
                if (!res.empty()) {
                    res.append(separator);
                }
                res.append(name(*begin));
                ++begin;
            }
            return res;
        }

        //!
        //! Return a comma-separated list of all possible names.
        //!
        //! @param [in] separator The separator to be used between values, a comma by default.
        //! @param [in] inQuote Opening quote for each name.
        //! @param [in] outQuote Closing quote for each name.
        //! @return A comma-separated list of all possible names.
        //!
        UString nameList(const UString& separator = u", ", const UString& inQuote = UString(), const UString& outQuote = UString()) const;

        //!
        //! A constant iterator type for the content of the object.
        //!
        //! An iterator points to a pair of integer and string representing
        //! an element of the enumeration or more formally to a
        //! <code>std::pair <int, UString></code>.
        //!
        //! Sample usage:
        //! @code
        //! ts::Enumeration e (...);
        //!
        //! for (ts::Enumeration::const_iterator it = e.begin(); it != e.end(); ++it) {
        //!     const int value (it->first);           // the int value
        //!     const UString name (it->second);   // the corresponding name
        //!     ....
        //! }
        //! @endcode
        //!
        typedef std::multimap<int, UString>::const_iterator const_iterator;

        //!
        //! Return an iterator pointing to the first element of this object.
        //!
        //! @return An iterator pointing to the first element of this object.
        //!
        const_iterator begin() const
        {
            return _map.begin();
        }

        //!
        //! Return an iterator pointing after the last element of this object.
        //!
        //! @return An iterator pointing after the last element of this object.
        //!
        const_iterator end() const
        {
            return _map.end();
        }

    private:
        typedef std::multimap<int,UString> EnumMap;
        EnumMap _map;
    };
}
