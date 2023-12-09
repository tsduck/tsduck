//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsAlgorithm.h"
#include "tsIntegerUtils.h"
#include "tsFileUtils.h"
#include "tsVersionInfo.h"
#include "tsOutputPager.h"
#include "tsDuckConfigFile.h"

// List of characters which are allowed thousands separators and decimal points in integer values
const ts::UChar* const ts::Args::THOUSANDS_SEPARATORS = u", ";
const ts::UChar* const ts::Args::DECIMAL_POINTS = u".";

// Enumeration description of HelpFormat.
const ts::Enumeration ts::Args::HelpFormatEnum({
    {u"name",        ts::Args::HELP_NAME},
    {u"description", ts::Args::HELP_DESCRIPTION},
    {u"usage",       ts::Args::HELP_USAGE},
    {u"syntax",      ts::Args::HELP_SYNTAX},
    {u"full",        ts::Args::HELP_FULL},
    {u"options",     ts::Args::HELP_OPTIONS},
});


//----------------------------------------------------------------------------
// Constructors for IOption
//----------------------------------------------------------------------------

ts::Args::IOption::IOption(const UChar* name_,
                           UChar        short_name_,
                           ArgType      type_,
                           size_t       min_occur_,
                           size_t       max_occur_,
                           int64_t      min_value_,
                           int64_t      max_value_,
                           size_t       decimals_,
                           uint32_t     flags_,
                           AbstractNumber* anumber_) :

    name(name_ == nullptr ? UString() : name_),
    short_name(short_name_),
    type(type_),
    min_occur(min_occur_),
    max_occur(max_occur_),
    min_value(min_value_),
    max_value(max_value_),
    decimals(decimals_),
    flags(flags_),
    anumber(anumber_)
{
    // Provide default max_occur
    if (max_occur == 0) {
        max_occur = name.empty() ? UNLIMITED_COUNT : 1;
    }
    // Handle invalid values
    if (max_occur < min_occur) {
        throw ArgsError(u"invalid occurences for " + display());
    }
    // Parameters are values by definition
    if (name.empty() && type == NONE) {
        type = STRING;
    }
    // Normalize all integer types to INTEGER
    switch (type) {
        case NONE:
        case TRISTATE:
        case IPADDR:
        case IPSOCKADDR:
        case IPSOCKADDR_OA:
        case IPSOCKADDR_OP:
        case IPSOCKADDR_OAP:
            min_value = 0;
            max_value = 0;
            break;
        case STRING:
        case FILENAME:
        case DIRECTORY:
        case HEXADATA:
            // Min and max values will be converted to size_t, be sure to stay within limits.
            min_value = std::max<int64_t>(0, std::min(min_value, bounded_cast<int64_t>(std::numeric_limits<size_t>::max())));
            max_value = std::max<int64_t>(0, std::min(max_value, bounded_cast<int64_t>(std::numeric_limits<size_t>::max())));
            // Max length of zero means unbounded.
            if (max_value == 0) {
                max_value = bounded_cast<int64_t>(std::numeric_limits<size_t>::max());
            }
            TS_FALLTHROUGH
        case INTEGER:
        case ANUMBER:
            if (max_value < min_value) {
                throw ArgsError(u"invalid value range for " + display());
            }
            break;
        case UNSIGNED:
            min_value = 0;
            max_value = std::numeric_limits<int64_t>::max();
            type = INTEGER;
            break;
        case POSITIVE:
            min_value = 1;
            max_value = std::numeric_limits<int64_t>::max();
            type = INTEGER;
            break;
        case UINT8:
            min_value = 0;
            max_value = 0xFF;
            type = INTEGER;
            break;
        case UINT16:
            min_value = 0;
            max_value = 0xFFFF;
            type = INTEGER;
            break;
        case UINT32:
            min_value = 0;
            max_value = 0xFFFFFFFF;
            type = INTEGER;
            break;
        case UINT63:
            min_value = 0;
            max_value = std::numeric_limits<int64_t>::max(); // 63-bit unsigned in practice
            type = INTEGER;
            break;
        case PIDVAL:
            min_value = 0;
            max_value = 0x1FFF;
            type = INTEGER;
            break;
        case INT8:
            min_value = -128;
            max_value = 127;
            type = INTEGER;
            break;
        case INT16:
            min_value = -32768;
            max_value = 32767;
            type = INTEGER;
            break;
        case INT32:
            min_value = -0x80000000LL;
            max_value = 0x7FFFFFFF;
            type = INTEGER;
            break;
        case INT64:
            min_value = std::numeric_limits<int64_t>::min();
            max_value = std::numeric_limits<int64_t>::max();
            type = INTEGER;
            break;
        default:
            throw ArgsError(UString::Format(u"invalid option type %d", {type}));
    }
}

ts::Args::IOption::IOption(const UChar*       name_,
                           UChar              short_name_,
                           const Enumeration& enumeration_,
                           size_t             min_occur_,
                           size_t             max_occur_,
                           uint32_t           flags_) :

    name(name_ == nullptr ? UString() : name_),
    short_name(short_name_),
    type(INTEGER),
    min_occur(min_occur_),
    max_occur(max_occur_),
    min_value(std::numeric_limits<int>::min()),
    max_value(std::numeric_limits<int>::max()),
    flags(flags_),
    enumeration(enumeration_)
{
    // Provide default max_occur
    if (max_occur == 0) {
        max_occur = name.empty() ? UNLIMITED_COUNT : 1;
    }
    // Handle invalid values
    if (max_occur < min_occur) {
        throw ArgsError(u"invalid occurences for " + display());
    }
}


