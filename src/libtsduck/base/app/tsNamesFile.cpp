//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNamesFile.h"
#include "tsFileUtils.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// A singleton which manages all NamesFile instances (thread-safe).
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::NamesFile::AllInstances);

// Constructor
ts::NamesFile::AllInstances::AllInstances()
{
    _predef[size_t(Predefined::DTV)].merge = true;
    _predef[size_t(Predefined::DTV)].name = u"tsduck.dtv.names";
    _predef[size_t(Predefined::IP)].name = u"tsduck.ip.names";
    _predef[size_t(Predefined::OUI)].name = u"tsduck.oui.names";
    _predef[size_t(Predefined::DEKTEC)].name = u"tsduck.dektec.names";
    _predef[size_t(Predefined::HIDES)].name = u"tsduck.hides.names";
}

// Lookup / load a names file.
ts::NamesFile::NamesFilePtr ts::NamesFile::AllInstances::getFile(const UString& fileName, bool mergeExtensions)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    auto it = _files.find(fileName);
    if (it != _files.end() && it->second != nullptr) {
        return it->second;
    }
    else {
        return _files[fileName] = NamesFilePtr(new NamesFile(fileName, mergeExtensions));
    }
}

// Lookup / load a predefined names file.
ts::NamesFile::NamesFilePtr ts::NamesFile::AllInstances::getFile(Predefined index)
{
    if (size_t(index) >= _predef.size()) {
        CERR.error(u"internal error, invalid predefined .names file index");
        return nullptr; // and the application will likely crash...
    }
    else {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        Predef& pr(_predef[size_t(index)]);
        if (pr.instance == nullptr) {
            pr.instance = getFile(pr.name, pr.merge);
        }
        return pr.instance;
    }
}

// Delete one instance.
void ts::NamesFile::AllInstances::unregister(Predefined index)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (size_t(index) < _predef.size() && _predef[size_t(index)].instance != nullptr) {
        for (auto it = _files.begin(); it != _files.end(); ++it) {
            if (_predef[size_t(index)].instance == it->second) {
                it = _files.erase(it);
                break;
            }
        }
        _predef[size_t(index)].instance = nullptr;
    }
}

// Add an extension file name (check that there is no duplicate).
void ts::NamesFile::AllInstances::addExtensionFile(const UString& fileName)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    AppendUnique(_extFiles, fileName);
}

// Remove an extension file name.
void ts::NamesFile::AllInstances::removeExtensionFile(const UString& fileName)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    for (auto it = _extFiles.begin(); it != _extFiles.end(); ++it) {
        if (*it == fileName) {
            _extFiles.erase(it);
            break;
        }
    }
}

// Get the list of all extension files.
void ts::NamesFile::AllInstances::getExtensionFiles(UStringList& fileNames)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    fileNames = _extFiles;
}


//----------------------------------------------------------------------------
// A class to register additional names files to merge with the TSDuck names file.
//----------------------------------------------------------------------------

ts::NamesFile::RegisterExtensionFile::RegisterExtensionFile(const UString& filename)
{
    CERR.debug(u"registering names file %s", filename);
    AllInstances::Instance().addExtensionFile(filename);
}

void ts::NamesFile::UnregisterExtensionFile(const UString& filename)
{
    CERR.debug(u"unregistering names file %s", filename);
    AllInstances::Instance().removeExtensionFile(filename);
}


//----------------------------------------------------------------------------
// Constructor (load the configuration file).
//----------------------------------------------------------------------------

