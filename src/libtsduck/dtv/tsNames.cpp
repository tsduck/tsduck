//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsNames.h"
#include "tsMPEG.h"
#include "tsDuckContext.h"
#include "tsSysUtils.h"
#include "tsFatal.h"
#include "tsCerrReport.h"
#include "tsPSIRepository.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Configuration instances.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::NamesMain);
ts::NamesMain::NamesMain() : Names(u"tsduck.names", true) {}
ts::NamesMain::~NamesMain() {}

TS_DEFINE_SINGLETON(ts::NamesOUI);
ts::NamesOUI::NamesOUI() : Names(u"tsduck.oui.names") {}
ts::NamesOUI::~NamesOUI() {}


//----------------------------------------------------------------------------
// Tables ids: specific standards processing
//----------------------------------------------------------------------------

ts::UString ts::names::TID(const DuckContext& duck, uint8_t tid, uint16_t cas, Flags flags)
{
    // Where to search table ids.
    const Names* const repo = NamesMain::Instance();
    const UString section(u"TableId");

    // Check without standard, then with all known standards in TSDuck context.
    // In all cases, use version with CAS first, then without CAS.
    // Return the first name which is found.
    // If no name is found in the list of supported standards but some with
    // other standards, use the first one that was found.

    const Names::Value casMask = Names::Value(CASFamilyOf(cas)) << 8;
    Names::Value finalValue = Names::Value(tid);

    if (repo->nameExists(section, finalValue | casMask)) {
        // Found without standard, with CAS.
        finalValue |= casMask;
    }
    else if (repo->nameExists(section, finalValue)) {
        // Found without standard, without CAS. Nothing to do. Keep this value.
    }
    else {
        // Loop on all possible standards.
        bool foundOnce = false;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            // TID value with mask for this standard:
            const Names::Value value = Names::Value(tid) | (Names::Value(mask) << 16);
            // Check if this standard is currently in TSDuck context.
            const bool supportedStandard = (duck.standards() & mask) != Standards::NONE;
            // Lookup name only if supported standard or no previous standard was found.
            if (!foundOnce || supportedStandard) {
                bool foundHere = repo->nameExists(section, value | casMask);
                if (foundHere) {
                    // Found with that standard, with CAS.
                    finalValue = value | casMask;
                    foundOnce = true;
                }
                else if (repo->nameExists(section, value)) {
                    // Found with that standard, without CAS.
                    finalValue = value;
                    foundHere = foundOnce = true;
                }
                if (foundHere && supportedStandard) {
                    break;
                }
            }
        }
    }

    // Return the name for best matched value.
    return repo->nameFromSection(section, finalValue, flags, 8);
}


//----------------------------------------------------------------------------
// Descriptor ids: specific processing for table-specific descriptors.
//----------------------------------------------------------------------------

bool ts::names::HasTableSpecificName(uint8_t did, uint8_t tid)
{
    return tid != TID_NULL &&
        did < 0x80 &&
        NamesMain::Instance()->nameExists(u"DescriptorId", (Names::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | Names::Value(did));
}

ts::UString ts::names::DID(uint8_t did, uint32_t pds, uint8_t tid, Flags flags)
{
    if (did >= 0x80 && pds != 0 && pds != PDS_NULL) {
        // If this is a private descriptor, only consider the private value.
        // Do not fallback because the same value with PDS == 0 can be different.
        return NamesMain::Instance()->nameFromSection(u"DescriptorId", (Names::Value(pds) << 8) | Names::Value(did), flags, 8);
    }
    else if (tid != 0xFF) {
        // Could be a table-specific descriptor.
        const Names::Value fullValue = (Names::Value(tid) << 40) | TS_UCONST64(0x000000FFFFFFFF00) | Names::Value(did);
        return NamesMain::Instance()->nameFromSectionWithFallback(u"DescriptorId", fullValue, Names::Value(did), flags, 8);
    }
    else {
        return NamesMain::Instance()->nameFromSection(u"DescriptorId", Names::Value(did), flags, 8);
    }
}

//----------------------------------------------------------------------------
// Public functions returning names.
//----------------------------------------------------------------------------

ts::UString ts::names::EDID(uint8_t edid, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DVBExtendedDescriptorId", Names::Value(edid), flags, 8);
}

ts::UString ts::names::StreamType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"StreamType", Names::Value(type), flags, 8);
}