//----------------------------------------------------------------------------
// Displayable name for IOption
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::display() const
{
    UString plural(min_occur > 1 ? u"s" : u"");
    if (name.empty()) {
        return u"parameter" + plural;
    }
    else {
        UString n;
        if (short_name != 0) {
            n = u" (-";
            n += short_name;
            n += u')';
        }
        return u"option" + plural + u" --" + name + n;
    }
}


//----------------------------------------------------------------------------
// Description of the option value.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::valueDescription(ValueContext ctx) const
{
    UString desc(syntax);
    if (syntax.empty()) {
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(switch-enum)
        TS_MSC_NOWARNING(4061)
        switch (type) {
            case NONE:      break;
            case FILENAME:  desc = u"file-name"; break;
            case DIRECTORY: desc = u"directory-name"; break;
            case HEXADATA:  desc = u"hexa-data"; break;
            case IPADDR:    desc = u"ip-address"; break;
            case IPSOCKADDR:     desc = u"ip-address:port"; break;
            case IPSOCKADDR_OA:  desc = u"[ip-address:]port"; break;
            case IPSOCKADDR_OP:  desc = u"ip-address[:port]"; break;
            case IPSOCKADDR_OAP: desc = u"[ip-address]:[port]"; break;
            default:             desc = u"value"; break;
        }
        TS_POP_WARNING()
    }

    if (type == NONE || (flags & (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) == (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) {
        // No value or value is optional and shall not be documented.
        return UString();
    }
    else if ((flags & IOPT_OPTVALUE) != 0) {
        return (ctx == LONG ? u"[=" : u"[") + desc + u"]";
    }
    else if (ctx == ALONE) {
        return desc;
    }
    else {
        return SPACE + desc;
    }
}


//----------------------------------------------------------------------------
// When the option has an Enumeration type, get a list of all valid names.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::optionNames(const UString& separator) const
{
    UStringList names;
    enumeration.getAllNames(names);
    names.sort([](const UString& s1, const UString& s2) { return s1.superCompare(s2, SCOMP_IGNORE_BLANKS | SCOMP_CASE_INSENSITIVE | SCOMP_NUMERIC) < 0; });
    for (auto& n : names) {
        n.insert(0, 1, u'"');
        n.append(u'"');
    }
    return UString::Join(names, separator);
}


//----------------------------------------------------------------------------
// Option type, as used in --help=options.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::optionType() const
{
    UString desc;
    if (type != NONE && (flags & IOPT_OPTVALUE) != 0) {
        desc += u":opt";
    }
    switch (type) {
        case INTEGER:
        case UNSIGNED:
        case POSITIVE:
        case UINT8:
        case UINT16:
        case UINT32:
        case UINT63:
        case PIDVAL:
        case INT8:
        case INT16:
        case INT32:
        case INT64:
            if (enumeration.empty()) {
                desc += u":int";
            }
            else {
                desc += u":enum:";
                desc += enumeration.nameList(u",");
            }
            break;
        case TRISTATE:
            desc += u":enum:true,false,unknown";
            break;
        case ANUMBER:
            desc += u":number";
            break;
        case STRING:
            desc += u":string";
            break;
        case FILENAME:
            desc += u":file";
            break;
        case DIRECTORY:
            desc += u":directory";
            break;
        case HEXADATA:
            desc += u":hexadata";
            break;
        case IPADDR:
            desc += u":ipaddress";
            break;
        case IPSOCKADDR:
        case IPSOCKADDR_OA:
        case IPSOCKADDR_OP:
        case IPSOCKADDR_OAP:
            desc += u":ipsocket";
            break;
        case NONE:
            desc += u":bool";
            break;
        default:
            break;
    }
    return desc;
}


//----------------------------------------------------------------------------
// Complete option help text.
//----------------------------------------------------------------------------

ts::UString ts::Args::IOption::helpText(size_t line_width) const
{
    IndentationContext indent_desc = TITLE;
    UString text;

    // Add option / parameter name.
    if (name.empty()) {
        // This is the parameters (ie. not options).
        indent_desc = PARAMETER_DESC;
        // Print nothing if parameters are undocumented.
        if (help.empty() && syntax.empty()) {
            return UString();
        }
        // Print generic title instead of option names.
        text += HelpLines(TITLE, max_occur <= 1 ? u"Parameter:" : u"Parameters:", line_width);
        text += LINE_FEED;
    }
    else {
        // This is an option.
        indent_desc = OPTION_DESC;
        if (short_name != 0) {
            text += HelpLines(OPTION_NAME, UString::Format(u"-%c%s", {short_name, valueDescription(IOption::SHORT)}), line_width);
        }
        text += HelpLines(OPTION_NAME, UString::Format(u"--%s%s", {name, valueDescription(IOption::LONG)}), line_width);
    }

    // Add option description.
    if (!help.empty()) {
        text += HelpLines(indent_desc, help, line_width);
    }
    else if (name.empty() && !syntax.empty()) {
        // For parameters (no option name previously displayed), use syntax as fallback for help.
        text += HelpLines(indent_desc, syntax, line_width);
    }

    // Document all possible values for enumeration types.
    if (!enumeration.empty() && (flags & (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) != (IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP)) {
        text += HelpLines(indent_desc, u"Must be one of " + optionNames(u", ") + u".", line_width);
    }

    // Document decimal values (with a decimal point).
    if (decimals > 0) {
        text += HelpLines(indent_desc, UString::Format(u"The value may include up to %d meaningful decimal digits.", {decimals}), line_width);
    }
    if (type == ANUMBER && !anumber.isNull()) {
        const UString desc(anumber->description());
        if (!desc.empty()) {
            text += HelpLines(indent_desc, UString::Format(u"The value must be a %s.", {desc}), line_width);
        }
    }

    return text;
}


//----------------------------------------------------------------------------
// Constructor for Args
//----------------------------------------------------------------------------

ts::Args::Args(const UString& description, const UString& syntax, int flags) :
    Report(),
    _saved_severity(maxSeverity()),
    _description(description),
    _syntax(syntax),
    _flags(flags)
{
    adjustPredefinedOptions();
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

void ts::Args::setDescription(const UString& description)
{
    _description = description;
}

void ts::Args::setSyntax(const UString& syntax)
{
    _syntax = syntax;
}

void ts::Args::setIntro(const UString& intro)
{
    _intro = intro;
}

void ts::Args::setTail(const UString& tail)
{
    _tail = tail;
}

void ts::Args::setFlags(int flags)
{
    _flags = flags;
    adjustPredefinedOptions();
}


//----------------------------------------------------------------------------
// Adjust predefined options based on flags.
//----------------------------------------------------------------------------

void ts::Args::adjustPredefinedOptions()
{
    // Option --help[=value].
    if ((_flags & NO_HELP) != 0) {
        _iopts.erase(u"help");
    }
    else if (!Contains(_iopts, u"help")) {
        addOption(IOption(u"help", 0, HelpFormatEnum, 0, 1, IOPT_PREDEFINED | IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP));
        help(u"help", u"Display this help text.");
    }

    // Option --version[=value].
    if ((_flags & NO_VERSION) != 0) {
        _iopts.erase(u"version");
    }
    else if (!Contains(_iopts, u"version")) {
        addOption(IOption(u"version", 0,  VersionInfo::FormatEnum, 0, 1, IOPT_PREDEFINED | IOPT_OPTVALUE | IOPT_OPTVAL_NOHELP));
        help(u"version", u"Display the TSDuck version number.");
    }

    // Option --verbose.
    if ((_flags & NO_VERBOSE) != 0) {
        _iopts.erase(u"verbose");
    }
    else if (!Contains(_iopts, u"verbose")) {
        addOption(IOption(u"verbose", 'v', NONE, 0, 1, 0, 0, 0, IOPT_PREDEFINED));
        help(u"verbose", u"Produce verbose output.");
    }

    // Option --debug[=value].
    if ((_flags & NO_DEBUG) != 0) {
        _iopts.erase(u"debug");
    }
    else if (!Contains(_iopts, u"debug")) {
        addOption(IOption(u"debug", 'd', POSITIVE, 0, 1, 0, 0, 0, IOPT_PREDEFINED | IOPT_OPTVALUE));
        help(u"debug", u"level", u"Produce debug traces. The default level is 1. Higher levels produce more messages.");
    }
}


//----------------------------------------------------------------------------
// Format help lines from a long text.
//----------------------------------------------------------------------------

ts::UString ts::Args::HelpLines(IndentationContext level, const UString& text, size_t line_width)
{
    // Actual indentation width.
    size_t indent = 0;
    if (level == PARAMETER_DESC || level == OPTION_NAME) {
        indent = 2;
    }
    else if (level == OPTION_DESC) {
        indent = 6;
    }

    // Format the line.
    const UString margin(indent, SPACE);
    return (margin + text.toTrimmed()).toSplitLines(line_width, u".,;:", margin) + u"\n";
}


//----------------------------------------------------------------------------
// Format the help options of the command.
//----------------------------------------------------------------------------

ts::UString ts::Args::formatHelpOptions(size_t line_width) const
{
    UString text;

    // Set introduction text.
    if (!_intro.empty()) {
        text = HelpLines(TITLE, _intro, line_width);
    }

    // Build a descriptive string from individual options.
    bool titleDone = false;
    for (auto& it : _iopts) {
        const IOption& opt(it.second);
        if (!text.empty()) {
            text += LINE_FEED;
        }
        // When this is an option, add 'Options:' the first time.
        if (!titleDone && !opt.name.empty()) {
            titleDone = true;
            text += HelpLines(TITLE, u"Options:", line_width);
            text += LINE_FEED;
        }
        text += opt.helpText(line_width);
    }

    // Set final text.
    if (!_tail.empty()) {
        text += LINE_FEED;
        text.append(HelpLines(TITLE, _tail, line_width));
    }
    return text;
}


//----------------------------------------------------------------------------
// Add a new option.
//----------------------------------------------------------------------------

void ts::Args::addOption(const IOption& opt)
{
    // Erase previous version, if any.
    _iopts.erase(opt.name);

    // If the new option has a short name, erase previous options with same short name.
    if (opt.short_name != 0) {
        for (auto& it : _iopts) {
            if (it.second.short_name == opt.short_name) {
                it.second.short_name = 0;
                break; // there was at most one
            }
        }
    }

    // Finally add the new option.
    _iopts.insert(std::make_pair(opt.name, opt));
}


//----------------------------------------------------------------------------
// Add the help text of an exiting option.
//----------------------------------------------------------------------------

ts::Args& ts::Args::help(const UChar* name, const UString& syntax, const UString& text)
{
    IOption& opt(getIOption(name));
    opt.syntax = syntax;
    opt.help = text;
    return *this;
}


//----------------------------------------------------------------------------
// When an option has an Enumeration type, get a list of all valid names.
//----------------------------------------------------------------------------

ts::UString ts::Args::optionNames(const ts::UChar* name, const ts::UString& separator) const
{
    const IOption& opt(getIOption(name));
    return opt.optionNames(separator);
}


//----------------------------------------------------------------------------
// Copy all option definitions from another Args object. Return this object.
// If override is true, override duplicated options.
//----------------------------------------------------------------------------

ts::Args& ts::Args::copyOptions(const Args& other, const bool replace)
{
    for (auto& it : other._iopts) {
        if ((it.second.flags & IOPT_PREDEFINED) == 0 && (replace || !Contains(_iopts, it.second.name))) {
            addOption(it.second);
        }
    }
    return *this;
}


//----------------------------------------------------------------------------
// Redirect report logging. Redirection cancelled if zero.
//----------------------------------------------------------------------------

ts::Report* ts::Args::redirectReport(Report* rep)
{
    // When leaving the default report, save the severity.
    if (_subreport == nullptr) {
        _saved_severity = this->maxSeverity();
    }

    // Switch report.
    Report* previous = _subreport;
    _subreport = rep;

    // Adjust severity.
    this->setMaxSeverity(rep == nullptr ? _saved_severity : rep->maxSeverity());

    return previous;
}


//----------------------------------------------------------------------------
// Adjust debug level, always increase verbosity, never decrease.
//----------------------------------------------------------------------------

void ts::Args::raiseMaxSeverity(int level)
{
    // Propagate to superclass (for this object).
    Report::raiseMaxSeverity(level);

    // Propagate to redirected report, if one is set.
    if (_subreport != nullptr) {
        _subreport->raiseMaxSeverity(level);
    }
}


//----------------------------------------------------------------------------
// Display an error message, as if it was produced during command line analysis.
//----------------------------------------------------------------------------

void ts::Args::writeLog(int severity, const UString& message)
{
    // Process error message if flag NO_ERROR_DISPLAY it not set.
    if ((_flags & NO_ERROR_DISPLAY) == 0) {
        if (_subreport != nullptr) {
            _subreport->log(severity, message);
        }
        else {
            if (severity < Severity::Info) {
                std::cerr << _app_name << ": ";
            }
            else if (severity > Severity::Verbose) {
                std::cerr << _app_name << ": " << Severity::Header(severity);
            }
            std::cerr << message << std::endl;
        }
    }

    // Mark this instance as error if severity <= Severity::Error.
    _is_valid = _is_valid && severity > Severity::Error;

    // Immediately abort application is severity == Severity::Fatal.
    if (severity == Severity::Fatal) {
        std::exit(EXIT_FAILURE);
    }
}


//----------------------------------------------------------------------------
// Exit application when errors were reported.
//----------------------------------------------------------------------------

void ts::Args::exitOnError(bool force)
{
    if (!_is_valid && (force || (_flags & NO_EXIT_ON_ERROR) == 0)) {
        std::exit(EXIT_FAILURE);
    }
}


//----------------------------------------------------------------------------
// Locate an option description. Return 0 if not found
//----------------------------------------------------------------------------

ts::Args::IOption* ts::Args::search(UChar c)
{
    for (auto& it : _iopts) {
        if (it.second.short_name == c) {
            return &it.second;
        }
    }
    error(UString::Format(u"unknown option -%c", {c}));
    return nullptr;
}


//----------------------------------------------------------------------------
// Locate an option description. Return 0 if not found
//----------------------------------------------------------------------------

ts::Args::IOption* ts::Args::search(const UString& name)
{
    IOption* previous = nullptr;

    for (auto& it : _iopts) {
        if (it.second.name == name) {
            // found an exact match
            return &it.second;
        }
        else if (!name.empty() && it.second.name.find(name) == 0) {
            // found an abbreviated version
            if (previous == nullptr) {
                // remember this one and continue searching
                previous = &it.second;
            }
            else {
                // another one already found, ambiguous option
                error(u"ambiguous option --" + name + u" (--" + previous->name + u", --" + it.second.name + u")");
                return nullptr;
            }
        }
    }

    if (previous != nullptr) {
        // exactly one abbreviation was found
        return previous;
    }
    else if (name.empty()) {
        error(u"no parameter allowed, use options only");
        return nullptr;
    }
    else {
        error(u"unknown option --" + name);
        return nullptr;
    }
}


//----------------------------------------------------------------------------
// Locate an IOption based on its complete long name.
// Throw ArgsError if option does not exist (application internal error)
//----------------------------------------------------------------------------

ts::Args::IOption& ts::Args::getIOption(const UChar* name)
{
    const UString name1(name == nullptr ? u"" : name);
    auto it = _iopts.find(name1);
    if (it != _iopts.end()) {
        return it->second;
    }
    else {
        throw ArgsError(_app_name + u": application internal error, option --" + name1 + u" undefined");
    }
}

const ts::Args::IOption& ts::Args::getIOption(const UChar* name) const
{
    // The non-const version above does not modify the object,
    // it just return a non-const reference.
    return const_cast<Args*>(this)->getIOption(name);
}


//----------------------------------------------------------------------------
// Check if option is present
//----------------------------------------------------------------------------

bool ts::Args::present(const UChar* name) const
{
    return !getIOption(name).values.empty();
}


//----------------------------------------------------------------------------
// Check the number of occurences of the option.
//----------------------------------------------------------------------------

size_t ts::Args::count(const UChar* name) const
{
    return getIOption(name).value_count;
}


//----------------------------------------------------------------------------
// Get the value of an option. The index designates the occurence of
// the option. If the option is not present, or not with this
// occurence, def_value is returned.
//----------------------------------------------------------------------------

ts::UString ts::Args::value(const UChar* name, const UChar* def_value, size_t index) const
{
    UString v;
    getValue(v, name, def_value, index);
    return v;
}

void ts::Args::getValue(UString& value, const UChar* name, const UChar* def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type == INTEGER) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is integer, cannot be accessed as string");
    }
    else if (index >= opt.values.size() || !opt.values[index].string.has_value()) {
        value= def_value;
    }
    else {
        value = opt.values[index].string.value();
    }
}

void ts::Args::getOptionalValue(std::optional<UString>& value, const UChar* name, bool clear_if_absent) const
{
    const IOption& opt(getIOption(name));
    if (opt.type == INTEGER) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is integer, cannot be accessed as string");
    }
    else if (!opt.values.empty() && opt.values[0].string.has_value()) {
        value = opt.values[0].string;
    }
    else if (clear_if_absent) {
        value.reset();
    }
}

void ts::Args::getPathValue(fs::path& value, const UChar* name, const fs::path& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != FILENAME && opt.type != DIRECTORY) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is not a filesystem path");
    }
    else if (index >= opt.values.size() || !opt.values[index].string.has_value()) {
        value = def_value;
    }
    else {
        value = fs::path(opt.values[index].string.value());
    }
}


