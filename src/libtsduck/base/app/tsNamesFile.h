//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsVersionInfo.h"

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
    //! In an instance of NamesFile, all names are loaded from one configuration file.
    //! @ingroup app
    //!
    class TSDUCKDLL NamesFile
    {
        TS_NOBUILD_NOCOPY(NamesFile);
    public:
        //!
        //! Constructor.
        //! Using this constructor directly is discouraged. Use Instance() instead.
        //! @param [in] fileName Configuration file name. Typically without directory name.
        //! Without directory, the file is automatically searched in the TSDuck configuration directory.
        //! @param [in] mergeExtensions If true, merge the content of names files from TSDuck extensions.
        //! @see Instance(const UString&, bool);
        //! @see Instance(Predefined, bool);
        //!
        NamesFile(const UString& fileName, bool mergeExtensions = false);

        //!
        //! Virtual destructor.
        //!
        virtual ~NamesFile();

        //!
        //! Get a common instance of NamesFile for a given configuration file.
        //! The file is loaded once and the instance is created the first time.
        //! When the same file is requested again, the same instance is returned.
        //! @param [in] fileName Configuration file name without directory name.
        //! The file is searched in the TSDuck configuration directory.
        //! @param [in] mergeExtensions If true, merge the content of names files
        //! from TSDuck extensions when the file is loaded the first time.
        //! @return A pointer to the NamesFile instance for that file. Never return
        //! a null pointer. In case of error (non existent file for instance), an
        //! empty instance is returned for that file.
        //!
        static const NamesFile* Instance(const UString& fileName, bool mergeExtensions = false);

        //!
        //! Identifiers for some predefined TSDuck names files.
        //! Using an idenfier is faster than looking for a file name.
        //!
        enum class Predefined {
            DTV    = 0,  //!< All Digital TV definitions (MPEG, DVB, ATSC, ISDB).
            IP     = 1,  //!< Internet protocols definitions.
            OUI    = 2,  //!< IEEE Organizationally Unique Identifiers.
            DEKTEC = 3,  //!< Dektec devices definitions.
            HIDES  = 4   //!< HiDes modulators definitions.
        };

        //!
        //! Get a common instance of NamesFile for a predefined configuration file.
        //! The file is loaded once and the instance is created the first time.
        //! When the same file is requested again, the same instance is returned.
        //! @param [in] index Identifier of the predefined file to get.
        //! @return A pointer to the NamesFile instance for that file. Never return
        //! a null pointer. In case of error (non existent file for instance), an
        //! empty instance is returned for that file.
        //!
        static const NamesFile* Instance(Predefined index);

        //!
        //! Delete a common instance of NamesFile for a predefined configuration file.
        //! This should never be necessary unless TSDuck extensions are loaded on the fly.
        //! This never happens since extensions are registered in macro TS_REGISTER_NAMES_FILE.
        //! This is useful in test programs only.
        //! @param [in] index Identifier of the predefined file to get.
        //!
        static void DeleteInstance(Predefined index);

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

        //!
        //! A class to register additional names files to merge with the TSDuck names file.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL RegisterExtensionFile
        {
            TS_NOBUILD_NOCOPY(RegisterExtensionFile);
        public:
            //!
            //! Register an additional names file.
            //! This file will be merged with the main names files.
            //! @param [in] filename Name of the names file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_REGISTER_NAMES_FILE
            //!
            RegisterExtensionFile(const UString& filename);
        };

        //!
        //! Unregister a previously registered extension.
        //! @param [in] filename Name of the names file to unregister,
        //!
        static void UnregisterExtensionFile(const UString& filename);

    private:
        // Description of a configuration entry.
        // The first value of the range is the key in a map.
        class ConfigEntry
        {
        public:
            Value   last = 0;  // Last value in the range.
            UString name {};   // Associated name.

            ConfigEntry(Value l = 0, const UString& n = UString()) : last(l), name(n) {}
        };

        // Map of configuration entries, indexed by first value of the range.
        typedef std::map<Value, ConfigEntry*> ConfigEntryMap;

        // Description of a configuration section.
        // The name of the section is the key in a map.
        class ConfigSection
        {
            TS_NOCOPY(ConfigSection);
        public:
            size_t          bits = 0;     // Number of significant bits in values of the type.
            ConfigEntryMap  entries {};   // All entries, indexed by names.
            UString         inherit {};   // Redirect to this section if value not found.

            ConfigSection() = default;
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

        // Get the section and name from a value, empty if not found. Section can be null.
        void getName(const UString& sectionName, Value value, ConfigSection*& section, UString& name) const;

        // Normalized section name.
        static UString NormalizedSectionName(const UString& sectionName) { return sectionName.toTrimmed().toLower(); }

        // Names private fields.
        Report&          _log;               // Error logger.
        const UString    _configFile;        // Configuration file path.
        size_t           _configErrors = 0;  // Number of errors in configuration file.
        ConfigSectionMap _sections {};       // Configuration sections.
    };

    //!
    //! Get a name from a specified section in the DTV names file.
    //! @tparam INT An integer name.
    //! @param [in] sectionName Name of section to search. Not case-sensitive.
    //! @param [in] value Value to get the name for.
    //! @param [in] flags Presentation flags.
    //! @param [in] bits Nominal size in bits of the data, optional.
    //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value, int>::type = 0>
    UString NameFromDTV(const UString& sectionName, INT value, NamesFlags flags = NamesFlags::NAME, size_t bits = 0, INT alternateValue = static_cast<INT>(0))
    {
        return NamesFile::Instance(NamesFile::Predefined::DTV)->nameFromSection(sectionName, NamesFile::Value(value), flags, bits, NamesFile::Value(alternateValue));
    }

    //!
    //! Get a name from a specified section in the DTV names file, with alternate fallback value.
    //! @tparam INT An integer name.
    //! @param [in] sectionName Name of section to search. Not case-sensitive.
    //! @param [in] value1 Value to get the name for.
    //! @param [in] value2 Alternate value if no name is found for @a value1.
    //! @param [in] flags Presentation flags.
    //! @param [in] bits Nominal size in bits of the data, optional.
    //! @param [in] alternateValue Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value, int>::type = 0>
    UString NameFromDTVWithFallback(const UString& sectionName, INT value1, INT value2, NamesFlags flags = NamesFlags::NAME, size_t bits = 0, INT alternateValue = static_cast<INT>(0))
    {
        return NamesFile::Instance(NamesFile::Predefined::DTV)->nameFromSectionWithFallback(sectionName, NamesFile::Value(value1), NamesFile::Value(value2), flags, bits, NamesFile::Value(alternateValue));
    }

    //!
    //! Get the name of an OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits.
    //! @param [in] oui Organizationally Unique Identifier), 24 bits.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString NameFromOUI(uint32_t oui, NamesFlags flags = NamesFlags::NAME);
}

//!
//! @hideinitializer
//! Registration of an extension names file inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a TSDuck extension.
//!
#define TS_REGISTER_NAMES_FILE(filename) \
    TS_LIBCHECK(); \
    static ts::NamesFile::RegisterExtensionFile TS_UNIQUE_NAME(_Registrar)(filename)