ts::UString ts::names::Content(uint8_t x, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"ContentId", Names::Value(x), flags, 8);
}

ts::UString ts::names::PrivateDataSpecifier(uint32_t pds, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"PrivateDataSpecifier", Names::Value(pds), flags, 32);
}

ts::UString ts::names::CASFamily(ts::CASFamily cas)
{
    return NamesMain::Instance()->nameFromSection(u"CASFamily", Names::Value(cas), NAME | DECIMAL);
}

ts::UString ts::names::CASId(const DuckContext& duck, uint16_t id, Flags flags)
{
    const UChar* section = (duck.standards() & Standards::ISDB) == Standards::ISDB ? u"ARIBCASystemId" : u"CASystemId";
    return NamesMain::Instance()->nameFromSection(section, Names::Value(id), flags, 16);
}

ts::UString ts::names::BouquetId(uint16_t id, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"BouquetId", Names::Value(id), flags, 16);
}

ts::UString ts::names::OriginalNetworkId(uint16_t id, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"OriginalNetworkId", Names::Value(id), flags, 16);
}

ts::UString ts::names::NetworkId(uint16_t id, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"NetworkId", Names::Value(id), flags, 16);
}

ts::UString ts::names::PlatformId(uint32_t id, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"PlatformId", Names::Value(id), flags, 24);
}

ts::UString ts::names::DataBroadcastId(uint16_t id, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DataBroadcastId", Names::Value(id), flags, 16);
}

ts::UString ts::names::OUI(uint32_t oui, Flags flags)
{
    return NamesOUI::Instance()->nameFromSection(u"OUI", Names::Value(oui), flags, 24);
}

ts::UString ts::names::StreamId(uint8_t sid, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"StreamId", Names::Value(sid), flags, 8);
}

ts::UString ts::names::PESStartCode(uint8_t code, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"PESStartCode", Names::Value(code), flags, 8);
}

ts::UString ts::names::AspectRatio(uint8_t ar, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"AspectRatio", Names::Value(ar), flags, 8);
}

ts::UString ts::names::ChromaFormat(uint8_t cf, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"ChromaFormat", Names::Value(cf), flags, 8);
}

ts::UString ts::names::AVCUnitType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"AVCUnitType", Names::Value(type), flags, 8);
}

ts::UString ts::names::AVCProfile(int profile, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"AVCProfile", Names::Value(profile), flags, 8);
}

ts::UString ts::names::ServiceType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"ServiceType", Names::Value(type), flags, 8);
}

ts::UString ts::names::LinkageType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"LinkageType", Names::Value(type), flags, 8);
}

ts::UString ts::names::TeletextType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"TeletextType", Names::Value(type), flags, 8);
}

ts::UString ts::names::RunningStatus(uint8_t status, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"RunningStatus", Names::Value(status), flags, 8);
}

ts::UString ts::names::AudioType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"AudioType", Names::Value(type), flags, 8);
}

ts::UString ts::names::SubtitlingType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"SubtitlingType", Names::Value(type), flags, 8);
}

ts::UString ts::names::DTSSampleRateCode(uint8_t x, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DTSSampleRate", Names::Value(x), flags, 8);
}

ts::UString ts::names::DTSBitRateCode(uint8_t x, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DTSBitRate", Names::Value(x), flags, 8);
}

ts::UString ts::names::DTSSurroundMode(uint8_t x, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DTSSurroundMode", Names::Value(x), flags, 8);
}

ts::UString ts::names::DTSExtendedSurroundMode(uint8_t x, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"DTSExtendedSurroundMode", Names::Value(x), flags, 8);
}