//----------------------------------------------------------------------------
// Get the value of tristate option
//----------------------------------------------------------------------------

void ts::Args::getTristateValue(Tristate& value, const UChar* name, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type == INTEGER) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is integer, cannot be accessed as tristate");
    }
    if (index >= opt.values.size()) {
        // Option not present, meaning unspecified.
        value = Tristate::Maybe;
    }
    else if (!opt.values[index].string.has_value()) {
        // Option present without value, meaning true.
        value = Tristate::True;
    }
    else if (!opt.values[index].string.value().toTristate(value)) {
        // Value present but not a valid tristate value. Should not occur if the
        // option was declared using TRISTATE type. So, this must be some string
        // option and we cannot decide the Tristate value.
        value = Tristate::Maybe;
    }
}

ts::Tristate ts::Args::tristateValue(const UChar* name, size_t index) const
{
    Tristate value = Tristate::Maybe;
    getTristateValue(value, name, index);
    return value;
}


//----------------------------------------------------------------------------
// Get the value of an hexadecimal option.
//----------------------------------------------------------------------------

void ts::Args::getHexaValue(ByteBlock& value, const UChar* name, const ByteBlock& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != STRING && opt.type != HEXADATA) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is not declared as string or hexa string");
    }
    if (index >= opt.values.size() || !opt.values[index].string.has_value()) {
        value = def_value;
    }
    else {
        opt.values[index].string.value().hexaDecode(value);
    }
}

