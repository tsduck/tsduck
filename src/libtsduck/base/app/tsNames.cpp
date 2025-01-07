//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsFileUtils.h"
#include "tsIntegerUtils.h"
#include "tsCerrReport.h"

// Limit the number of inheritance levels to avoid infinite loop.
#define MAX_INHERIT 16

// Visitor virtual destructor.
ts::Names::Visitor::~Visitor() {}


//----------------------------------------------------------------------------
// Constructor from a variable list of string/value pairs.
//----------------------------------------------------------------------------

ts::Names::Names(std::initializer_list<NameValue> values)
{
    // No need to lock, this is a constructor.
    for (const auto& it : values) {
        addValueImplLocked(it);
    }
}


//----------------------------------------------------------------------------
// Check if a range is free, ie no value is defined in the range.
//----------------------------------------------------------------------------

bool ts::Names::freeRange(uint_t first, uint_t last) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return freeRangeLocked(first, last);
}

bool ts::Names::freeRangeLocked(uint_t first, uint_t last) const
{
    // Get an iterator pointing to the first element that is "not less" than 'first'.
    auto it = _entries.lower_bound(first);

    if (it != _entries.end() && it->first <= last) {
        // This is an existing range which starts inside [first..last].
        assert(it->first >= first);
        return false;
    }

    if (it != _entries.begin() && (--it)->second->last >= first) {
        // The previous range ends inside [first..last].
        assert(it->first < first);
        return false;
    }

    // No overlap found.
    return true;
}


//----------------------------------------------------------------------------
// Add a value in the set of translation.
//----------------------------------------------------------------------------

void ts::Names::addValueImpl(const NameValue& range)
{
    // Write lock (exclusive).
    std::lock_guard<std::shared_mutex> lock(_mutex);
    addValueImplLocked(range);
}

void ts::Names::addValueImplLocked(const NameValue& range)
{
    // One single negative value marks the instance as "signed".
    if (range.neg_first || range.neg_last) {
        _is_signed = true;
    }

    // Add a range if non empty (ie. first <= last).
    if (range.neg_first == range.neg_last && range.first <= range.last) {
        addValueImplLocked(range.name, range.first, range.last);
    }
    else if (range.neg_first && !range.neg_last) {
        addValueImplLocked(range.name, range.first, std::numeric_limits<uint_t>::max());
        addValueImplLocked(range.name, 0, range.last);
    }
}

void ts::Names::addValueImplLocked(const UString& name, uint_t first, uint_t last)
{
    _entries.insert(std::make_pair(first, std::make_shared<ValueRange>(first, last, name)));
    for (auto vis : _visitors) {
        for (uint_t i = first; i <= last; ++i) {
            vis->handleNameValue(*this, i, name);
        }
    }
}


//----------------------------------------------------------------------------
// Get the range for a given value, nullptr if not found.
//----------------------------------------------------------------------------

