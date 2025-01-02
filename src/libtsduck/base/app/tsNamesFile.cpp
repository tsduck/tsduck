//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNamesFile.h"
#include "tsFileUtils.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"

// Limit the number of inheritance levels to avoid infinite loop.
#define MAX_INHERIT 16

// Visitor virtual destructor.
ts::NamesFile::Visitor::~Visitor() {}


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
ts::NamesFile::NamesFilePtr ts::NamesFile::AllInstances::getFile(const UString& file_name, bool merge_extensions)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    auto it = _files.find(file_name);
    if (it != _files.end() && it->second != nullptr) {
        return it->second;
    }
    else {
        return _files[file_name] = NamesFilePtr(new NamesFile(file_name, merge_extensions));
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
void ts::NamesFile::AllInstances::addExtensionFile(const UString& file_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (AppendUnique(_ext_file_names, file_name)) {
        // This is a new extension file. Merge it in predefined files which are already loaded.
        for (const auto& pd : _predef) {
            if (pd.merge && pd.instance != nullptr) {
                pd.instance->mergeConfigurationFile(file_name);
            }
        }
    }
}

// Remove an extension file name.
void ts::NamesFile::AllInstances::removeExtensionFile(const UString& file_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    for (auto it = _ext_file_names.begin(); it != _ext_file_names.end(); ++it) {
        if (*it == file_name) {
            _ext_file_names.erase(it);
            break;
        }
    }
}

// Get the list of all extension files.
void ts::NamesFile::AllInstances::getExtensionFiles(UStringList& file_names)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    file_names = _ext_file_names;
}


//----------------------------------------------------------------------------
// A class to register additional names files to merge with the TSDuck names file.
//----------------------------------------------------------------------------

ts::NamesFile::RegisterExtensionFile::RegisterExtensionFile(const UString& file_name)
{
    CERR.debug(u"registering names file %s", file_name);
    AllInstances::Instance().addExtensionFile(file_name);
}

void ts::NamesFile::UnregisterExtensionFile(const UString& file_name)
{
    CERR.debug(u"unregistering names file %s", file_name);
    AllInstances::Instance().removeExtensionFile(file_name);
}


//----------------------------------------------------------------------------
// Constructor (load the configuration file).
//----------------------------------------------------------------------------

ts::NamesFile::NamesFile(const UString& fileName, bool mergeExtensions) :
    _log(CERR),
    _config_file(SearchConfigurationFile(fileName))
{
    // Locate the configuration file.
    if (_config_file.empty()) {
        // Cannot load configuration, names will not be available.
        _log.error(u"configuration file '%s' not found", fileName);
    }
    else {
        mergeFile(_config_file);
    }

    // Merge extensions if required.
    if (mergeExtensions) {
        // Get list of extension names.
        UStringList files;
        AllInstances::Instance().getExtensionFiles(files);
        for (const auto& name : files) {
            mergeConfigurationFile(name);
        }
    }
}


//----------------------------------------------------------------------------
// Load a configuration file and merge its content into this instance.
//----------------------------------------------------------------------------

bool ts::NamesFile::mergeConfigurationFile(const UString& file_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const UString path(SearchConfigurationFile(file_name));
    if (path.empty()) {
        _log.error(u"configuration file '%s' not found", file_name);
        return false;
    }
    else {
        return mergeFile(path);
    }
}


//----------------------------------------------------------------------------
// Load a names file and merge its content into this instance.
//----------------------------------------------------------------------------