ts::ByteBlock ts::Args::hexaValue(const UChar* name, const ByteBlock& def_value, size_t index) const
{
    ByteBlock value;
    getHexaValue(value, name, def_value, index);
    return value;
}


//----------------------------------------------------------------------------
// Get the value of an option as an IPv4 address or socket address.
//----------------------------------------------------------------------------

void ts::Args::getIPValue(IPv4Address& value, const UChar* name, const IPv4Address& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != IPADDR && opt.type != IPSOCKADDR && opt.type != IPSOCKADDR_OA && opt.type != IPSOCKADDR_OP && opt.type != IPSOCKADDR_OAP) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is not declared as IPv4 address");
    }
    value = index >= opt.values.size() ? def_value : opt.values[index].address;
    if (!value.hasAddress() && def_value.hasAddress()) {
        value.setAddress(def_value.address());
    }
}

ts::IPv4Address ts::Args::ipValue(const UChar* name, const IPv4Address& def_value, size_t index) const
{
    IPv4Address value;
    getIPValue(value, name, def_value, index);
    return value;
}

void ts::Args::getSocketValue(IPv4SocketAddress& value, const UChar* name, const IPv4SocketAddress& def_value, size_t index) const
{
    const IOption& opt(getIOption(name));
    if (opt.type != IPSOCKADDR && opt.type != IPSOCKADDR_OA && opt.type != IPSOCKADDR_OP && opt.type != IPSOCKADDR_OAP) {
        throw ArgsError(_app_name + u": application internal error, option --" + opt.name + u" is not declared as IPv4 socket address");
    }
    value = index >= opt.values.size() ? def_value : opt.values[index].address;
    if (!value.hasAddress() && def_value.hasAddress()) {
        value.setAddress(def_value.address());
    }
    if (!value.hasPort() && def_value.hasPort()) {
        value.setPort(def_value.port());
    }
}