ts::Names::ValueRangePtr ts::Names::getRangeLocked(uint_t val) const
{
    // Eliminate trivial cases which would cause issues with the code below.
    if (_entries.empty()) {
        return nullptr;
    }

    // The key in the '_entries' map is the _first_ value of a range.
    // Get an iterator pointing to the first element that is "not less" than 'val'.
    auto it = _entries.lower_bound(val);

    if (it == _entries.end() || (it != _entries.begin() && it->first != val)) {
        // There is no entry with a value range starting at 'val'.
        // Maybe 'val' is in the range of the previous entry.
        --it;
    }

    assert(it != _entries.end());
    assert(it->second != nullptr);

    return val >= it->second->first && val <= it->second->last ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Translate a string as a value.
//----------------------------------------------------------------------------

ts::Names::int_t ts::Names::value(const UString& name, bool case_sensitive, bool abbreviated) const
{
    uint_t val = 0;
    return getValueImpl(val, name, case_sensitive, abbreviated) ? static_cast<int_t>(val) : UNKNOWN;
}

bool ts::Names::getValueImpl(uint_t& e, const UString& name, bool case_sensitive, bool abbreviated) const
{
    const UString lc_name(name.toLower());
    size_t previous_count = 0;
    ValueRangePtr previous;

    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    for (const auto& it : _entries) {
        if ((case_sensitive && it.second->name == name) || (!case_sensitive && it.second->name.toLower() == lc_name)) {
            // Found an exact match.
            e = it.second->first;
            return true;
        }
        else if (abbreviated && it.second->name.startsWith(name, case_sensitive ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
            // Found an abbreviated version
            if (++previous_count == 1) {
                // First abbreviation, remember it and continue searching
                previous = it.second;
            }
            else {
                // Another abbreviation already found, name is ambiguous
                break;
            }
        }
    }

    if (previous_count == 1) {
        // Only one solution for abbreviation
        e = previous->first;
        return true;
    }

    // Check if name evaluates to an integer
    return name.toInteger(e, u",");
}


//----------------------------------------------------------------------------
// Get the error message about a name failing to match a value.
//----------------------------------------------------------------------------

ts::UString ts::Names::error(const UString& name, bool case_sensitive, bool abbreviated, const UString& designator, const UString& prefix) const
{
    const UString lc_name(name.toLower());
    UStringList maybe;

    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    for (const auto& it : _entries) {
        if ((case_sensitive && it.second->name == name) || (!case_sensitive && it.second->name.toLower() == lc_name)) {
            // Found an exact match, there is no error.
            return UString();
        }
        else if (abbreviated && it.second->name.startsWith(name, case_sensitive ? CASE_SENSITIVE : CASE_INSENSITIVE)) {
            // Found an abbreviated version.
            maybe.push_back(prefix + it.second->name);
        }
    }

    if (maybe.empty()) {
        return UString::Format(u"unknown %s \"%s%s\"", designator, prefix, name);
    }
    else if (maybe.size() == 1) {
        // Only one possibility, there is no error.
        return UString();
    }
    else {
        return UString::Format(u"ambiguous %s \"%s%s\", could be one of %s", designator, prefix, name, UString::Join(maybe));
    }
}


//----------------------------------------------------------------------------
// Check if a name exists for a given value.
//----------------------------------------------------------------------------

bool ts::Names::containsImpl(uint_t value) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return getRangeLocked(value) != nullptr;
}


//----------------------------------------------------------------------------
// Translate a value as a string.
//----------------------------------------------------------------------------

ts::UString ts::Names::getNameImpl(uint_t value, bool hexa, size_t hex_digits, size_t default_hex_digits) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    const auto range = getRangeLocked(value);
    if (range != nullptr && !range->name.empty()) {
        return range->name;
    }
    else if (hexa) {
        // Actual number of hexa digits to print.
        if (hex_digits == 0) {
            hex_digits = _bits == 0 ? default_hex_digits : (_bits + 3) / 4;
        }
        return UString::Format(u"0x%0*X", hex_digits, value);
    }
    else {
        return UString::Decimal(value, 0, true, UString());
    }
}


//----------------------------------------------------------------------------
// Get the names from a bit-mask value.
//----------------------------------------------------------------------------

ts::UString ts::Names::bitMaskNamesImpl(uint_t value, const UString& separator, bool hexa, size_t hex_digits, size_t default_hex_digits) const
{
    UString list;
    uint_t done = 0; // Bitmask of all values which are already added in the list.

    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // Insert all known names. Only consider first value of all ranges.
    for (const auto& it : _entries) {
        if ((value & it.first) == it.first) {
            // This bit pattern is present.
            done |= it.first;
            if (!list.empty()) {
                list += separator;
            }
            list += it.second->name;
        }
    }

    // Actual number of hexa digits to print.
    if (hex_digits == 0) {
        hex_digits = _bits == 0 ? default_hex_digits : (_bits + 3) / 4;
    }

    // Now loop on bits which were not already printed.
    value &= ~done;
    for (int_t mask = 1; value != 0 && mask != 0; mask <<= 1) {
        value &= ~mask;
        if (!list.empty()) {
            list += separator;
        }
        if (hexa) {
            list += UString::Format(u"0x%0*X", hex_digits, mask);
        }
        else {
            list += UString::Decimal(mask, 0, true, UString());
        }
    }

    return list;
}


//----------------------------------------------------------------------------
// Get a fully formatted name from a value.
//----------------------------------------------------------------------------