bool ts::NamesFile::mergeFile(const UString& file_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _log.debug(u"loading names file %s", file_name);

    // Open configuration file.
    std::ifstream strm(file_name.toUTF8().c_str());
    if (!strm) {
        _config_errors++;
        _log.error(u"error opening file %s", file_name);
        return false;
    }

    ConfigSectionPtr section;
    VisitorBounds visitors {_visitors.begin(), _visitors.begin()};
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
            line = NormalizedSectionName(line);

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
            visitors = _visitors.equal_range(line);
        }
        else if (!decodeDefinition(section_name, visitors, line, section)) {
            // Invalid line.
            _log.error(u"%s: invalid line %d: %s", file_name, lineNumber, line);
            if (++_config_errors >= 20) {
                // Give up after that number of errors
                _log.error(u"%s: too many errors, giving up", file_name);
                break;
            }
        }
    }
    strm.close();

    // Verify that all sections have bits size.
    for (const auto& sec_iter : _sections) {
        const auto& sname(sec_iter.first);
        auto& sec(*sec_iter.second);

        // Fetch bits value from "superclasses".
        UString parent(sec.inherit);
        while (sec.bits == 0 && !parent.empty()) {
            auto next = _sections.find(NormalizedSectionName(parent));
            if (next == _sections.end()) {
                _log.error(u"%d: section %s inherits from non-existent section %s", _config_file, sname, parent);
                break;
            }
            sec.bits = next->second->bits;
            parent = next->second->inherit;
        }

        // Verify the presence of bits size.
        if (sec.bits == 0) {
            _log.error(u"%d: no specified bits size in section %s", _config_file, sname);
        }
        else {
            // Mask to extract the basic value, without the potential extension.
            sec.mask = ~Value(0) >> (8 * sizeof(Value) - sec.bits);

            // Verify the presence of extended values in the section.
            bool extended = false;
            for (const auto& val : sec.entries) {
                // Only check the extension in 'last', it is greated than 'first'.
                if ((val.second->last & ~sec.mask) != 0) {
                    extended = true;
                    break;
                }
            }
            if (extended != sec.extended) {
                _log.error(u"%d: section %s, extended is %s, found%s extended values", _config_file, sname, sec.extended, extended ? u"" : u" no");
            }

            // In the presence of extended values, build the 'short_entries' multimap, indexed by short values.
            if (extended) {
                assert(sec.bits < 8 * sizeof(Value));
                // If there are more than one value in the range, it is possible that they span multiple short values.
                const Value increment = Value(1) << sec.bits;
                const Value max = std::numeric_limits<Value>::max() - increment;
                for (const auto& val : sec.entries) {
                    Value index = val.second->first;
                    while (index <= val.second->last) {
                        sec.short_entries.insert(std::make_pair(index & sec.mask, val.second));
                        if (index > max) {
                            break; // avoid integer overflow
                        }
                        index += increment;
                    }
                }
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decode a line as "first[-last] = name". Return true on success.
//----------------------------------------------------------------------------

bool ts::NamesFile::decodeDefinition(const UString& section_name, const VisitorBounds& visitors, const UString& line, ConfigSectionPtr section)
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
            _log.error(u"%s: section %s, duplicated bits clauses %d and %s", _config_file, section_name, section->bits, value);
            return false;
        }
        else if (value.toInteger(bits, ignore, 0, UString()) && bits > 0 && bits <= 8 * sizeof(Value)) {
            section->bits = bits;
            return true;
        }
        else {
            _log.error(u"%s: section %s, invalid bits value; %s", _config_file, section_name, value);
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
            _log.error(u"%s: section %s, duplicated inherit clauses %s and %s", _config_file, section_name, section->inherit, value);
            return false;
        }
    }
    else if (range.similar(u"extended")) {
        // "extended = true|false" indicates the presence of extended values, larger than the specified bit size.
        return value.toBool(section->extended);
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
            // Valid range, add it.
            section->addEntry(first, last, value);
            // Notify visitors of the new value.
            for (auto vis : _full_visitors) {
                for (Value i = first; i <= last; ++i) {
                    vis->handleNameValue(section_name, i, value);
                }
            }
            for (auto vis = visitors.first; vis != visitors.second; ++vis) {
                for (Value i = first; i <= last; ++i) {
                    vis->second->handleNameValue(section_name, i, value);
                }
            }
        }
        else {
            _log.error(u"%s: section %s, range 0x%X-0x%X overlaps with an existing range", _config_file, section_name, first, last);
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
    ConfigEntryPtr entry = std::make_shared<ConfigEntry>();
    entry->first = first;
    entry->last = last;
    entry->name = name;
    entries.insert(std::make_pair(first, entry));
}


//----------------------------------------------------------------------------
// Get an entry or name from a value.
//----------------------------------------------------------------------------

// Get a name from a value, empty if not found.
ts::UString ts::NamesFile::ConfigSection::getName(Value val) const
{
    const auto entry(getEntry(val));
    return entry != nullptr ? entry->name : UString();
}

// Get the entry for a given value, nullptr if not found.
ts::NamesFile::ConfigEntryPtr ts::NamesFile::ConfigSection::getEntry(Value val) const
{
    // Eliminate trivial cases which would cause issues with code below.
    if (entries.empty()) {
        return nullptr;
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

    return val >= it->second->first && val <= it->second->last ? it->second : nullptr;
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

ts::UString ts::NamesFile::Formatted(Value value, const UString& name, NamesFlags flags, size_t bits, Value alternate_value)
{
    // If neither decimal nor hexa are specified, hexa is the default.
    if (!(flags & (NamesFlags::DECIMAL | NamesFlags::HEXA))) {
        flags |= NamesFlags::HEXA;
    }

    // Actual value to display.
    if (bool(flags & NamesFlags::ALTERNATE)) {
        value = alternate_value;
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

void ts::NamesFile::getName(const UString& section_name, Value value, ConfigSectionPtr& section, UString& name) const
{
    // Normalized section name.
    UString sname(NormalizedSectionName(section_name));

    // Limit the number of inheritance levels to avoid infinite loop.
    int levels = MAX_INHERIT;

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
// Get basic properties.
//----------------------------------------------------------------------------

size_t ts::NamesFile::errorCount() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    return _config_errors;
}


//----------------------------------------------------------------------------
// Check if a name exists in a specified section.
//----------------------------------------------------------------------------

bool ts::NamesFile::nameExists(const UString& section_name, Value value) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    ConfigSectionPtr section;
    UString name;
    getName(section_name, value, section, name);
    return !name.empty();
}


//----------------------------------------------------------------------------
// Get a name from a specified section.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::nameFromSection(const UString& section_name, Value value, NamesFlags flags, Value alternate_value, size_t bits) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    ConfigSectionPtr section;
    UString name;
    getName(section_name, value, section, name);

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value, UString(), flags, bits, alternate_value);
    }
    else {
        return Formatted(value, name, flags, bits != 0 ? bits : section->bits, alternate_value);
    }
}


//----------------------------------------------------------------------------
// Get a name from a specified section, with alternate fallback value.
//----------------------------------------------------------------------------

ts::UString ts::NamesFile::nameFromSectionWithFallback(const UString& section_name, Value value1, Value value2, NamesFlags flags, Value alternate_value, size_t bits) const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    ConfigSectionPtr section;
    UString name;
    getName(section_name, value1, section, name);

    if (section == nullptr) {
        // Non-existent section, no name.
        return Formatted(value1, UString(), flags, bits, alternate_value);
    }
    else if (!name.empty()) {
        // value1 has a name
        return Formatted(value1, name, flags, bits != 0 ? bits : section->bits, alternate_value);
    }
    else {
        // value1 has no name, use value2, restart from the beginning in case of inheritance.
        return nameFromSection(section_name, value2, flags, alternate_value, bits);
    }
}