ts::IPv4SocketAddress ts::Args::socketValue(const UChar* name, const IPv4SocketAddress& def_value, size_t index) const
{
    IPv4SocketAddress value;
    getSocketValue(value, name, def_value, index);
    return value;
}


//----------------------------------------------------------------------------
// Get the full command line from the last command line analysis.
//----------------------------------------------------------------------------

ts::UString ts::Args::commandLine() const
{
    UString line(_app_name.toQuoted());
    if (!_args.empty()) {
        line.append(SPACE);
        line.append(UString::ToQuotedLine(_args));
    }
    return line;
}


//----------------------------------------------------------------------------
// Get the application name from a standard argc/argv pair.
//----------------------------------------------------------------------------

ts::UString ts::Args::GetAppName(int argc, char* argv[])
{
    return argc < 1 || argv == nullptr ? UString() : BaseName(UString::FromUTF8(argv[0]), EXECUTABLE_FILE_SUFFIX);
}


//----------------------------------------------------------------------------
// Load arguments and analyze them, overloads.
//----------------------------------------------------------------------------

bool ts::Args::analyze(const UString& command, bool processRedirections)
{
    UString app;
    UStringVector args;
    command.fromQuotedLine(args);
    if (!args.empty()) {
        app = args.front();
        args.erase(args.begin());
    }
    return analyze(app, args, processRedirections);
}

bool ts::Args::analyze(int argc, char* argv[], bool processRedirections)
{
    UStringVector args;
    if (argc > 0) {
        UString::Assign(args, argc - 1, argv + 1);
    }
    return analyze(GetAppName(argc, argv), args, processRedirections);
}


//----------------------------------------------------------------------------
// Common code: analyze the command line.
//----------------------------------------------------------------------------