ts::UString ts::Names::formatted(uint_t value, NamesFlags flags, uint_t alternate_value, size_t bits) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);
    return formattedLocked(value, flags, alternate_value, bits);
}

ts::UString ts::Names::formattedLocked(uint_t value, NamesFlags flags, uint_t alternate_value, size_t bits) const
{
    const auto range = getRangeLocked(value);
    if (range == nullptr) {
        // Non-existent value, no name.
        return Format(value, UString(), flags, bits, alternate_value);
    }
    else {
        return Format(value, range->name, flags, bits != 0 ? bits : _bits, alternate_value);
    }
}


//----------------------------------------------------------------------------
// Get a fully formatted name from a value, with alternate fallback value.
//----------------------------------------------------------------------------

ts::UString ts::Names::formattedWithFallback(uint_t value1, uint_t value2, NamesFlags flags, uint_t alternate_value, size_t bits) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    const auto range = getRangeLocked(value1);
    if (range == nullptr) {
        // value1 has no name, use value2, restart from the beginning in case of inheritance.
        return formattedLocked(value2, flags, alternate_value, bits);
    }
    else {
        // value1 has a name
        return Format(value1, range->name, flags, bits != 0 ? bits : _bits, alternate_value);
    }
}


//----------------------------------------------------------------------------
// Return a comma-separated list of all possible names.
//----------------------------------------------------------------------------

ts::UString ts::Names::nameList(const UString& separator, const UString& in_quote, const UString& out_quote) const
{
    // Read lock (shared).
    std::shared_lock<std::shared_mutex> lock(_mutex);

    UStringVector sl;
    sl.reserve(_entries.size());
    for (const auto& it : _entries) {
        sl.push_back(in_quote + it.second->name + out_quote);
    }
    std::sort(sl.begin(), sl.end());
    return UString::Join(sl, separator);
}


//----------------------------------------------------------------------------
// Format a name.
//----------------------------------------------------------------------------

ts::UString ts::Names::Format(uint_t value, const UString& name, NamesFlags flags, size_t bits, uint_t alternate_value)
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
    value &= LSBMask<uint_t>(bits);

    // Number of hexa digits if hexa display is required.
    const int hexa_digits = int((bits + 3) / 4);

    // Default name.
    UString default_name;
    const UString* display_name = &name;
    if (name.empty()) {
        // Name not found.
        if (bool(flags & NamesFlags::NO_UNKNOWN)) {
            // Do not format unknown values.
            return UString();
        }
        else if (!(flags & NamesFlags::NAME_OR_VALUE)) {
            // Force value display with a default name.
            flags |= NamesFlags::NAME_VALUE;
            default_name = u"unknown";
            display_name = &default_name;
        }
        else if (bool(flags & NamesFlags::DECIMAL)) {
            // Display decimal value only.
            return UString::Format(u"%d", value);
        }
        else {
            // Display hexadecimal value only.
            return UString::Format(u"0x%0*X", hexa_digits, value);
        }
    }

    if (!(flags & (NamesFlags::NAME_VALUE | NamesFlags::VALUE_NAME))) {
        // Name only.
        return *display_name;
    }

    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(switch-enum) // enumeration values not explicitly handled in switch
    TS_MSC_NOWARNING(4061)         // enumerator in switch of enum is not explicitly handled by a case label

    switch (flags & (NamesFlags::VALUE_NAME | NamesFlags::DECIMAL | NamesFlags::HEXA)) {
        case NamesFlags::DECIMAL:
            return UString::Format(u"%s (%d)", *display_name, value);
        case NamesFlags::HEXA:
            return UString::Format(u"%s (0x%0*X)", *display_name, hexa_digits, value);
        case NamesFlags::HEXA | NamesFlags::DECIMAL:
            return UString::Format(u"%s (0x%0*X, %d)", *display_name, hexa_digits, value, value);
        case NamesFlags::DECIMAL | NamesFlags::VALUE_NAME:
            return UString::Format(u"%d (%s)", value, *display_name);
        case NamesFlags::HEXA | NamesFlags::VALUE_NAME:
            return UString::Format(u"0x%0*X (%s)", hexa_digits, value, *display_name);
        case NamesFlags::HEXA | NamesFlags::DECIMAL | NamesFlags::VALUE_NAME:
            return UString::Format(u"0x%0*X (%d, %s)", hexa_digits, value, value, *display_name);
        default:
            assert(false);
            return UString();
    }

    TS_POP_WARNING()
}