//----------------------------------------------------------------------------
// Get all values in a section.
//----------------------------------------------------------------------------

size_t ts::NamesFile::visitSection(Visitor* visitor, const UString& section_name) const
{
    // Trivial cose, nothing to visit.
    if (visitor == nullptr) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    size_t visit_count = 0;
    const UString* secname = &section_name;

    // Loop on inherited sections.
    for (int levels = MAX_INHERIT; levels > 0; --levels) {

        // Get current section.
        if (secname->empty()) {
            break; // No more inherited section.
        }
        const auto it = _sections.find(NormalizedSectionName(*secname));
        ConfigSectionPtr section(it == _sections.end() ? nullptr : it->second);
        if (section == nullptr) {
            break; // Non-existent section.
        }

        // Loop on all values in this section.
        for (const auto& ent : section->entries) {
            for (Value i = ent.second->first; i <= ent.second->last; ++i) {
                visit_count++;
                if (!visitor->handleNameValue(*secname, i, ent.second->name)) {
                    return visit_count;
                }
            }
        }

        // "Superclass" section name.
        secname = &section->inherit;
    }

    return visit_count;
}


//----------------------------------------------------------------------------
// Get all extended values of a specified value in a section.
//----------------------------------------------------------------------------

size_t ts::NamesFile::visitSection(Visitor* visitor, const UString& section_name, Value value) const
{
    // Trivial cose, nothing to visit.
    if (visitor == nullptr) {
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(_mutex);
    size_t visit_count = 0;
    const UString* secname = &section_name;

    // Loop on inherited sections.
    for (int levels = MAX_INHERIT; levels > 0; --levels) {

        // Get current section.
        if (secname->empty()) {
            break; // No more inherited section.
        }
        const auto it = _sections.find(NormalizedSectionName(*secname));
        ConfigSectionPtr section(it == _sections.end() ? nullptr : it->second);
        if (section == nullptr) {
            break; // Non-existent section.
        }

        // When "Extended=false" (the default), there is only one value, the short_entries multimap is empty.
        if (section->short_entries.empty()) {
            // Add the target value alone if it is registered.
            const auto entry(section->getEntry(value));
            if (entry != nullptr) {
                visit_count++;
                if (!visitor->handleNameValue(*secname, value, entry->name)) {
                    return visit_count;
                }
            }
        }
        else {
            // There are extended values in short_entries.
            const Value increment = Value(1) << section->bits;
            const Value max = std::numeric_limits<Value>::max() - increment;

            // Get all values in the multimap for the base value.
            const auto bounds(section->short_entries.equal_range(value & section->mask));
            for (auto next = bounds.first; next != bounds.second; ++next) {
                const auto& val(*next->second);
                Value i = (val.first & ~section->mask) | (value & section->mask);
                while (i <= val.last) {
                    visit_count++;
                    if (!visitor->handleNameValue(*secname, i, val.name)) {
                        return visit_count;
                    }
                    if (i > max) {
                        break; // avoid integer overflow
                    }
                    i += increment;
                }
            }
        }

        // "Superclass" section name.
        secname = &section->inherit;
    }

    return visit_count;
}


//----------------------------------------------------------------------------
// Subscribe to all new values which will be merged into the file.
//----------------------------------------------------------------------------

void ts::NamesFile::subscribe(Visitor* visitor, const UString& section_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (visitor != nullptr) {
        const UString name(NormalizedSectionName(section_name));
        if (name.empty()) {
            // Subscribe for all sections.
            _full_visitors.insert(visitor);
        }
        else {
            // Check if already subscribed.
            const auto bounds(_visitors.equal_range(name));
            for (auto next = bounds.first; next != bounds.second; ++next) {
                if (next->second == visitor) {
                    // Already subscribed.
                    return;
                }
            }
            _visitors.insert(std::make_pair(name, visitor));
        }
    }
}


//----------------------------------------------------------------------------
// Unsubscribe from all new values which will be merged into the file.
//----------------------------------------------------------------------------

void ts::NamesFile::unsubscribe(Visitor* visitor, const UString& section_name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const UString name(NormalizedSectionName(section_name));

    if (visitor == nullptr) {
        if (name.empty()) {
            // Unsubscribe all visitors from everything.
            _visitors.clear();
            _full_visitors.clear();
        }
        else {
            // Unsubscribe all visitors from one section.
            _visitors.erase(name);
        }
    }
    else {
        if (name.empty()) {
            // Unsubscribe one visitor from everything.
            _full_visitors.erase(visitor);
            for (auto next = _visitors.begin(); next != _visitors.end(); ) {
                if (next->second == visitor) {
                    next = _visitors.erase(next);
                }
                else {
                    ++next;
                }
            }
        }
        else {
            // Unsubscribe one visitor from one section.
            for (auto next = _visitors.lower_bound(name); next != _visitors.end() && next->first == name; ) {
                if (next->second == visitor) {
                    next = _visitors.erase(next);
                }
                else {
                    ++next;
                }
            }
        }
    }
}