bool ts::Args::analyze(const UString& app_name, const UStringVector& arguments, bool processRedirections)
{
    // Save command line and arguments.
    _app_name = app_name;
    _args = arguments;

    // Clear previous values
    for (auto& it : _iopts) {
        it.second.values.clear();
        it.second.value_count = 0;
   }

    // Process default arguments from configuration file.
    if ((_flags & NO_CONFIG_FILE) == 0) {
        // Prepend and append default options.
        UStringVector pre;
        UStringVector post;
        DuckConfigFile::Instance().value(u"prepend.options").splitShellStyle(pre);
        DuckConfigFile::Instance().value(u"append.options").splitShellStyle(post);
        _args.insert(_args.begin(), pre.begin(), pre.end());
        _args.insert(_args.end(), post.begin(), post.end());

        // Default arguments if there is none.
        if (_args.empty()) {
            DuckConfigFile::Instance().value(u"default.options").splitShellStyle(_args);
        }
    }

    // Process redirections.
    _is_valid = !processRedirections || processArgsRedirection(_args);

    // Process argument list
    size_t next_arg = 0;            // Index of next arg to process
    size_t short_opt_arg = NPOS;    // Index of arg containing short options
    size_t short_opt_index = NPOS;  // Short option index in _args[short_opt_arg]
    bool force_parameters = false;  // Force all items to be parameters

    while (_is_valid && (short_opt_arg != NPOS || next_arg < _args.size())) {

        IOption* opt = nullptr;
        std::optional<UString> val;

        // Locate option name and value
        if (short_opt_arg != NPOS) {
            // Analyzing several short options in a string
            opt = search(_args[short_opt_arg][short_opt_index++]);
            if (short_opt_index >= _args[short_opt_arg].length()) {
                // Reached end of short option string
                short_opt_arg = NPOS;
                short_opt_index = NPOS;
            }
        }
        else if (force_parameters || _args[next_arg].size() < 2 || _args[next_arg][0] != u'-') {
            // Arg is a parameter (can be empty or '-' alone).
            if ((opt = search(u"")) == nullptr) {
                ++next_arg;
            }
            force_parameters = (_flags & GATHER_PARAMETERS) != 0;
        }
        else if (_args[next_arg].length() == 1) {
            // Arg is '-', next arg is a parameter, even if it starts with '-'
            ++next_arg;
            if ((opt = search(u"")) == nullptr) {
                ++next_arg;
            }
        }
        else if (_args[next_arg][1] == '-') {
            // Arg starts with '--', this is a long option
            size_t equal = _args[next_arg].find('=');
            if (equal != NPOS) {
                // Value is in the same arg: --option=value
                opt = search(_args[next_arg].substr(2, equal - 2));
                val = _args[next_arg].substr(equal + 1);
            }
            else {
                // Simple form: --option
                opt = search(_args[next_arg].substr(2));
            }
            ++next_arg;
        }
        else {
            // Arg starts with one single '-'
            opt = search(_args[next_arg][1]);
            if (_args[next_arg].length() > 2) {
                // More short options or value in arg
                short_opt_arg = next_arg;
                short_opt_index = 2;
            }
            ++next_arg;
        }

        // If IOption found...
        if (opt != nullptr) {
            // Get the value string from short option, if present
            if (short_opt_arg != NPOS && opt->type != NONE) {
                assert(!val.has_value());
                // Get the value from the rest of the short option string
                val = _args[short_opt_arg].substr(short_opt_index);
                short_opt_arg = NPOS;
                short_opt_index = NPOS;
            }

            // Check presence of mandatory values in next arg if not already found
            if (!val.has_value() && opt->type != NONE && (opt->flags & IOPT_OPTVALUE) == 0 && next_arg < _args.size()) {
                val = _args[next_arg++];
            }

            // Validate option value.
            validateParameter(*opt, val);
        }
    }

    // Process --verbose predefined option
    if ((_flags & NO_VERBOSE) == 0 && present(u"verbose") && (search(u"verbose")->flags & IOPT_PREDEFINED) != 0) {
        raiseMaxSeverity(Severity::Verbose);
    }

    // Process --debug predefined option
    if ((_flags & NO_DEBUG) == 0 && present(u"debug") && (search(u"debug")->flags & IOPT_PREDEFINED) != 0) {
        raiseMaxSeverity(intValue(u"debug", Severity::Debug));
    }

    // Display the analyzed command line. Do it outside the previous condition (checking for --debug) if the debug
    // mode was set outside this analysis (typically debug mode set at command level and propagated to plugins).
    if (debug()) {
        debug(u"====> %s%s%s %s", {_shell, _shell.empty() ? u"" : u" ", _app_name.toQuoted(), UString::ToQuotedLine(_args)});
    }

    // Process --help predefined option
    if ((_flags & NO_HELP) == 0 && present(u"help") && (search(u"help")->flags & IOPT_PREDEFINED) != 0) {
        processHelp();
        _is_valid = false;
        return false;
    }

    // Process --version predefined option
    if ((_flags & NO_VERSION) == 0 && present(u"version") && (search(u"version")->flags & IOPT_PREDEFINED) != 0) {
        processVersion();
        _is_valid = false;
        return false;
    }

    // Look for parameters/options number of occurences.
    // Don't do that if command already proven wrong.
    if (_is_valid) {
        for (auto& it : _iopts) {
            const IOption& op(it.second);
            // Don't check number of occurences when the option has no value.
            // Specifying such an option multiple times is the same as once.
            if (op.type != NONE) {
                if (op.value_count < op.min_occur) {
                    error(u"missing " + op.display() + (op.min_occur < 2 ? u"" : UString::Format(u", %d required", {op.min_occur})));
                }
                else if (op.value_count > op.max_occur) {
                    error(u"too many " + op.display() + (op.max_occur < 2 ? u"" : UString::Format(u", %d maximum", {op.max_occur})));
                }
            }
        }
    }

    // In case of error, exit
    exitOnError();

    return _is_valid;
}


