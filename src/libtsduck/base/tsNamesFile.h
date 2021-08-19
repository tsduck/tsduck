//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Representation of a ".names" file, containing names for identifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsEnumUtils.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Flags to be used in the formating of names in NamesFile.
    //! Values can be used as bit-masks.
    //! @ingroup app
    //!
    enum class NamesFlags : uint16_t {
        NAME          = 0x0000,   //!< Name only, no value. This is the default.
        VALUE         = 0x0001,   //!< Include the value: "name (value)".
        FIRST         = 0x0002,   //!< Same with value first: "value (name)".
        HEXA          = 0x0004,   //!< Value in hexadecimal. This is the default.
        DECIMAL       = 0x0008,   //!< Value in decimal. Both DECIMAL and HEXA can be specified.
        BOTH          = HEXA | DECIMAL,          //!< Value in decimal and hexadecimal.
        HEXA_FIRST    = FIRST | HEXA,            //!< Value in hexadecimal in first position.
        DECIMAL_FIRST = FIRST | DECIMAL,         //!< Value in decimal in first position.
        BOTH_FIRST    = FIRST | HEXA | DECIMAL,  //!< Value in decimal and hexadecimal in first position.
        ALTERNATE     = 0x0010,                  //!< Display an alternate integer value.
        NAME_OR_VALUE = 0x0020,                  //!< Display name if defined or value only if not defined.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::NamesFlags);

namespace ts {
    //!
    //! Representation of a ".names" file, containing names for identifiers.
    //! All names are loaded from one configuration file.
    //! @ingroup app
    //!
    class TSDUCKDLL NamesFile
    {
        TS_NOBUILD_NOCOPY(NamesFile);
    public:
        //!
        //! Constructor.
        //! @param [in] fileName Configuration file name. Typically without directory name.
        //! Without directory, the file is automatically searched in the TSDuck configuration directory.
        //! @param [in] mergeExtensions If true, merge the content of names files from TSDuck extensions.
        //!
        NamesFile(const UString& fileName, bool mergeExtensions = false);

        //!
        //! Virtual destructor.
        //!
        virtual ~NamesFile();

        //!
        //! Largest integer type we manage in the repository of names.
        //!
        typedef uint64_t Value;

        //!
        //! Get the complete path of the configuration file from which the names were loaded.
        //! @return The complete path of the configuration file. Empty if does not exist.
        //!
        UString configurationFile() const { return _configFile; }

        //!
        //! Get the number of errors in the configuration file.
        //! @return The number of errors in the configuration file.
        //!
        size_t errorCount() const { return _configErrors; }

        //!
        //! Check if a name exists in a specified section.
        //! @param [in] sectionName Name of section to search. Not case-sensitive.
        //! @param [in] value Value to get the name for.
        //! @return True if a name exists for @a value in @a sectionName.
        //!
        bool nameExists(const UString& sectionName, Value value) const;

        //!
        //! Get a name from a specified section.
        //! @param [in] sectionName Name of section to search. Not case-sensitive.
        //! @param [in] value Value to get the name for.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
        //! @return The corresponding name.
        //!
        UString nameFromSection(const UString& sectionName, Value value, NamesFlags flags = NamesFlags::NAME, size_t bits = 0, Value alternateValue = 0) const;

        //!
        //! Get a name from a specified section, with alternate fallback value.
        //! @param [in] sectionName Name of section to search. Not case-sensitive.
        //! @param [in] value1 Value to get the name for.
        //! @param [in] value2 Alternate value if no name is found for @a value1.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
        //! @return The corresponding name.
        //!
        UString nameFromSectionWithFallback(const UString& sectionName, Value value1, Value value2, NamesFlags flags = NamesFlags::NAME, size_t bits = 0, Value alternateValue = 0) const;

        //!
        //! Format a name using flags.
        //! @param [in] value Value for the name.
        //! @param [in] name Name for the value.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
        //! @return The corresponding name.
        //!
        static UString Formatted(Value value, const UString& name, NamesFlags flags, size_t bits, Value alternateValue = 0);

    private:
        // Description of a configuration entry.
        // The first value of the range is the key in a map.
        class ConfigEntry
        {
        public:
            Value   last;   // Last value in the range.
            UString name;   // Associated name.

            ConfigEntry(Value l = 0, const UString& n = UString());
        };

        // Map of configuration entries, indexed by first value of the range.
        typedef std::map<Value, ConfigEntry*> ConfigEntryMap;

        // Description of a configuration section.
        // The name of the section is the key in a map.
        class ConfigSection
        {
        public:
            size_t          bits;     // Number of significant bits in values of the type.
            ConfigEntryMap  entries;  // All entries, indexed by names.

            ConfigSection();
            ~ConfigSection();

            // Check if a range is free, ie no value is defined in the range.
            bool freeRange(Value first, Value last) const;

            // Add a new entry.
            void addEntry(Value first, Value last, const UString& name);

            // Get a name from a value, empty if not found.
            UString getName(Value val) const;
        };

        // Map of configuration sections, indexed by name.
        typedef std::map<UString, ConfigSection*> ConfigSectionMap;

        // Decode a line as "first[-last] = name". Return true on success, false on error.
        bool decodeDefinition(const UString& line, ConfigSection* section);

        // Compute a number of hexa digits.
        static int HexaDigits(size_t bits);

        // Compute the display mask
        static Value DisplayMask(size_t bits);

        // Load a configuration file and merge its content into this instance.
        void loadFile(const UString& fileName);

        // Names private fields.
        Report&          _log;           // Error logger.
        const UString    _configFile;    // Configuration file path.
        size_t           _configErrors;  // Number of errors in configuration file.
        ConfigSectionMap _sections;      // Configuration sections.
    };

    //!
    //! Get a name from a specified section in the DVB names file.
    //! @tparam INT An integer name.
    //! @param [in] sectionName Name of section to search. Not case-sensitive.
    //! @param [in] value Value to get the name for.
    //! @param [in] flags Presentation flags.
    //! @param [in] bits Nominal size in bits of the data, optional.
    //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    //@@@ template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type* = nullptr>
    //@@@ UString NameFromSection(const UString& sectionName, INT value, names::Flags flags = names::NAME, size_t bits = 0, INT alternateValue = 0)
    //@@@ {
    //@@@     return NamesMain::Instance()->nameFromSection(sectionName, Names::Value(value), flags, bits, Names::Value(alternateValue));
    //@@@ }
}