//----------------------------------------------------------------------------
// Visitors subscriptions
//----------------------------------------------------------------------------

void ts::Names::subscribe(Visitor* visitor)
{
    if (visitor != nullptr) {
        // Write lock (exclusive).
        std::lock_guard<std::shared_mutex> lock(_mutex);
        _visitors.insert(visitor);
    }
}

void ts::Names::unsubscribe(Visitor* visitor)
{
    if (visitor != nullptr) {
        // Write lock (exclusive).
        std::lock_guard<std::shared_mutex> lock(_mutex);
        _visitors.erase(visitor);
    }
}


//----------------------------------------------------------------------------
// Visit all values in this Names instance.
//----------------------------------------------------------------------------

size_t ts::Names::visit(Visitor* visitor) const
{
    // Trivial cose, nothing to visit.
    if (visitor == nullptr) {
        return 0;
    }

    size_t visit_count = 0;
    const Names* sec = this;
    NamesPtr sec_ptr;

    // Loop on inherited sections.
    for (int levels = MAX_INHERIT; levels > 0; --levels) {

        // Loop on all values in this section.
        {
            // Read lock (shared).
            std::shared_lock<std::shared_mutex> lock(sec->_mutex);
            for (const auto& ent : sec->_entries) {
                for (uint_t i = ent.second->first; i <= ent.second->last; ++i) {
                    visit_count++;
                    if (!visitor->handleNameValue(*sec, i, ent.second->name)) {
                        return visit_count;
                    }
                }
            }
        }

        // "Superclass" section name.
        if (sec->_inherit.empty()) {
            break;
        }
        sec_ptr = AllInstances::Instance().get(sec->_inherit);
        sec = sec_ptr.get();
    }
    return visit_count;
}

size_t ts::Names::visit(Visitor* visitor, uint_t value) const
{
    // Trivial cose, nothing to visit.
    if (visitor == nullptr) {
        return 0;
    }

    size_t visit_count = 0;
    const Names* sec = this;
    NamesPtr sec_ptr;

    // Loop on inherited sections.
    for (int levels = MAX_INHERIT; levels > 0; --levels) {

        {
            // Read lock (shared).
            std::shared_lock<std::shared_mutex> lock(sec->_mutex);

            // When "Extended=false" (the default), there is only one value, the short_entries multimap is empty.
            if (sec->_short_entries.empty()) {
                // Add the target value alone if it is registered.
                const auto range = getRangeLocked(value);
                if (range != nullptr) {
                    visit_count++;
                    if (!visitor->handleNameValue(*sec, value, range->name)) {
                        return visit_count;
                    }
                }
            }
            else {
                // There are extended values in short_entries.
                const uint_t increment = uint_t(1) << sec->_bits;
                const uint_t max = std::numeric_limits<uint_t>::max() - increment;

                // Get all values in the multimap for the base value.
                const auto bounds(sec->_short_entries.equal_range(value & sec->_mask));
                for (auto next = bounds.first; next != bounds.second; ++next) {
                    const auto& val(*next->second);
                    uint_t i = (val.first & ~sec->_mask) | (value & sec->_mask);
                    while (i <= val.last) {
                        visit_count++;
                        if (!visitor->handleNameValue(*sec, i, val.name)) {
                            return visit_count;
                        }
                        if (i > max) {
                            break; // avoid integer overflow
                        }
                        i += increment;
                    }
                }
            }
        }

        // "Superclass" section name.
        if (sec->_inherit.empty()) {
            break;
        }
        sec_ptr = AllInstances::Instance().get(sec->_inherit);
        sec = sec_ptr.get();
    }
    return visit_count;
}


//----------------------------------------------------------------------------
// Load a ".names" file and merge its content into all loaded Names instances.
//----------------------------------------------------------------------------

bool ts::Names::MergeFile(const UString& file_name)
{
    return AllInstances::Instance().loadFile(file_name);
}