//----------------------------------------------------------------------------
// Validate the content of an option, add the value.
//----------------------------------------------------------------------------

bool ts::Args::validateParameter(IOption& opt, const std::optional<UString>& val)
{
    int64_t last = 0;
    size_t point = NPOS;

    // Build the argument value.
    ArgValue arg;
    arg.string = val;

    if (opt.type == NONE) {
        // There should be no value, this is a flag without value.
        if (val.has_value()) {
            // In the case --option=value
            error(u"no value allowed for %s", {opt.display()});
            return false;
        }
    }
    else if (!val.has_value()) {
        // No value set, must be an optional value.
        if ((opt.flags & IOPT_OPTVALUE) == 0) {
            error(u"missing value for %s", {opt.display()});
            return false;
        }
    }
    else if (opt.type == TRISTATE) {
        Tristate t;
        if (!val.value().toTristate(t)) {
            error(u"invalid value %s for %s, use one of %s", {val.value(), opt.display(), UString::TristateNamesList()});
            return false;
        }
    }
    else if (opt.type == ANUMBER) {
        // We keep the arg as a string value after validation and will parse it again when the value is queried later.
        if (opt.anumber.isNull()) {
            error(u"internal error, option %s has no abstract number instance for validation", {opt.display()});
            return false;
        }
        else if (!opt.anumber->fromString(val.value())) {
            error(u"invalid value %s for %s", {val.value(), opt.display()});
            return false;
        }
        else if (!opt.anumber->inRange(opt.min_value, opt.max_value)) {
            error(u"value for %s must be in range %'d to %'d", {opt.display(), opt.min_value, opt.max_value});
            return false;
        }
    }
    else if (opt.type == STRING) {
        if (val.value().size() < size_t(opt.min_value)) {
            error(u"invalid size %d for %s, must be at least %d characters", {val.value().size(), opt.display(), opt.min_value});
            return false;
        }
        if (val.value().size() > size_t(opt.max_value)) {
            error(u"invalid size %d for %s, must be at most %d characters", {val.value().size(), opt.display(), opt.max_value});
            return false;
        }
    }
    else if (opt.type == HEXADATA) {
        // We keep the arg as a string value after validation and will parse it again when the value is queried later.
        ByteBlock data;
        if (!val.value().hexaDecode(data)) {
            error(u"invalid hexadecimal value '%s' for %s", {val.value(), opt.display()});
            return false;
        }
        if (data.size() < size_t(opt.min_value)) {
            error(u"invalid size %d for %s, must be at least %d bytes", {data.size(), opt.display(), opt.min_value});
            return false;
        }
        if (data.size() > size_t(opt.max_value)) {
            error(u"invalid size %d for %s, must be at most %d bytes", {data.size(), opt.display(), opt.max_value});
            return false;
        }
    }
    else if (opt.type == IPADDR) {
        IPv4Address addr;
        if (!addr.resolve(val.value(), *this)) {
            return false;
        }
        arg.address.setAddress(addr);
        arg.address.setPort(0);
    }
    else if (opt.type == IPSOCKADDR || opt.type == IPSOCKADDR_OA || opt.type == IPSOCKADDR_OP || opt.type == IPSOCKADDR_OAP) {
        if (!arg.address.resolve(val.value(), *this)) {
            return false;
        }
        if (!arg.address.hasAddress() && opt.type != IPSOCKADDR_OA && opt.type != IPSOCKADDR_OAP) {
            error(u"mandatory IP address is missing in %s, use ip-address:port", {val.value()});
            return false;
        }
        if (!arg.address.hasPort() && opt.type != IPSOCKADDR_OP && opt.type != IPSOCKADDR_OAP) {
            error(u"mandatory port number is missing in %s, use ip-address:port", {val.value()});
            return false;
        }
    }
    else if (opt.type != INTEGER) {
        // These cases must have been previously eliminated.
        assert(opt.type != UNSIGNED);
        assert(opt.type != POSITIVE);
        assert(opt.type != UINT8);
        assert(opt.type != UINT16);
        assert(opt.type != UINT32);
        assert(opt.type != PIDVAL);
        assert(opt.type != INT8);
        assert(opt.type != INT16);
        assert(opt.type != INT32);
    }
    else if (!opt.enumeration.empty()) {
        // Enumeration value expected, get corresponding integer value (not case sensitive)
        int i = opt.enumeration.value(val.value(), false);
        if (i == Enumeration::UNKNOWN) {
            error(u"invalid value %s for %s, use one of %s", {val.value(), opt.display(), optionNames(opt.name.c_str())});
            return false;
        }
        // Replace with actual integer value.
        arg.int_base = i;
        arg.int_count = 1;
    }
    else if (val.value().toInteger(arg.int_base, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS)) {
        // Found exactly one integer value.
        arg.int_count = 1;
    }
    else if ((point = val.value().find(u'-')) != NPOS &&
             point + 1 < val.value().size() &&
             val.value().substr(0, point).toInteger(arg.int_base, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS) &&
             val.value().substr(point + 1).toInteger(last, THOUSANDS_SEPARATORS, opt.decimals, DECIMAL_POINTS))
    {
        // Found one range of integer values.
        if (last < arg.int_base) {
            error(u"invalid range of integer values \"%s\" for %s", {val.value(), opt.display()});
            return false;
        }
        arg.int_count = size_t(last + 1 - arg.int_base);
    }
    else {
        error(u"invalid integer value %s for %s", {val.value(), opt.display()});
        return false;
    }

    // Check validity of integer values.
    if (opt.type == INTEGER && arg.int_count > 0) {
        if (arg.int_base < opt.min_value) {
            error(u"value for %s must be >= %'d", {opt.display(), opt.min_value});
            return false;
        }
        else if (arg.int_base + int64_t(arg.int_count) - 1 > opt.max_value) {
            error(u"value for %s must be <= %'d", {opt.display(), opt.max_value});
            return false;
        }
    }

    // Push value. For optional parameters without value, an unset variable is pushed.
    opt.values.push_back(arg);

    // Add the number of occurences. Can be more than one in case of integer range.
    opt.value_count += opt.type == INTEGER && arg.int_count > 0 ? arg.int_count : 1;

    return true;
}


