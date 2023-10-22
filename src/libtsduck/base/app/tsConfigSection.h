//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a "configuration section".
//!
//!  A configuration section contains a list of "entries". Each entry has one
//!  or more values. A value can be interpreted as a string, integer or boolean.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a "configuration section".
    //! @ingroup app
    //!
    //! A configuration section contains a list of "entries". Each entry has one
    //! or more values. A value can be interpreted as a string, integer or boolean.
    //!
    class TSDUCKDLL ConfigSection
    {
    public:
        //!
        //! Constructor.
        //!
        ConfigSection() = default;

        //!
        //! Reset content of the section
        //!
        void reset();

        //!
        //! Get the number of entries in the section.
        //! @return The number of entries in the section.
        //!
        size_t entryCount() const
        {
            return _entries.size();
        }

        //!
        //! Get the names of all entries in the section.
        //! @param [out] names The returned names of all entries in the section.
        //!
        void getEntryNames(UStringVector& names) const;

        //!
        //! Get the number of values in an entry.
        //! @param [in] entry Entry name.
        //! @return The number of values in @a entry or 0 if the entry does not exist.
        //!
        size_t valueCount(const UString& entry) const;

        //!
        //! Get a value in an entry.
        //! @param [in] entry Entry name.
        //! @param [in] index Index of the value in the entry.
        //! @param [in] defvalue Default value.
        //! @return The value in the entry or @a defvalue if @a entry does not exist, or if @a index is out of range
        //!
        UString value(const UString& entry, size_t index = 0, const UString& defvalue = UString()) const;

        //!
        //! Get an integer value in an entry.
        //! @tparam INT An integer type.
        //! @param [in] entry Entry name.
        //! @param [in] index Index of the value in the entry.
        //! @param [in] defvalue Default value.
        //! @return The value in the entry or @a defvalue if @a entry does not exist, or if @a index is out of range.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        INT value(const UString& entry, size_t index = 0, const INT& defvalue = static_cast<INT>(0)) const;

        //!
        //! Get a boolean value in an entry.
        //! @param [in] entry Entry name.
        //! @param [in] index Index of the value in the entry.
        //! @param [in] defvalue Default value.
        //! @return The value in the entry or @a defvalue if @a entry does not exist,
        //! or if @a index is out of range, or if the value cannot be interpreted as a boolean.
        //! Valid boolean representations are "true" / "yes" / "1" and "false" / "no" / "0".
        //!
        bool boolValue(const UString& entry, size_t index = 0, bool defvalue = false) const;

        //!
        //! Delete an entry
        //! @param [in] entry Entry name.
        //!
        void deleteEntry(const UString& entry)
        {
            _entries.erase(entry);
        }

        //!
        //! Set the string value of an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        void set(const UString& entry, const UString& val);

        //!
        //! Set the string value of an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        void set(const UString& entry, const UChar* val)
        {
            set(entry, UString(val));
        }

        //!
        //! Set the value of an entry from a vector of strings.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to set.
        //!
        void set(const UString& entry, const UStringVector& val);

        //!
        //! Set the boolean value of an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        void set(const UString& entry, bool val);

        //!
        //! Set the value of an entry from a vector of booleans.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to set.
        //!
        void set(const UString& entry, const std::vector<bool>& val);

        //!
        //! Set the integer value of an entry.
        //! @tparam INT An integer type.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void set(const UString& entry, const INT& val)
        {
            this->set(entry, UString::Decimal(val, 0, true, UString()));
        }

        //!
        //! Set the value of an entry from a vector of integers.
        //! @tparam INT An integer type.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to set.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void set(const UString& entry, const std::vector<INT>& val);

        //!
        //! Append a string value to an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        void append(const UString& entry, const UString& val);

        //!
        //! Append a string value to an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to set.
        //!
        void append(const UString& entry, const UChar* val)
        {
            append(entry, UString(val));
        }

        //!
        //! Append a vector of strings to an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to append.
        //!
        void append(const UString& entry, const UStringVector& val);

        //!
        //! Append a boolean value to an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to append.
        //!
        void append(const UString& entry, bool val);

        //!
        //! Append a vector of booleans to an entry.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to append.
        //!
        void append(const UString& entry, const std::vector<bool>& val);

        //!
        //! Append an integer value to an entry.
        //! @tparam INT An integer type.
        //! @param [in] entry Entry name.
        //! @param [in] val The value to append.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void append(const UString& entry, const INT& val);

        //!
        //! Append a vector of integers to an entry.
        //! @tparam INT An integer type.
        //! @param [in] entry Entry name.
        //! @param [in] val The values to append.
        //!
        template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
        void append(const UString& entry, const std::vector<INT>& val);

        //!
        //! Save the content of the section in a text streams.
        //! @param [in,out] strm An standard stream in output mode.
        //! @return A reference to the @a strm object.
        //!
        std::ostream& save(std::ostream& strm) const;

    private:
        // Content of a section:
        // - The map key is the entry name.
        // - An entry is a vector of strings.
        typedef std::map<UString, UStringVector> EntryMap;

        // Private members:
        EntryMap _entries {};
    };
}

//!
//! Output operator for the class @link ts::ConfigSection @endlink on standard text streams.
//! @param [in,out] strm An standard stream in output mode.
//! @param [in] config A @link ts::ConfigSection @endlink object.
//! @return A reference to the @a strm object.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::ConfigSection& config)
{
    return config.save(strm);
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Get a value in an entry.
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::ConfigSection::value(const UString& entry, size_t index, const INT& defvalue) const
{
    INT result = static_cast<INT>(0);
    if (this->value(entry, index).toInteger(result, UString::DEFAULT_THOUSANDS_SEPARATOR)) {
        return result;
    }
    else {
        return defvalue;
    }
}

// Set the value of an entry as a vector of integers
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::ConfigSection::set(const UString& entry, const std::vector<INT>& val)
{
    this->deleteEntry(entry);
    this->append(entry, val);
}

// Append an integer value in an antry
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::ConfigSection::append(const UString& entry, const INT& val)
{
    this->append(entry, UString::Decimal(val, 0, true, UString()));
}

// Append a vector of integer values in an antry
template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::ConfigSection::append(const UString& entry, const std::vector<INT>& val)
{
    for (size_t i = 0; i < val.size(); ++i) {
        this->append(entry, val[i]);
    }
}