//----------------------------------------------------------------------------
// The singleton which manages all named instances of Names.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::Names::AllInstances);

ts::Names::AllInstances::AllInstances()
{
}

// Load a file, if not already loaded, and create one Names instance per section.
bool ts::Names::AllInstances::loadFile(const UString& file_name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return loadFileLocked(file_name);
}

// Get or create a section.
ts::NamesPtr ts::Names::AllInstances::get(const UString& section_name, const UString& file_name)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!file_name.empty()) {
        // Ignore errors (logged on standard error)
        loadFileLocked(file_name);
    }
    return getLocked(section_name);
}

// Get or create a section with exclusive lock already held.
ts::NamesPtr ts::Names::AllInstances::getLocked(const UString& section_name)
{
    const UString sname(NormalizedSectionName(section_name));
    const auto it = _names.find(sname);
    if (it != _names.end()) {
        return it->second;
    }
    else {
        auto sec = _names[sname] = std::make_shared<Names>();
        sec->_section_name = section_name;
        return sec;
    }
}


//----------------------------------------------------------------------------
// Load a file with exclusive lock already held.
//----------------------------------------------------------------------------

bool ts::Names::AllInstances::loadFileLocked(const UString& file_name)
{
    // To speed up future lookups, we save in _loaded_files all forms of paths
    // for the file, including the common names without directory.
    if (_loaded_files.contains(file_name)) {
        return true;
    }

    // The file has not been recorded in its short form. Build the list of names
    // to record if the file is successfully loaded later.
    std::set<UString> names;
    names.insert(file_name);

    // If no directory is specified, try with ".names" extension and "tsduck." prefix.
    UString full_path(SearchConfigurationFile(file_name));
    if (full_path.empty() && !file_name.endsWith(u".names", CASE_INSENSITIVE)) {
        UString name2(file_name + u".names");
        names.insert(name2);
        full_path = SearchConfigurationFile(name2);
        if (full_path.empty() && !file_name.contains(u'/') && !file_name.contains(u'\\') && !name2.startsWith(u"tsduck.", CASE_INSENSITIVE)) {
            name2.insert(0, u"tsduck.");
            names.insert(name2);
            full_path = SearchConfigurationFile(name2);
        }
    }

    // Log error on stderr if no file is found.
    if (full_path.empty()) {
        CERR.error(u"configuration file '%s' not found", file_name);
        return false;
    }

    // Now we have an existing file and several possible names for it. Keep all names so that we won't try to reload it again.
    // If there are errors in the file, this won't change in a future reload (assuming that the file remains unchanged).
    _loaded_files.insert(names.begin(), names.end());
    _loaded_files.insert(full_path);

    std::ifstream strm(full_path.toUTF8().c_str());
    if (!strm) {
        CERR.error(u"error opening file %s", full_path);
        return false;
    }

    // Read configuration file line by line.
    UString line;
    std::set<UString> section_names;
    NamesPtr section;
    size_t error_count = 0;

    try {
        for (size_t line_number = 1; line.getLine(strm); ++line_number) {

            // Remove leading and trailing spaces in line.
            line.trim();

            if (line.empty() || line[0] == UChar('#')) {
                // Empty or comment line, ignore.
            }
            else if (line.front() == UChar('[') && line.back() == UChar(']')) {
                // Handle beginning of section, get section name.
                line.erase(0, 1);
                line.pop_back();
                section_names.insert(line);
                // Unlock previous section.
                if (section != nullptr) {
                    section->_mutex.unlock();
                }
                // Get or create associated section.
                section = getLocked(line);
                // Get write lock on this section (exclusive).
                section->_mutex.lock();
            }
            else if (!decodeDefinition(full_path, line, section)) {
                // Invalid line.
                CERR.error(u"%s: invalid line %d: %s", full_path, line_number, line);
                if (++error_count >= 20) {
                    // Give up after that number of errors
                    CERR.error(u"%s: too many errors, giving up", full_path);
                    break;
                }
            }
        }
    }
    catch (...) {
        // Unlock section before rethrowing.
        if (section != nullptr) {
            section->_mutex.unlock();
            section = nullptr;
        }
        throw;
    }
    // Unlock previous section.
    if (section != nullptr) {
        section->_mutex.unlock();
    }
    strm.close();

    // Verify that all sections have bits size.
    for (const auto& sname : section_names) {
        auto& sec(*getLocked(sname));

        // Fetch bits value from "superclasses".
        UString parent(sec._inherit);
        while (sec._bits == 0 && !parent.empty()) {
            auto next = _names.find(NormalizedSectionName(parent));
            if (next == _names.end()) {
                CERR.error(u"%d: section %s inherits from non-existent section %s", full_path, sname, parent);
                error_count++;
                break;
            }
            sec._bits = next->second->_bits;
            parent = next->second->_inherit;
        }

        // Verify the presence of bits size.
        if (sec._bits == 0) {
            CERR.error(u"%d: no specified bits size in section %s", full_path, sname);
            error_count++;
        }
        else {
            // Mask to extract the basic value, without the potential extension.
            sec._mask = LSBMask<uint_t>(sec._bits);

            // Verify the presence of extended values in the section.
            bool extended = false;
            {
                // Read lock (shared).
                std::shared_lock<std::shared_mutex> lock(sec._mutex);
                for (const auto& val : sec._entries) {
                    // Only check the extension in 'last', it is greated than 'first'.
                    if ((val.second->last & ~sec._mask) != 0) {
                        extended = true;
                        break;
                    }
                }
                if (extended != sec._has_extended) {
                    CERR.error(u"%d: section %s, extended is %s, found%s extended values", full_path, sname, sec._has_extended, extended ? u"" : u" no");
                    error_count++;
                }
            }

            // In the presence of extended values, build the 'short_entries' multimap, indexed by short values.
            if (extended) {
                assert(sec._bits < 8 * sizeof(uint_t));
                // Write lock (exclusive).
                std::lock_guard<std::shared_mutex> lock(sec._mutex);
                // If there are more than one value in the range, it is possible that they span multiple short values.
                const uint_t increment = uint_t(1) << sec._bits;
                const uint_t max = std::numeric_limits<uint_t>::max() - increment;
                for (const auto& val : sec._entries) {
                    uint_t index = val.second->first;
                    while (index <= val.second->last) {
                        sec._short_entries.insert(std::make_pair(index & sec._mask, val.second));
                        if (index > max) {
                            break; // avoid integer overflow
                        }
                        index += increment;
                    }
                }
            }
        }
    }

    return error_count == 0;
}


