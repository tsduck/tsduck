//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsIntegerUtils.h"
#include "tsEnumUtils.h"
#include "tsReport.h"
#include "tsVersionInfo.h"

namespace ts {
    //!
    //! Flags to be used in the formating of names using class Names.
    //! Values can be used as bit-masks.
    //! @ingroup app
    //!
    enum class NamesFlags : uint16_t {
        NAME          = 0x0000,   //!< Name only, no value. This is the default.
        NAME_VALUE    = 0x0001,   //!< Include the value after name: "name (value)".
        VALUE_NAME    = 0x0002,   //!< Same with value first: "value (name)".
        HEXA          = 0x0004,   //!< Value in hexadecimal. This is the default.
        DECIMAL       = 0x0008,   //!< Value in decimal. Both DECIMAL and HEXA can be specified.
        ALTERNATE     = 0x0010,   //!< Display an alternate integer value.
        NAME_OR_VALUE = 0x0020,   //!< Display name if defined or value only if not defined.
        NO_UNKNOWN    = 0x0040,   //!< Ignore unknown values, return an empty string.
        HEX_DEC            = HEXA | DECIMAL,               //!< Value in decimal and hexadecimal.
        HEX_VALUE_NAME     = VALUE_NAME | HEXA,            //!< Value in hexadecimal in first position.
        DEC_VALUE_NAME     = VALUE_NAME | DECIMAL,         //!< Value in decimal in first position.
        HEX_DEC_VALUE_NAME = VALUE_NAME | HEXA | DECIMAL,  //!< Value in decimal and hexadecimal in first position.
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
        //! @param [in] file_name Configuration file name. Typically without directory name.
        //! Without directory, the file is automatically searched in the TSDuck configuration directory.
        //! @param [in] merge_extensions If true, merge the content of names files from TSDuck extensions.
        //! @see Instance(const UString&, bool);
        //! @see Instance(Predefined, bool);
        //!
        NamesFile(const UString& file_name, bool merge_extensions = false);

        //!
        //! Load a names file and merge its content into this instance.
        //! @param [in] file_name Configuration file name.
        //! @return True on success, false on error.
        //!
        bool mergeFile(const UString& file_name);

        //!
        //! Load a configuration file and merge its content into this instance.
        //! @param [in] file_name Configuration file name. Typically without directory name.
        //! Without directory, the file is automatically searched in the TSDuck configuration directory.
        //! @return True on success, false on error.
        //!
        bool mergeConfigurationFile(const UString& file_name);

        //!
        //! Shared pointer to a names file.
        //!
        using NamesFilePtr = std::shared_ptr<NamesFile>;

        //!
        //! Get a common instance of NamesFile for a given configuration file.
        //! The file is loaded once and the instance is created the first time.
        //! When the same file is requested again, the same instance is returned.
        //! @param [in] file_name Configuration file name without directory name.
        //! The file is searched in the TSDuck configuration directory.
        //! @param [in] merge_extensions If true, merge the content of names files
        //! from TSDuck extensions when the file is loaded the first time.
        //! @return A pointer to the NamesFile instance for that file. Never return
        //! a null pointer. In case of error (non existent file for instance), an
        //! empty instance is returned for that file.
        //!
        static NamesFilePtr Instance(const UString& file_name, bool merge_extensions = false)
        {
            return AllInstances::Instance().getFile(file_name, merge_extensions);
        }

        //!
        //! Identifiers for some predefined TSDuck names files.
        //! Using an idenfier is faster than looking for a file name.
        //!
        enum class Predefined {
            DTV    = 0,  //!< All Digital TV definitions (MPEG, DVB, ATSC, ISDB).
            IP     = 1,  //!< Internet protocols definitions.
            OUI    = 2,  //!< IEEE Organizationally Unique Identifiers.
            DEKTEC = 3,  //!< Dektec devices definitions.
            HIDES  = 4,  //!< HiDes modulators definitions.
            COUNT  = 5   //!< Not a real value, just the number of values.
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
        static NamesFilePtr Instance(Predefined index)
        {
            return AllInstances::Instance().getFile(index);
        }

        //!
        //! Delete a common instance of NamesFile for a predefined configuration file.
        //! This should never be necessary unless TSDuck extensions are loaded on the fly.
        //! This never happens since extensions are registered in macro TS_REGISTER_NAMES_FILE.
        //! This is useful in test programs only.
        //! @param [in] index Identifier of the predefined file to get.
        //!
        static void DeleteInstance(Predefined index)
        {
            AllInstances::Instance().unregister(index);
        }

        //!
        //! Largest integer type we manage in the repository of names.
        //!
        using Value = std::uintmax_t;

        //!
        //! Get the complete path of the configuration file from which the names were loaded.
        //! @return The complete path of the configuration file. Empty if does not exist.
        //!
        UString configurationFile() const { return _config_file; }

        //!
        //! Get the number of errors in the configuration file.
        //! @return The number of errors in the configuration file.
        //!
        size_t errorCount() const;

        //!
        //! Check if a name exists in a specified section.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @param [in] value Value to get the name for.
        //! @return True if a name exists for @a value in @a section_name.
        //!
        bool nameExists(const UString& section_name, Value value) const;