ts::UString ts::names::ScramblingControl(uint8_t scv, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"ScramblingControl", Names::Value(scv), flags, 8);
}

ts::UString ts::names::T2MIPacketType(uint8_t type, Flags flags)
{
    return NamesMain::Instance()->nameFromSection(u"T2MIPacketType", Names::Value(type), flags, 8);
}


//----------------------------------------------------------------------------
// Component Type (in Component Descriptor)
//----------------------------------------------------------------------------

ts::UString ts::names::ComponentType(const DuckContext& duck, uint16_t type, Flags flags)
{
    // There is a special case here. The binary layout of the 16 bits are:
    //   stream_content_ext (4 bits)
    //   stream_content (4 bits)
    //   component_type (8 bits).
    //
    // In the beginning, stream_content_ext did not exist and, as a reserved
    // field, was 0xF. Starting with stream_content > 8, stream_content_ext
    // appeared and may have different values. Logically, stream_content_ext
    // is a subsection of stream_content. So, the bit order for values in the
    // name file is stream_content || stream_content_ext || component_type.
    //
    // We apply the following transformations:
    // - The lookup the name, we use stream_content || stream_content_ext || component_type.
    // - To display the value, we use the real binary value where stream_content_ext
    //   is forced to zero when stream_content is in the range 1 to 8.

    // Stream content:
    const uint16_t sc = (type & 0x0F00) >> 8;

    // Value to use for name lookup:
    const uint16_t nType = (sc >= 1 && sc <= 8 ? 0x0F00 : ((type & 0xF000) >> 4)) | uint16_t((type & 0x0F00) << 4) | (type & 0x00FF);

    // Value to display:
    const uint16_t dType = sc >= 1 && sc <= 8 ? (type & 0x0FFF) : type;

    if ((duck.standards() & Standards::JAPAN) == Standards::JAPAN) {
        // Japan / ISDB uses a completely different mapping.
        return NamesMain::Instance()->nameFromSection(u"ComponentTypeJapan", Names::Value(nType), flags | names::ALTERNATE, 16, dType);
    }
    else if ((nType & 0xFF00) == 0x3F00) {
        return SubtitlingType(nType & 0x00FF, flags);
    }
    else if ((nType & 0xFF00) == 0x4F00) {
        return AC3ComponentType(nType & 0x00FF, flags);
    }
    else {
        return NamesMain::Instance()->nameFromSection(u"ComponentType", Names::Value(nType), flags | names::ALTERNATE, 16, dType);
    }
}


//----------------------------------------------------------------------------
// AC-3 Component Type, field-based, no buildin list of values
//----------------------------------------------------------------------------

ts::UString ts::names::AC3ComponentType(uint8_t type, Flags flags)
{
    ts::UString s((type & 0x80) != 0 ? u"Enhanced AC-3" : u"AC-3");

    s += (type & 0x40) != 0 ? u", full" : u", combined";

    switch (type & 0x38) {
        case 0x00: s += u", complete main"; break;
        case 0x08: s += u", music and effects"; break;
        case 0x10: s += u", visually impaired"; break;
        case 0x18: s += u", hearing impaired"; break;
        case 0x20: s += u", dialogue"; break;
        case 0x28: s += u", commentary"; break;
        case 0x30: s += u", emergency"; break;
        case 0x38: s += (type & 0x40) ? u", karaoke" : u", voiceover"; break;
        default: assert(false); // unreachable
    }

    switch (type & 0x07) {
        case 0: s += u", mono"; break;
        case 1: s += u", 1+1 channel"; break;
        case 2: s += u", 2 channels"; break;
        case 3: s += u", 2 channels dolby surround"; break;
        case 4: s += u", multichannel > 2"; break;
        case 5: s += u", multichannel > 5.1"; break;
        case 6: s += u", multiple substreams"; break;
        case 7: s += u", reserved"; break;
        default: assert(false); // unreachable
    }

    return Names::Formatted(type, s, flags, 8);
}