//----------------------------------------------------------------------------
// Get a formatted help text.
//----------------------------------------------------------------------------

ts::UString ts::Args::getHelpText(HelpFormat format, size_t line_width) const
{
    switch (format) {
        case HELP_NAME: {
            // Return the application name as set by the application.
            return _app_name;
        }
        case HELP_DESCRIPTION: {
            // Return the descripton string as set by the application.
            return _description;
        }
        case HELP_USAGE: {
            // Return the usage string with application name and syntax.
            UString text;
            if (!_shell.empty()) {
                text.append(_shell);
                text.append(SPACE);
            }
            text.append(_app_name);
            if (!_syntax.empty()) {
                text.append(SPACE);
                text.append(_syntax);
            }
            return text;
        }
        case HELP_SYNTAX: {
            // Same as usage but on one line.
            UString str(getHelpText(HELP_USAGE, line_width));
            // Replace all backslash-newline by newline.
            str.substitute(u"\\\n", u"\n");
            // Remove all newlines and compact spaces.
            size_t pos = 0;
            while ((pos = str.find('\n')) != NPOS) {
                // Locate the first space in the sequence.
                while (pos > 0 && IsSpace(str[pos - 1])) {
                    pos--;
                }
                // Replace the first space with a true space.
                str[pos] = ' ';
                // Remove all subsequent spaces.
                while (pos < str.length() - 1 && IsSpace(str[pos + 1])) {
                    str.erase(pos + 1, 1);
                }
            }
            return str;
        }
        case HELP_FULL: {
            // Default full complete help text.
            return u"\n" + _description + u"\n\nUsage: " + getHelpText(HELP_USAGE, line_width) + u"\n\n" + formatHelpOptions(line_width);
        }
        case HELP_OPTIONS: {
            // Options names, one by line.
            UString text;
            for (auto& it : _iopts) {
                const IOption& opt(it.second);
                const UString desc(opt.optionType());
                if (!text.empty()) {
                    text += LINE_FEED;
                }
                if (opt.short_name != CHAR_NULL) {
                    text += u'-';
                    text += opt.short_name;
                    text += desc;
                    text += LINE_FEED;
                }
                if (opt.name.empty()) {
                    text += u"@"; // meaning parameter
                }
                else {
                    text += u"--";
                    text += opt.name;
                }
                text += desc;
            }
            return text;
        }
        default: {
            return UString();
        }
    }
}


//----------------------------------------------------------------------------
// Process --help predefined option.
//----------------------------------------------------------------------------

void ts::Args::processHelp()
{
    // Build the help text. Use full text by default.
    const HelpFormat format = intValue(u"help", HELP_FULL);
    const UString text(getHelpText(format));

    // Create a pager process if we intend to exit immediately after a full help text.
    OutputPager pager;
    if (format == HELP_FULL && (_flags & NO_EXIT_ON_HELP) == 0 && pager.canPage() && pager.open(true, 0, *this)) {
        pager.write(text, *this);
        pager.write(u"\n", *this);
        pager.close(*this);
    }
    else if ((_flags & HELP_ON_THIS) != 0) {
        info(text);
    }
    else if (format == HELP_OPTIONS) {
        // --help=options is sent on stdout for automation.
        std::cout << text << std::endl;
    }
    else {
        std::cerr << text << std::endl;
    }

    // Exit application, unless specified otherwise.
    if ((_flags & NO_EXIT_ON_HELP) == 0) {
        std::exit(EXIT_SUCCESS);
    }
}


//----------------------------------------------------------------------------
// Process --version predefined option.
//----------------------------------------------------------------------------

void ts::Args::processVersion()
{
    // The meaning of the option value is managed inside GetVersion.
    info(VersionInfo::GetVersion(intValue(u"version", VersionInfo::Format::LONG), _app_name));

    // Exit application, unless specified otherwise.
    if ((_flags & NO_EXIT_ON_VERSION) == 0) {
        std::exit(EXIT_SUCCESS);
    }
}


//----------------------------------------------------------------------------
// Process argument redirection using @c '\@' on a vector of strings.
//----------------------------------------------------------------------------

bool ts::Args::processArgsRedirection(UStringVector& args)
{
    bool result = true;

    for (auto it = args.begin(); it != args.end(); ) {
        if (it->startWith(u"@@")) {
            // An initial double @ means a single literal @. Remove the first @.
            it->erase(0, 1);
            ++it;
        }
        else if (it->startWith(u"@")) {
            // Replace the line with the content of a file.

            // Get the file name.
            const UString fileName(it->substr(1));

            // Remove the line from the argument array.
            it = args.erase(it);

            // Load the text file.
            UStringVector lines;
            if (UString::Load(lines, fileName)) {
                // Insert the loaded lines. Then, make "it" point to the first inserted element.
                // This means that the loaded content will now be processed, allowing nested '@' directives.
                it = args.insert(it, lines.begin(), lines.end());
            }
            else {
                // Error loading file.
                result = false;
                error(u"error reading command line arguments from file \"%s\"", {fileName});
            }
        }
        else {
            // No leading '@', nothing to do
            ++it;
        }
    }

    return result;
}
