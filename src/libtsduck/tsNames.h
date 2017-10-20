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
//!  Names of various MPEG entities.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsUString.h"
#include "tsCASFamily.h"
#include "tsReportInterface.h"
#include "tsStaticInstance.h"

namespace ts {
    //!
    //! Namespace for functions returning MPEG/DVB names.
    //!
    namespace names {
        //!
        //! Flags to be used in the formating of MPEG/DVB names.
        //! Values can be or'ed.
        //!
        enum Flags {
            NAME          = 0x0000,   //! Name only, no value. This is the default.
            VALUE         = 0x0001,   //! Include the value: "name (value)".
            FIRST         = 0x0002,   //! Same with value first: "value (name)".
            HEXA          = 0x0004,   //! Value in hexadecimal. This is the default.
            DECIMAL       = 0x0008,   //! Value in decimal. Both USE_DECIMAL and USE_HEXA can be specified.
            BOTH          = HEXA | DECIMAL,          //! Value in decimal and hexadecimal.
            HEXA_FIRST    = FIRST | HEXA,            //! Value in hexadecimal in first position.
            DECIMAL_FIRST = FIRST | DECIMAL,         //! Value in decimal in first position.
            BOTH_FIRST    = FIRST | HEXA | DECIMAL,  //! Value in decimal and hexadecimal in first position.
        };

        //!
        //! Name of Table ID.
        //! @param [in] tid Table id.
        //! @param [in] cas CAS family for EMM/ECM table ids.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString TID(uint8_t tid, ts::CASFamily cas = CAS_OTHER, Flags flags = NAME);

        //!
        //! Name of Descriptor ID.
        //! @param [in] did Descriptor ID.
        //! @param [in] pds Private data specified if @a did >= 0x80.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DID(uint8_t did, uint32_t pds = 0, Flags flags = NAME);

        //!
        //! Name of Extended descriptor ID.
        //! @param [in] edid Extended descriptor ID.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString EDID(uint8_t edid, Flags flags = NAME);

        //!
        //! Name of Private Data Specifier.
        //! @param [in] pds Private Data Specifier.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PrivateDataSpecifier(uint32_t pds, Flags flags = NAME);

        //!
        //! Name of Stream type (in PMT).
        //! @param [in] st Stream type (in PMT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString StreamType(uint8_t st, Flags flags = NAME);

        //!
        //! Name of Stream ID (in PES header).
        //! @param [in] sid Stream ID (in PES header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString StreamId(uint8_t sid, Flags flags = NAME);

        //!
        //! Name of PES start code value.
        //! @param [in] code PES start code value.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString PESStartCode(uint8_t code, Flags flags = NAME);

        //!
        //! Name of aspect ratio values (in MPEG-1/2 video sequence header).
        //! @param [in] a Aspect ratio value (in MPEG-1/2 video sequence header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AspectRatio(uint8_t a, Flags flags = NAME);

        //!
        //! Name of Chroma format values (in MPEG-1/2 video sequence header).
        //! @param [in] c Chroma format value (in MPEG-1/2 video sequence header).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ChromaFormat(uint8_t c, Flags flags = NAME);

        //!
        //! Name of AVC (ISO 14496-10, ITU H.264) access unit (aka "NALunit") type.
        //! @param [in] ut AVC access unit type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AVCUnitType(uint8_t ut, Flags flags = NAME);

        //!
        //! Name of AVC (ISO 14496-10, ITU H.264) profile.
        //! @param [in] p AVC profile.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AVCProfile(int p, Flags flags = NAME);

        //!
        //! Name of service type (in Service Descriptor).
        //! @param [in] st Service type (in Service Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ServiceType(uint8_t st, Flags flags = NAME);

        //!
        //! Name of linkage type (in Linkage Descriptor).
        //! @param [in] lt Linkage type (in Linkage Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString LinkageType(uint8_t lt, Flags flags = NAME);

        //!
        //! Name of subtitling type (in Subtitling Descriptor).
        //! @param [in] st Subtitling type (in Subtitling Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString SubtitlingType(uint8_t st, Flags flags = NAME);

        //!
        //! Name of Teletext type (in Teletext Descriptor).
        //! @param [in] tt Teletext type (in Teletext Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString TeletextType(uint8_t tt, Flags flags = NAME);

        //!
        //! Name of Conditional Access System Id (in CA Descriptor).
        //! @param [in] casid Conditional Access System Id (in CA Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString CASId(uint16_t casid, Flags flags = NAME);

        //!
        //! Name of Conditional Access Families.
        //! @param [in] cas CAS family
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString CASFamily(ts::CASFamily cas);

        //!
        //! Name of Running Status (in SDT).
        //! @param [in] rs Running Status (in SDT).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString RunningStatus(uint8_t rs, Flags flags = NAME);

        //!
        //! Name of audio type (in ISO639 Language Descriptor).
        //! @param [in] at Audio type (in ISO639 Language Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AudioType(uint8_t at, Flags flags = NAME);

        //!
        //! Name of Component Type (in Component Descriptor).
        //! @param [in] ct Component Type (in Component Descriptor).
        //! Combination of stream_content (4 bits) and component_type (8 bits).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ComponentType(uint16_t ct, Flags flags = NAME);

        //!
        //! Name of AC-3 Component Type.
        //! @param [in] t AC-3 Component Type.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString AC3ComponentType(uint8_t t, Flags flags = NAME);

        //!
        //! Name of DTS Audio Sample Rate code.
        //! @param [in] c DTS Audio Sample Rate code.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSSampleRateCode(uint8_t c, Flags flags = NAME);