//----------------------------------------------------------------------------
// Constructor (load the configuration file).
//----------------------------------------------------------------------------

ts::Names::Names(const UString& fileName, bool mergeExtensions) :
    _log(CERR),
    _configFile(SearchConfigurationFile(fileName)),
    _configErrors(0),
    _sections()
{
    // Locate the configuration file.
    if (_configFile.empty()) {
        // Cannot load configuration, names will not be available.
        _log.error(u"configuration file '%s' not found", {fileName});
    }
    else {
        loadFile(_configFile);
    }

    // Merge extensions if required.
    if (mergeExtensions) {
        // Get list of extension names.
        UStringList files;
        PSIRepository::Instance()->getRegisteredNamesFiles(files);
        for (auto name = files.begin(); name != files.end(); ++name) {
            const UString path(SearchConfigurationFile(*name));
            if (path.empty()) {
                _log.error(u"extension file '%s' not found", {*name});
            }
            else {
                loadFile(path);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Load a configuration file and merge its content into this instance.
//----------------------------------------------------------------------------

void ts::Names::loadFile(const UString& fileName)
{
    // Open configuration file.
    std::ifstream strm(fileName.toUTF8().c_str());
    if (!strm) {
        _log.error(u"error opening file %s", {fileName});
        return;
    }

    ConfigSection* section = nullptr;
    UString line;

    // Read configuration file line by line.
    for (size_t lineNumber = 1; line.getLine(strm); ++lineNumber) {

        // Remove leading and trailing spaces in line.
        line.trim();

        if (line.empty() || line[0] == UChar('#')) {
            // Empty or comment line, ignore.
        }
        else if (line.front() == UChar('[') && line.back() == UChar(']')) {
            // Handle beginning of section, get section name.
            line.erase(0, 1);
            line.pop_back();
            line.convertToLower();

            // Get or create associated section.
            ConfigSectionMap::iterator it = _sections.find(line);
            if (it != _sections.end()) {
                section = it->second;
            }
            else {
                // Create new section.
                section = new ConfigSection;
                CheckNonNull(section);
                _sections.insert(std::make_pair(line, section));
            }
        }
        else if (!decodeDefinition(line, section)) {
            // Invalid line.
            _log.error(u"%s: invalid line %d: %s", {fileName, lineNumber, line});
            if (++_configErrors >= 20) {
                // Give up after that number of errors
                _log.error(u"%s: too many errors, giving up", {fileName});
                break;
            }
        }
    }
    strm.close();
}


//----------------------------------------------------------------------------
// Decode a line as "first[-last] = name". Return true on success.
//----------------------------------------------------------------------------

bool ts::Names::decodeDefinition(const UString& line, ConfigSection* section)
{
    // Check the presence of the '=' and in a valid section.
    const size_t equal = line.find(UChar('='));
    if (equal == 0 || equal == NPOS || section == nullptr) {
        return false;
    }

    // Extract fields.
    UString range(line, 0, equal);
    range.trim();

    UString value(line, equal + 1, line.length() - equal - 1);
    value.trim();

    // Allowed "thousands separators" (ignored characters)
    const UString ignore(u".,_");

    // Special case: specification of size in bits of values in this section.
    if (range.similar(u"bits")) {
        return value.toInteger(section->bits, ignore, 0, UString());
    }

    // Decode "first[-last]"
    Value first = 0;
    Value last = 0;
    const size_t dash = range.find(UChar('-'));
    bool valid = false;

    if (dash == NPOS) {
        valid = range.toInteger(first, ignore, 0, UString());
        last = first;
    }
    else {
        valid = range.substr(0, dash).toInteger(first, ignore, 0, UString()) && range.substr(dash + 1).toInteger(last, ignore, 0, UString()) && last >= first;
    }

    // Add the definition.
    if (valid) {
        if (section->freeRange(first, last)) {
            section->addEntry(first, last, value);
        }
        else {
            _log.error(u"%s: range 0x%X-0x%X overlaps with an existing range", {_configFile, first, last});
            valid = false;
        }
    }
    return valid;
}


//----------------------------------------------------------------------------
// Destructor: free all resources.
//----------------------------------------------------------------------------

ts::Names::~Names()
{
    // Deallocate all configuration sections.
    for (ConfigSectionMap::iterator it = _sections.begin(); it != _sections.end(); ++it) {
        delete it->second;
    }
    _sections.clear();
}


//----------------------------------------------------------------------------
// Configuration entry.
//----------------------------------------------------------------------------

ts::Names::ConfigEntry::ConfigEntry(Value l, const UString& n) :
    last(l),
    name(n)
{
}


//----------------------------------------------------------------------------
// Configuration section.
//----------------------------------------------------------------------------

ts::Names::ConfigSection::ConfigSection() :
    bits(0),
    entries()
{
}

ts::Names::ConfigSection::~ConfigSection()
{
    // Deallocate all configuration entries.
    for (ConfigEntryMap::iterator it = entries.begin(); it != entries.end(); ++it) {
        delete it->second;
    }
    entries.clear();
}


//----------------------------------------------------------------------------
// Check if a range is free, ie no value is defined in the range.
//----------------------------------------------------------------------------

bool ts::Names::ConfigSection::freeRange(Value first, Value last) const
{
    // Get an iterator pointing to the first element that is "not less" than 'first'.
    ConfigEntryMap::const_iterator it = entries.lower_bound(first);

    if (it != entries.end() && it->first <= last) {
        // This is an existing range which starts inside [first..last].
        assert(it->first >= first);
        return false;
    }

    if (it != entries.begin() && (--it)->second->last >= first) {
        // The previous range ends inside [first..last].
        assert(it->first < first);
        return false;
    }

    // No overlap found.
    return true;
}


//----------------------------------------------------------------------------
// Add a new configuration entry.
//----------------------------------------------------------------------------

void ts::Names::ConfigSection::addEntry(Value first, Value last, const UString& name)
{
    ConfigEntry* entry = new ConfigEntry(last, name);
    CheckNonNull(entry);
    entries.insert(std::make_pair(first, entry));
}


//----------------------------------------------------------------------------
// Get a name from a value, empty if not found.
//----------------------------------------------------------------------------

ts::UString ts::Names::ConfigSection::getName(Value val) const
{
    // Eliminate trivial cases which would cause issues with code below.
    if (entries.empty()) {
        return UString();
    }

    // The key in the 'entries' map is the _first_ value of a range.
    // Get an iterator pointing to the first element that is "not less" than 'val'.
    ConfigEntryMap::const_iterator it = entries.lower_bound(val);

    if (it == entries.end() || (it != entries.begin() && it->first != val)) {
        // There is no entry with a value range starting at 'val'.
        // Maybe 'val' is in the range of the previous entry.
        --it;
    }

    assert(it != entries.end());
    assert(it->second != nullptr);

    return val >= it->first && val <= it->second->last ? it->second->name : UString();
}


//----------------------------------------------------------------------------
// Format helper
//----------------------------------------------------------------------------

// Compute a number of hexa digits.
int ts::Names::HexaDigits(size_t bits)
{
    return int((bits + 3) / 4);
}

// Compute the display mask
ts::Names::Value ts::Names::DisplayMask(size_t bits)
{
    if (bits == 0 || bits >= 4 * sizeof(Value)) {
        // Unspecified, keep all bits.
        return ~Value(0);
    }
    else {
        return ~Value(0) >> (8 * sizeof(Value) - bits);
    }
}


//----------------------------------------------------------------------------
// Format a name.
//----------------------------------------------------------------------------

ts::UString ts::Names::Formatted(Value value, const UString& name, names::Flags flags, size_t bits, Value alternateValue)
{
    // If neither decimal nor hexa are specified, hexa is the default.
    if ((flags & (names::DECIMAL | names::HEXA)) == 0) {
        flags |= names::HEXA;
    }

    // Actual value to display.
    if ((flags & names::ALTERNATE) != 0) {
        value = alternateValue;
    }

    // Display meaningful bits only.
    value &= DisplayMask(bits);

    // Default name.
    UString defaultName;
    const UString* displayName = &name;
    if (name.empty()) {
        // Name not found.
        if ((flags & names::NAME_OR_VALUE) == 0) {
            // Force value display with a default name.
            flags |= names::VALUE;
            defaultName = u"unknown";
            displayName = &defaultName;
        }
        else if ((flags & names::DECIMAL) != 0) {
            // Display decimal value only.
            return UString::Format(u"%d", {value});
        }
        else {
            // Display hexadecimal value only.
            return UString::Format(u"0x%0*X", {HexaDigits(bits), value});
        }
    }

    if ((flags & (names::VALUE | names::FIRST)) == 0) {
        // Name only.
        return *displayName;
    }

    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum) // enumeration values not explicitly handled in switch
    TS_MSC_NOWARNING(4061)         // enumerator in switch of enum is not explicitly handled by a case label

    switch (flags & (names::FIRST | names::DECIMAL | names::HEXA)) {
        case names::DECIMAL:
            return UString::Format(u"%s (%d)", {*displayName, value});
        case names::HEXA:
            return UString::Format(u"%s (0x%0*X)", {*displayName, HexaDigits(bits), value});
        case names::HEXA | names::DECIMAL:
            return UString::Format(u"%s (0x%0*X, %d)", {*displayName, HexaDigits(bits), value, value});
        case names::DECIMAL | names::FIRST:
            return UString::Format(u"%d (%s)", {value, *displayName});
        case names::HEXA | names::FIRST:
            return UString::Format(u"0x%0*X (%s)", {HexaDigits(bits), value, *displayName});
        case names::HEXA | names::DECIMAL | names::FIRST:
            return UString::Format(u"0x%0*X (%d, %s)", {HexaDigits(bits), value, value, *displayName});
        default:
            assert(false);
            return UString();
    }

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Check if a name exists in a specified section.
//----------------------------------------------------------------------------

bool ts::Names::nameExists(const UString& sectionName, Value value) const
{
    // Get the section, normalize the section name.
    ConfigSectionMap::const_iterator it = _sections.find(sectionName.toTrimmed().toLower());
    return it != _sections.end() && !it->second->getName(value).empty();
}


//----------------------------------------------------------------------------
// Get a name from a specified section.
//----------------------------------------------------------------------------

ts::UString ts::Names::nameFromSection(const UString& sectionName, Value value, names::Flags flags, size_t bits, Value alternateValue) const
{
    // Get the section, normalize the section name.
    ConfigSectionMap::const_iterator it = _sections.find(sectionName.toTrimmed().toLower());
    const ConfigSection* section = it == _sections.end() ? nullptr : it->second;

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value, UString(), flags, bits, alternateValue);
    }
    else {
        return Formatted(value, section->getName(value), flags, bits != 0 ? bits : section->bits, alternateValue);
    }
}


//----------------------------------------------------------------------------
// Get a name from a specified section, with alternate fallback value.
//----------------------------------------------------------------------------

ts::UString ts::Names::nameFromSectionWithFallback(const UString& sectionName, Value value1, Value value2, names::Flags flags, size_t bits, Value alternateValue) const
{
    // Get the section, normalize the section name.
    ConfigSectionMap::const_iterator it = _sections.find(sectionName.toTrimmed().toLower());
    const ConfigSection* section = it == _sections.end() ? nullptr : it->second;

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value1, UString(), flags, bits, alternateValue);
    }
    else {
        const UString name(section->getName(value1));
        if (!name.empty()) {
            // value1 has a name
            return Formatted(value1, name, flags, bits != 0 ? bits : section->bits, alternateValue);
        }
        else {
            // value1 has no name, use value2.
            return Formatted(value2, section->getName(value2), flags, bits != 0 ? bits : section->bits, alternateValue);
        }
    }
}
