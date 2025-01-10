//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic map of integer to integer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNames.h"
#include "tsjsonValue.h"

namespace ts {
    //!
    //! Generic map of integers, indexed by integer.
    //! @ingroup cpp
    //!
    //! @tparam KEY Integer type for keys.
    //! @tparam VALUE Integer type for values.
    //! @tparam NAMESFILE If not null, names file containing @a KEYNAMESECTION.
    //! @tparam KEYNAMESECTION If not null, name of the section which defines names for the keys in the map.
    //!
    template <typename KEY, typename VALUE, const UChar* NAMESFILE = nullptr, const UChar* KEYNAMESECTION = nullptr>
        requires std::integral<KEY> && std::integral<VALUE>
    class IntegerMap : public std::map<KEY, VALUE>
    {
    public:
        using SuperClass = std::map<KEY, VALUE>;   //!< Identification of the superclass.
        IntegerMap() : SuperClass() {}             //!< Default constructor.

        //!
        //! Accumulate all values from another map.
        //! Non-existent entries in this object are implicitely created.
        //! @param [in] val Value map to add.
        //!
        void accumulate(const IntegerMap& val);

        //!
        //! Format a string for all keys in the map.
        //! Include percentages of values and key name.
        //! @param [in] total Total sum of values. Can be larger than the sum of values in the map.
        //! If zero, the total is computed from the map.
        //! @return A string for all keys in the map.
        //!
        UString toStringKeys(VALUE total = 0) const;

        //!
        //! Build a string of all keys for "normalized" output in TSDuck.
        //! @return "Normalized" string representing the keys in this object.
        //!
        UString toNormalizedKeys() const;

        //!
        //! Display a normalized representation of all keys in the map.
        //! When the value is displayed, it is followed by a column.
        //! @param [in,out] stm Output stream on which the map is displayed.
        //! @param [in] type Type of the entry, as in "type=1,2,7:"
        //! @param [in] ignore_empty If true and the map is empty, display nothing.
        //!
        void addNormalizedKeys(std::ostream& stm, const UChar* type, bool ignore_empty) const;

        //!
        //! Add a list of all keys as a JSON array.
        //! @param [in,out] parent Existing JSON parent.
        //! @param [in] path Path to access or create under @a parent. The created node is a JSON array.
        //! @param [in] ignore_empty If true and the map is empty, do nothing.
        //!
        void addKeys(json::Value& parent, const UString& path, bool ignore_empty) const;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Accumulate all values from another map.
template<typename KEY, typename VALUE, const ts::UChar* NAMESFILE, const ts::UChar* KEYNAMESECTION>
    requires std::integral<KEY> && std::integral<VALUE>
void ts::IntegerMap<KEY,VALUE,NAMESFILE,KEYNAMESECTION>::accumulate(const IntegerMap& val)
{
    for (const auto& it : val) {
        (*this)[it.first] += it.second;
    }
}

// Format a string for all keys in the map.
template<typename KEY, typename VALUE, const ts::UChar* NAMESFILE, const ts::UChar* KEYNAMESECTION>
    requires std::integral<KEY> && std::integral<VALUE>
ts::UString ts::IntegerMap<KEY,VALUE,NAMESFILE,KEYNAMESECTION>::toStringKeys(VALUE total) const
{
    // Adjust total if not present.
    if (total == 0) {
        for (const auto& it : *this) {
            total += it.second;
        }
    }

    // Display percentage if more than one value or not all values in single entry.
    const bool percent = total > 0 && (this->size() > 1 || (this->size() == 1 && this->begin()->second != total));

    // File names to use.
    NamesPtr section;
    if constexpr (KEYNAMESECTION != nullptr) {
        section = Names::GetSection(NAMESFILE, KEYNAMESECTION, false);
    }

    // Format the list.
    UString str;
    const UChar* const sep = u", ";
    for (const auto& it : *this) {
        str.format(u"%d", it.first);
        if (percent || section != nullptr) {
            str.append(u" (");
        }
        if constexpr (KEYNAMESECTION != nullptr) {
            if (section != nullptr) {
                str.append(section->name(it.first, NamesFlags::NAME));
            }
        }
        if (percent && section != nullptr) {
            str.append(u' ');
        }
        if (percent) {
            str.format(u"%.1f%%", (100.0 * double(it.second)) / double(total));
        }
        if (percent || section != nullptr) {
            str.append(u')');
        }
        str.append(sep);
    }

    // Remove final separator if the string is not empty.
    str.removeSuffix(sep);
    return str;
}

// Build a string of all keys for "normalized" output in TSDuck.
template<typename KEY, typename VALUE, const ts::UChar* NAMESFILE, const ts::UChar* KEYNAMESECTION>
    requires std::integral<KEY> && std::integral<VALUE>
ts::UString ts::IntegerMap<KEY,VALUE,NAMESFILE,KEYNAMESECTION>::toNormalizedKeys() const
{
    UString str;
    for (const auto& it : *this) {
        if (!str.empty()) {
            str.append(u',');
        }
        str.format(u"%d", it.first);
    }
    return str;
}

// Display a normalized representation of all keys in the map.
template<typename KEY, typename VALUE, const ts::UChar* NAMESFILE, const ts::UChar* KEYNAMESECTION>
    requires std::integral<KEY> && std::integral<VALUE>
void ts::IntegerMap<KEY,VALUE,NAMESFILE,KEYNAMESECTION>::addNormalizedKeys(std::ostream& stm, const UChar* type, bool ignore_empty) const
{
    if (!ignore_empty || !this->empty()) {
        stm << type << '=' << toNormalizedKeys() << ':';
    }
}

// Add a list of all keys as a JSON array.
template<typename KEY, typename VALUE, const ts::UChar* NAMESFILE, const ts::UChar* KEYNAMESECTION>
    requires std::integral<KEY> && std::integral<VALUE>
void ts::IntegerMap<KEY,VALUE,NAMESFILE,KEYNAMESECTION>::addKeys(json::Value& parent, const UString& path, bool ignore_empty) const
{
    if (!ignore_empty || !this->empty()) {
        json::Value& arr(parent.query(path, true, json::Type::Array));
        for (const auto& it : *this) {
            arr.set(it.first);
        }
    }
}