        //!
        //! Get a name from a specified section.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @param [in] value Value to get the name for.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        UString nameFromSection(const UString& section_name, Value value, NamesFlags flags = NamesFlags::NAME, Value alternate_value = 0, size_t bits = 0) const;

        //!
        //! Get a name from a specified section, with alternate fallback value.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @param [in] value1 Value to get the name for.
        //! @param [in] value2 Alternate value if no name is found for @a value1.
        //! @param [in] flags Presentation flags.
        //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
        //! @param [in] bits Optional size in bits of the displayed data.
        //! Used in replacement of the "Bits=XX" directive in the .names file.
        //! @return The corresponding name.
        //!
        UString nameFromSectionWithFallback(const UString& section_name, Value value1, Value value2, NamesFlags flags = NamesFlags::NAME, Value alternate_value = 0, size_t bits = 0) const;

        //!
        //! A visitor interface class to be implemented by applications needing ranges of values.
        //!
        class TSDUCKDLL Visitor
        {
            TS_INTERFACE(Visitor);
        public:
            //!
            //! Called for each name/value pair to visit.
            //! @param [in] section_name The name of the section containing the value.
            //! @param [in] value The value.
            //! @param [in] name The name of the value.
            //! @return True to continue visiting other values, false to abort the visit.
            //!
            virtual bool handleNameValue(const UString& section_name, Value value, const UString& name) = 0;
        };

        //!
        //! Get all values in a section.
        //! @param [in,out] visitor An instance of a subclass of Visitor which receives all values.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @return The number of visited values.
        //!
        size_t visitSection(Visitor* visitor, const UString& section_name) const;

        //!
        //! Get all extended values of a specified value in a section.
        //! All sections shall have a nominal width, "Bits=8" for instance. However, when the section has "Extended=true",
        //! "extended" values can be provided. With "Bits=8", the value 0x00AA, 0x01AA, or 0xFFAA, are all extended values
        //! for the base 8-bit value 0xAA, as an example.
        //! @param [in,out] visitor An instance of a subclass of Visitor which receives all extended values for @a value.
        //! @param [in] section_name Name of section to search. Not case-sensitive.
        //! @param [in] value The base value to get extended values for.
        //! @return The number of visited values.
        //!
        size_t visitSection(Visitor* visitor, const UString& section_name, Value value) const;

        //!
        //! Subscribe to all new values which will be merged into the file.
        //! @param [in,out] visitor An instance of a subclass of Visitor which will receive all new values in @a section_name.
        //! @param [in] section_name Name of section to notify. Not case-sensitive. If empty, @a visitor will
        //! be notified of all values in all sections.
        //!
        void subscribe(Visitor* visitor, const UString& section_name = UString());

        //!
        //! Unsubscribe from all new values which will be merged into the file.
        //! @param [in,out] visitor An instance of a subclass of Visitor to unsubscribe.
        //! If null, remove all visitors for @a section_name.
        //! @param [in] section_name Name of section to remove. Not case-sensitive.
        //! If empty, @a visitor will unsubscribed of everything.
        //!
        void unsubscribe(Visitor* visitor, const UString& section_name = UString());

        //!
        //! Format a name using flags.
        //! @param [in] value Value for the name.
        //! @param [in] name Name for the value.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data.
        //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
        //! @return The corresponding name.
        //!
        static UString Formatted(Value value, const UString& name, NamesFlags flags, size_t bits, Value alternate_value = 0);

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
            //! @param [in] file_name Name of the names file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_REGISTER_NAMES_FILE
            //!
            RegisterExtensionFile(const UString& file_name);
        };

        //!
        //! Unregister a previously registered extension.
        //! @param [in] file_name Name of the names file to unregister,
        //!
        static void UnregisterExtensionFile(const UString& file_name);

    private:
        // Description of a configuration entry.
        class ConfigEntry
        {
        public:
            Value   first = 0; // First value in the range.
            Value   last = 0;  // Last value in the range.
            UString name {};   // Associated name.
        };
        using ConfigEntryPtr = std::shared_ptr<ConfigEntry>;

        // Description of a configuration section.
        class ConfigSection
        {
            TS_NOCOPY(ConfigSection);
        public:
            size_t          bits = 0;          // Number of significant bits in values of the type.
            Value           mask = 0;          // Mask to apply to extract the specified bits.
            bool            extended = false;  // Contains extended values, larger than specified bit size.
            UString         inherit {};        // Redirect to this section if value not found.

            // All entries, indexed by full value (first value of the range).
            std::map<Value, ConfigEntryPtr> entries {};

            // All entries, indexed by shortened value ('bits' size) of the first value of the range.
            // Unused when extended = false.
            std::multimap<Value, ConfigEntryPtr> short_entries {};

            // Constructor.
            ConfigSection() = default;

            // Check if a range is free, ie no value is defined in the range.
            bool freeRange(Value first, Value last) const;

            // Add a new entry.
            void addEntry(Value first, Value last, const UString& name);

            // Get the entry for a given value, nullptr if not found.
            ConfigEntryPtr getEntry(Value val) const;

