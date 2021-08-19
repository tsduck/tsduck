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

#include "tsNamesFile.h"
#include "tsFileUtils.h"
#include "tsCerrReport.h"
#include "tsFatal.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor (load the configuration file).
//----------------------------------------------------------------------------

ts::NamesFile::NamesFile(const UString& fileName, bool mergeExtensions) :
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
        //@@@@ PSIRepository::Instance()->getRegisteredNamesFiles(files);
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

void ts::NamesFile::loadFile(const UString& fileName)
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

bool ts::NamesFile::decodeDefinition(const UString& line, ConfigSection* section)
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

ts::NamesFile::~NamesFile()
{
    // Deallocate all configuration sections.
    for (auto it = _sections.begin(); it != _sections.end(); ++it) {
        delete it->second;
    }
    _sections.clear();
}


//----------------------------------------------------------------------------
// Configuration entry.
//----------------------------------------------------------------------------

ts::NamesFile::ConfigEntry::ConfigEntry(Value l, const UString& n) :
    last(l),
    name(n)
{
}


//----------------------------------------------------------------------------
// Configuration section.
//----------------------------------------------------------------------------

ts::NamesFile::ConfigSection::ConfigSection() :
    bits(0),
    entries()
{
}

ts::NamesFile::ConfigSection::~ConfigSection()
{
    // Deallocate all configuration entries.
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        delete it->second;
    }
    entries.clear();
}


//----------------------------------------------------------------------------
// Check if a range is free, ie no value is defined in the range.
//----------------------------------------------------------------------------

bool ts::NamesFile::ConfigSection::freeRange(Value first, Value last) const
{
    // Get an iterator pointing to the first element that is "not less" than 'first'.
    auto it = entries.lower_bound(first);

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

void ts::NamesFile::ConfigSection::addEntry(Value first, Value last, const UString& name)
{
    ConfigEntry* entry = new ConfigEntry(last, name);
    CheckNonNull(entry);
    entries.insert(std::make_pair(first, entry));
}


//----------------------------------------------------------------------------
// Get a name from a value, empty if not found.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::ConfigSection::getName(Value val) const
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
int ts::NamesFile::HexaDigits(size_t bits)
{
    return int((bits + 3) / 4);
}

// Compute the display mask
ts::NamesFile::Value ts::NamesFile::DisplayMask(size_t bits)
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

ts::UString ts::NamesFile::Formatted(Value value, const UString& name, NamesFlags flags, size_t bits, Value alternateValue)
{
    // If neither decimal nor hexa are specified, hexa is the default.
    if (!(flags & (NamesFlags::DECIMAL | NamesFlags::HEXA))) {
        flags |= NamesFlags::HEXA;
    }

    // Actual value to display.
    if (bool(flags & NamesFlags::ALTERNATE)) {
        value = alternateValue;
    }

    // Display meaningful bits only.
    value &= DisplayMask(bits);

    // Default name.
    UString defaultName;
    const UString* displayName = &name;
    if (name.empty()) {
        // Name not found.
        if (!(flags & NamesFlags::NAME_OR_VALUE)) {
            // Force value display with a default name.
            flags |= NamesFlags::VALUE;
            defaultName = u"unknown";
            displayName = &defaultName;
        }
        else if (bool(flags & NamesFlags::DECIMAL)) {
            // Display decimal value only.
            return UString::Format(u"%d", {value});
        }
        else {
            // Display hexadecimal value only.
            return UString::Format(u"0x%0*X", {HexaDigits(bits), value});
        }
    }

    if (!(flags & (NamesFlags::VALUE | NamesFlags::FIRST))) {
        // Name only.
        return *displayName;
    }

    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum) // enumeration values not explicitly handled in switch
    TS_MSC_NOWARNING(4061)         // enumerator in switch of enum is not explicitly handled by a case label

    switch (flags & (NamesFlags::FIRST | NamesFlags::DECIMAL | NamesFlags::HEXA)) {
        case NamesFlags::DECIMAL:
            return UString::Format(u"%s (%d)", {*displayName, value});
        case NamesFlags::HEXA:
            return UString::Format(u"%s (0x%0*X)", {*displayName, HexaDigits(bits), value});
        case NamesFlags::HEXA | NamesFlags::DECIMAL:
            return UString::Format(u"%s (0x%0*X, %d)", {*displayName, HexaDigits(bits), value, value});
        case NamesFlags::DECIMAL | NamesFlags::FIRST:
            return UString::Format(u"%d (%s)", {value, *displayName});
        case NamesFlags::HEXA | NamesFlags::FIRST:
            return UString::Format(u"0x%0*X (%s)", {HexaDigits(bits), value, *displayName});
        case NamesFlags::HEXA | NamesFlags::DECIMAL | NamesFlags::FIRST:
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

bool ts::NamesFile::nameExists(const UString& sectionName, Value value) const
{
    // Get the section, normalize the section name.
    ConfigSectionMap::const_iterator it = _sections.find(sectionName.toTrimmed().toLower());
    return it != _sections.end() && !it->second->getName(value).empty();
}


//----------------------------------------------------------------------------
// Get a name from a specified section.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::nameFromSection(const UString& sectionName, Value value, NamesFlags flags, size_t bits, Value alternateValue) const
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

ts::UString ts::NamesFile::nameFromSectionWithFallback(const UString& sectionName, Value value1, Value value2, NamesFlags flags, size_t bits, Value alternateValue) const
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
