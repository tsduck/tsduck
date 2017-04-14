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
//
//  This class holds a "configuration section".
//  A configuration section contains a list of "entries". Each entry has one
//  or more values. A value can be interpreted as a string, integer or boolean.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsStringUtils.h"

namespace ts {

    class TSDUCKDLL ConfigSection
    {
    public:
        // Constructor & destructor
        ConfigSection();
        ~ConfigSection();

        // Reset content of the section
        void reset();

        // Get the number of entries in a section
        size_t entryCount() const {return _entries.size();}

        // Get the names of all entries in a section
        void getEntryNames(StringVector& names) const;

        // Get the number of values in an entry.
        // Return 0 if section or entry does not exist.
        size_t valueCount(const std::string& entry) const;

        // Get a value in an entry.
        // Return defvalue if entry does not exist, or if index is out of range
        const std::string& value(const std::string& entry,
                                 size_t index = 0,
                                 const std::string& defvalue = "") const;

        // Same as above but interpret the content as an integer.
        // Return defvalue if the value cannot be interpreted as an integer.
        template <typename INT>
        INT value(const std::string& entry, size_t index = 0, const INT& defvalue = static_cast<INT>(0)) const;

        // Same as above but interpret the content as a boolean.
        // Return defvalue if the value cannot be interpreted as a boolean.
        // Valid boolean representations are "true"/"yes"/"1" and "false"/"no"/"0".
        bool boolValue(const std::string& entry, size_t index = 0, bool defvalue = false) const;

        // Delete an entry
        void deleteEntry(const std::string& entry) {_entries.erase(entry);}

        // Set the value of an entry.
        void set(const std::string& entry, const std::string& value);
        void set(const std::string& entry, const StringVector& value);
        void set(const std::string& entry, const char* value) {set(entry, std::string(value));}
        void set(const std::string& entry, bool value);
        void set(const std::string& entry, const std::vector <bool>& value);

        template <typename INT>
        void set(const std::string& entry, const INT& value);

        template <typename INT>
        void set(const std::string& entry, const std::vector <INT>& value);

        // Append values in an entry
        void append(const std::string& entry, const std::string& value);
        void append(const std::string& entry, const std::vector <std::string>& value);
        void append(const std::string& entry, const char* value) {append(entry, std::string(value));}
        void append(const std::string& entry, bool value);
        void append(const std::string& entry, const std::vector<bool>& value);

        template <typename INT>
        void append(const std::string& entry, const INT& value);

        template <typename INT>
        void append(const std::string& entry, const std::vector <INT>& value);

        // Set the value of an entry from a string representation:
        //    entryname = value [, value ...]
        void set(const std::string& text);

        // Save the content of a section in a stream
        std::ostream& save(std::ostream& strm) const;

    private:
        // An entry is a vector of strings.
        typedef std::vector <std::string> Entry;

        // Content of a section.
        // The map key is the entry name.
        typedef std::map <std::string, Entry> EntryMap;

        // Private members:
        EntryMap _entries;
    };
}

// Output operator
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::ConfigSection& config)
{
    return config.save (strm);
}

#include "tsConfigSectionTemplate.h"