//----------------------------------------------------------------------------
// Decode a line as "first[-last] = name". Return true on success.
//----------------------------------------------------------------------------

bool ts::Names::AllInstances::decodeDefinition(const UString& file_name, const UString& line, NamesPtr section)
{
    // Check the presence of the '=' and in a valid section.
    const size_t equal = line.find(u'=');
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
        if (section->_bits > 0) {
            CERR.error(u"%s: section %s, duplicated bits clauses %d and %s", file_name, section->_section_name, section->_bits, value);
            return false;
        }
        else if (value.toInteger(bits, ignore, 0, UString()) && bits > 0 && bits <= 8 * sizeof(uint_t)) {
            section->_bits = bits;
            return true;
        }
        else {
            CERR.error(u"%s: section %s, invalid bits value; %s", file_name, section->_section_name, value);
            return false;
        }
    }
    else if (range.similar(u"inherit")) {
        // Name of a section where to search unknown values here.
        if (section->_inherit.empty()) {
            section->_inherit = value;
            return true;
        }
        else {
            CERR.error(u"%s: section %s, duplicated inherit clauses %s and %s", file_name, section->_section_name, section->_inherit, value);
            return false;
        }
    }
    else if (range.similar(u"extended")) {
        // "extended = true|false" indicates the presence of extended values, larger than the specified bit size.
        return value.toBool(section->_has_extended);
    }

    // Decode "first[-last]"
    uint_t first = 0;
    uint_t last = 0;
    const size_t dash = range.find(u'-');
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
        if (section->freeRangeLocked(first, last)) {
            // Valid range, add it.
            section->add(value, first, last);
        }
        else {
            CERR.error(u"%s: section %s, range 0x%X-0x%X overlaps with an existing range", file_name, section->_section_name, first, last);
            valid = false;
        }
    }
    return valid;
}