        //!
        //! Name of DTS Audio Bit Rate Code.
        //! @param [in] c DTS Audio Bit Rate Code.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSBitRateCode(uint8_t c, Flags flags = NAME);

        //!
        //! Name of DTS Audio Surround Mode.
        //! @param [in] mode DTS Audio Surround Mode.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSSurroundMode(uint8_t mode, Flags flags = NAME);

        //!
        //! Name of DTS Audio Extended Surround Mode.
        //! @param [in] mode DTS Audio Extended Surround Mode.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DTSExtendedSurroundMode(uint8_t mode, Flags flags = NAME);

        //!
        //! Name of content name (in Content Descriptor).
        //! @param [in] c Content name.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString Content(uint8_t c, Flags flags = NAME);

        //!
        //! Name of scrambling control value in TS header
        //! @param [in] sc Scrambling control value in TS header
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString ScramblingControl(uint8_t sc, Flags flags = NAME);

        //!
        //! Name of Bouquet Id.
        //! @param [in] id Bouquet Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString BouquetId(uint16_t id, Flags flags = NAME);

        //!
        //! Name of Original Network Id.
        //! @param [in] id Original Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString OriginalNetworkId(uint16_t id, Flags flags = NAME);

        //!
        //! Name of Network Id.
        //! @param [in] id Network Id.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString NetworkId(uint16_t id, Flags flags = NAME);

        //!
        //! Name of Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] id Data broadcast id (in Data Broadcast Id Descriptor).
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString DataBroadcastId(uint16_t id, Flags flags = NAME);

        //!
        //! Name of OUI (IEEE-assigned Organizationally Unique Identifier), 24 bits.
        //! @param [in] oui Organizationally Unique Identifier), 24 bits.
        //! @param [in] flags Presentation flags.
        //! @return The corresponding name.
        //!
        TSDUCKDLL UString OUI(uint32_t oui, Flags flags = NAME);
    }

    //!
    //! A repository of names for MPEG/DVB entities.
    //! All names are loaded from configuration files @em tsduck.*.names.
    //!
    class TSDUCKDLL Names
    {
    public:
        //!
        //! Constructor.
        //! @param [in] fileName Configuration file name. Typically without directory name.
        //!
        Names(const std::string& fileName);

        //!
        //! Virtual destructor.
        //!
        virtual ~Names();

        //!
        //! Largest integer type we manage in the repository of names.
        //!
        typedef uint64_t Value;

        //!
        //! Get the complete path of the configuration file from which the names were loaded.
        //! @return The complete path of the configuration file. Empty if does not exist.
        //!
        std::string configurationFile() const
        {
            return _configFile;
        }

        //!
        //! Get the number of errors in the configuration file.
        //! @return The number of errors in the configuration file.
        //!
        size_t errorCount() const
        {
            return _configErrors;
        }

        //!
        //! Get a name from a specified section.
        //! @param [in] sectionName Name of section to search. Not case-sensitive.
        //! @param [in] value Value to get the name for.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @return The corresponding name.
        //!
        UString nameFromSection(const UString& sectionName, Value value, names::Flags flags = names::NAME, size_t bits = 0) const;

        //!
        //! Get a name from a specified section, with alternate fallback value.
        //! @param [in] sectionName Name of section to search. Not case-sensitive.
        //! @param [in] value1 Value to get the name for.
        //! @param [in] value2 Alternate value if no name is found for @a value1.
        //! @param [in] flags Presentation flags.
        //! @param [in] bits Nominal size in bits of the data, optional.
        //! @return The corresponding name.
        //!
        UString nameFromSectionWithFallback(const UString& sectionName, Value value1, Value value2, names::Flags flags = names::NAME, size_t bits = 0) const;

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

            // Add a new entry.
            void addEntry(Value first, Value last, const UString& name);

            // Get a name from a value, empty if not found.
            UString getName(Value val) const;
        };

        // Map of configuration sections, indexed by name.
        typedef std::map<UString, ConfigSection*> ConfigSectionMap;

        // Decode a line as "first[-last] = name". Return true on success, false on error.
        bool decodeDefinition(const UString& line, ConfigSection* section);

        // Format a name.
        UString formatted(Value value, const UString& name, names::Flags flags, size_t bits) const;

        // Compute a number of hexa digits.
        static int HexaDigits(size_t bits);

        // Compute the display mask
        static Value DisplayMask(size_t bits);

        // Names private fields.
        ReportInterface&  _log;           // Error logger.
        const std::string _configFile;    // Configuration file path.
        size_t            _configLines;   // Number of lines in configuration file.
        size_t            _configErrors;  // Number of errors in configuration file.
        ConfigSectionMap  _sections;      // Configuration sections.

        // Inaccessible operations.
        Names() = delete;
        Names(const Names&) = delete;
        Names& operator=(const Names&) = delete;
    };

    //!
    //! An instance of names repository containing all MPEG and DVB identifiers.
    //!
    TS_STATIC_INSTANCE_DECLARATION(ts::Names, TSDUCKDLL, NamesDVB);

    //!
    //! An instance of names repository containing all IEEE-assigned Organizationally Unique Identifiers (OUI).
    //! Since the number of OUI values is very large, they are placed in a separate configuration file.
    //!
    TS_STATIC_INSTANCE_DECLARATION(ts::Names, TSDUCKDLL, NamesOUI);
}

TS_FLAGS_OPERATORS(ts::names::Flags)