ts::NamesFile::NamesFile(const UString& fileName, bool mergeExtensions) :
    _log(CERR),
    _configFile(SearchConfigurationFile(fileName))
{
    // Locate the configuration file.
    if (_configFile.empty()) {
        // Cannot load configuration, names will not be available.
        _log.error(u"configuration file '%s' not found", fileName);
    }
    else {
        loadFile(_configFile);
    }

    // Merge extensions if required.
    if (mergeExtensions) {
        // Get list of extension names.
        UStringList files;
        AllInstances::Instance().getExtensionFiles(files);
        for (const auto& name : files) {
            const UString path(SearchConfigurationFile(name));
            if (path.empty()) {
                _log.error(u"extension file '%s' not found", name);
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
    _log.debug(u"loading names file %s", fileName);

    // Open configuration file.
    std::ifstream strm(fileName.toUTF8().c_str());
    if (!strm) {
        _configErrors++;
        _log.error(u"error opening file %s", fileName);
        return;
    }

    ConfigSectionPtr section;
    UString section_name;
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
            section_name = line;
            line.convertToLower();

            // Get or create associated section.
            auto it = _sections.find(line);
            if (it != _sections.end()) {
                section = it->second;
            }
            else {
                // Create new section.
                section = std::make_shared<ConfigSection>();
                _sections.insert(std::make_pair(line, section));
            }
        }
        else if (!decodeDefinition(section_name, line, section)) {
            // Invalid line.
            _log.error(u"%s: invalid line %d: %s", fileName, lineNumber, line);
            if (++_configErrors >= 20) {
                // Give up after that number of errors
                _log.error(u"%s: too many errors, giving up", fileName);
                break;
            }
        }
    }
    strm.close();

    // Verify that all sections have bits size.
    for (const auto& it : _sections) {
        // Fetch bits value from "superclasses".
        UString parent(it.second->inherit);
        while (it.second->bits == 0 && !parent.empty()) {
            auto next = _sections.find(parent.toLower());
            if (next == _sections.end()) {
                _log.error(u"%d: section %s inherits from non-existent section %s", _configFile, it.first, parent);
                break;
            }
            it.second->bits = next->second->bits;
            parent = next->second->inherit;
        }
        if (it.second->bits == 0) {
            _log.error(u"%d: no specified bits size in section %s", _configFile, it.first);
        }
        else {
            it.second->mask = ~Value(0) >> (8 * sizeof(Value) - it.second->bits);
        }
    }
}


//----------------------------------------------------------------------------
// Decode a line as "first[-last] = name". Return true on success.
//----------------------------------------------------------------------------

bool ts::NamesFile::decodeDefinition(const UString& section_name, const UString& line, ConfigSectionPtr section)
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

    // Special cases (not values):
    if (range.similar(u"bits")) {
        // Specification of size in bits of values in this section.
        size_t bits = 0;
        if (section->bits > 0) {
            _log.error(u"%s: section %s, duplicated bits clauses %d and %s", _configFile, section_name, section->bits, value);
            return false;
        }
        else if (value.toInteger(bits, ignore, 0, UString()) && bits > 0 && bits <= 8 * sizeof(Value)) {
            section->bits = bits;
            return true;
        }
        else {
            _log.error(u"%s: section %s, invalid bits value; %s", _configFile, section_name, value);
            return false;
        }
    }
    else if (range.similar(u"inherit")) {
        // Name of a section where to search unknown values here.
        if (section->inherit.empty()) {
            section->inherit = value;
            return true;
        }
        else {
            _log.error(u"%s: section %s, duplicated inherit clauses %s and %s", _configFile, section_name, section->inherit, value);
            return false;
        }
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
            _log.error(u"%s: section %s, range 0x%X-0x%X overlaps with an existing range", _configFile, section_name, first, last);
            valid = false;
        }
    }
    return valid;
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
    auto it = entries.lower_bound(val);

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
        if (bool(flags & NamesFlags::NO_UNKNOWN)) {
            // Do not format unknown values.
            return UString();
        }
        else if (!(flags & NamesFlags::NAME_OR_VALUE)) {
            // Force value display with a default name.
            flags |= NamesFlags::VALUE;
            defaultName = u"unknown";
            displayName = &defaultName;
        }
        else if (bool(flags & NamesFlags::DECIMAL)) {
            // Display decimal value only.
            return UString::Format(u"%d", value);
        }
        else {
            // Display hexadecimal value only.
            return UString::Format(u"0x%0*X", HexaDigits(bits), value);
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
            return UString::Format(u"%s (%d)", *displayName, value);
        case NamesFlags::HEXA:
            return UString::Format(u"%s (0x%0*X)", *displayName, HexaDigits(bits), value);
        case NamesFlags::HEXA | NamesFlags::DECIMAL:
            return UString::Format(u"%s (0x%0*X, %d)", *displayName, HexaDigits(bits), value, value);
        case NamesFlags::DECIMAL | NamesFlags::FIRST:
            return UString::Format(u"%d (%s)", value, *displayName);
        case NamesFlags::HEXA | NamesFlags::FIRST:
            return UString::Format(u"0x%0*X (%s)", HexaDigits(bits), value, *displayName);
        case NamesFlags::HEXA | NamesFlags::DECIMAL | NamesFlags::FIRST:
            return UString::Format(u"0x%0*X (%d, %s)", HexaDigits(bits), value, value, *displayName);
        default:
            assert(false);
            return UString();
    }

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Get the section and name from a value, empty if not found.
//----------------------------------------------------------------------------

void ts::NamesFile::getName(const UString& sectionName, Value value, ConfigSectionPtr& section, UString& name) const
{
    // Normalized section name.
    UString sname(NormalizedSectionName(sectionName));

    // Limit the number of inheritance levels to avoid infinite loop.
    int levels = 16;

    // Loop on inherited sections, until a name is found.
    for (;;) {
        // Get the section.
        const auto it = _sections.find(sname);
        if (it == _sections.end()) {
            // Section not found, no name.
            section = nullptr;
            name.clear();
            return;
        }

        // Get the name of the value in the section.
        section = it->second;
        name = section->getName(value);

        // Return when name found or no "superclass" or too many levels of inheritance.
        if (!name.empty() || section->inherit.empty() || levels-- <= 0) {
            return;
        }

        // Loop on "superclass".
        sname = NormalizedSectionName(section->inherit);
    }
}


//----------------------------------------------------------------------------
// Check if a name exists in a specified section.
//----------------------------------------------------------------------------

bool ts::NamesFile::nameExists(const UString& sectionName, Value value) const
{
    ConfigSectionPtr section;
    UString name;
    getName(sectionName, value, section, name);
    return !name.empty();
}


//----------------------------------------------------------------------------
// Get a name from a specified section.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::nameFromSection(const UString& sectionName, Value value, NamesFlags flags, size_t bits, Value alternateValue) const
{
    ConfigSectionPtr section;
    UString name;
    getName(sectionName, value, section, name);

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value, UString(), flags, bits, alternateValue);
    }
    else {
        return Formatted(value, name, flags, bits != 0 ? bits : section->bits, alternateValue);
    }
}


//----------------------------------------------------------------------------
// Get a name from a specified section, with alternate fallback value.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::nameFromSectionWithFallback(const UString& sectionName, Value value1, Value value2, NamesFlags flags, size_t bits, Value alternateValue) const
{
    ConfigSectionPtr section;
    UString name;
    getName(sectionName, value1, section, name);

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value1, UString(), flags, bits, alternateValue);
    }
    else if (!name.empty()) {
        // value1 has a name
        return Formatted(value1, name, flags, bits != 0 ? bits : section->bits, alternateValue);
    }
    else {
        // value1 has no name, use value2, restart from the beginning in case of inheritance.
        return nameFromSection(sectionName, value2, flags, bits, alternateValue);
    }
}