            // Get a name from a value, empty if not found.
            UString getName(Value val) const;
        };
        using ConfigSectionPtr = std::shared_ptr<ConfigSection>;

        using ConfigSectionMap = std::map<UString, ConfigSectionPtr>;
        using VisitorMap = std::multimap<UString, Visitor*>;
        using VisitorBounds = std::pair<VisitorMap::iterator, VisitorMap::iterator>;

        // Names private fields.
        Report&            _log;                 // Error logger.
        const UString      _config_file;         // Configuration file path.
        mutable std::recursive_mutex _mutex {};  // Protect access to all subsequent fields.
        size_t             _config_errors = 0;   // Number of errors in configuration file.
        ConfigSectionMap   _sections {};         // Configuration sections, indexed by section names.
        VisitorMap         _visitors {};         // Visitors, indexed by section names.
        std::set<Visitor*> _full_visitors {};    // Visitors for all sections.

        // Decode a line as "first[-last] = name". Return true on success, false on error.
        bool decodeDefinition(const UString& section_name, const VisitorBounds& visitors, const UString& line, ConfigSectionPtr section);

        // Compute a number of hexa digits.
        static int HexaDigits(size_t bits);

        // Compute the display mask
        static Value DisplayMask(size_t bits);

        // Get the section and name from a value, empty if not found. Section can be null.
        void getName(const UString& section_name, Value value, ConfigSectionPtr& section, UString& name) const;

        // Normalized section name.
        static UString NormalizedSectionName(const UString& section_name) { return section_name.toTrimmed().toLower(); }

        // A singleton which manages all NamesFile instances (thread-safe).
        class AllInstances
        {
            TS_SINGLETON(AllInstances);
        public:
            NamesFilePtr getFile(const UString& fileName, bool mergeExtensions);
            NamesFilePtr getFile(Predefined index);
            void unregister(Predefined index);
            void addExtensionFile(const UString& fileName);
            void removeExtensionFile(const UString& fileName);
            void getExtensionFiles(UStringList& fileNames);

        private:
            std::recursive_mutex            _mutex {};           // Protected access to other fields.
            std::map<UString, NamesFilePtr> _files {};           // Loaded instances by name.
            UStringList                     _ext_file_names {};  // Additional names files.

            // Array of predefined instances, in a direct lookup table.
            class Predef
            {
            public:
                NamesFilePtr     instance {};
                const ts::UChar* name = nullptr;
                bool             merge = false; // merge extension files
            };
            std::array <Predef, size_t(Predefined::COUNT)> _predef {};
        };
    };

    //!
    //! Get a name from a specified section in the DTV names file.
    //! @tparam INT An integer or enum type.
    //! @param [in] section_name Name of section to search. Not case-sensitive.
    //! @param [in] value Value to get the name for.
    //! @param [in] flags Presentation flags.
    //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    template <typename INT> requires ts::int_enum<INT>
    UString NameFromDTV(const UString& section_name, INT value, NamesFlags flags = NamesFlags::NAME, INT alternate_value = static_cast<INT>(0))
    {
        return NamesFile::Instance(NamesFile::Predefined::DTV)->nameFromSection(section_name, NamesFile::Value(value), flags, NamesFile::Value(alternate_value));
    }

    //!
    //! Get a name from a specified section in the DTV names file, with alternate fallback value.
    //! @tparam INT An integer or enum type.
    //! @param [in] section_name Name of section to search. Not case-sensitive.
    //! @param [in] value1 Value to get the name for.
    //! @param [in] value2 Alternate value if no name is found for @a value1.
    //! @param [in] flags Presentation flags.
    //! @param [in] alternate_value Display this integer value if flags ALTERNATE is set.
    //! @return The corresponding name.
    //!
    template <typename INT> requires ts::int_enum<INT>
    UString NameFromDTVWithFallback(const UString& section_name, INT value1, INT value2, NamesFlags flags = NamesFlags::NAME, INT alternate_value = static_cast<INT>(0))
    {
        return NamesFile::Instance(NamesFile::Predefined::DTV)->nameFromSectionWithFallback(section_name, NamesFile::Value(value1), NamesFile::Value(value2), flags, NamesFile::Value(alternate_value));
    }

    //!
    //! Get the name of an OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits.
    //! @param [in] oui Organizationally Unique Identifier), 24 bits.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL inline UString NameFromOUI(uint32_t oui, NamesFlags flags = NamesFlags::NAME)
    {
        return NamesFile::Instance(NamesFile::Predefined::OUI)->nameFromSection(u"OUI", NamesFile::Value(oui), flags);
    }
}

//!
//! @hideinitializer
//! Registration of an extension names file inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a TSDuck extension.
//! @param filename Name of a @c .names file. If the name does not include a directory,
//! the file is searched in the default configuration directories of TSDuck.
//! @see SearchConfigurationFile()
//!
#define TS_REGISTER_NAMES_FILE(filename) \
    TS_LIBCHECK(); \
    static ts::NamesFile::RegisterExtensionFile TS_UNIQUE_NAME(_Registrar)(filename)
