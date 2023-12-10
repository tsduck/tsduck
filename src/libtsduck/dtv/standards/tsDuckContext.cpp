//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDuckContext.h"
#include "tsDuckConfigFile.h"
#include "tsDVBCharTableSingleByte.h"
#include "tsDVBCharTableUTF8.h"
#include "tsARIBCharset.h"
#include "tsHFBand.h"
#include "tsTime.h"
#include "tsCerrReport.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructor and destructors.
//----------------------------------------------------------------------------

ts::DuckContext::DuckContext(Report* report, std::ostream* output) :
    _report(report != nullptr ? report : &CERR),
    _initial_out(output != nullptr ? output : &std::cout),
    _out(_initial_out),
    _charsetIn(&DVBCharTableSingleByte::DVB_ISO_6937),  // default DVB charset
    _charsetOut(&DVBCharTableSingleByte::DVB_ISO_6937),
    _timeRefConfig(DuckConfigFile::Instance().value(u"default.time")),
    _predefined_cas{{CASID_CONAX_MIN,      u"conax"},
                    {CASID_IRDETO_MIN,     u"irdeto"},
                    {CASID_MEDIAGUARD_MIN, u"mediaguard"},
                    {CASID_NAGRA_MIN,      u"nagravision"},
                    {CASID_NDS_MIN,        u"nds"},
                    {CASID_SAFEACCESS,     u"safeaccess"},
                    {CASID_VIACCESS_MIN,   u"viaccess"},
                    {CASID_WIDEVINE_MIN,   u"widevine"}}
{
    // Initialize time reference from configuration file. Ignore errors.
    if (!_timeRefConfig.empty() && !setTimeReference(_timeRefConfig)) {
        CERR.verbose(u"invalid default.time '%s' in %s", {_timeRefConfig, DuckConfigFile::Instance().fileName()});
    }

    // Get leap.seconds initial value from configuration file. Default value is true.
    const UString ls(DuckConfigFile::Instance().value(u"leap.seconds"));
    if (!ls.empty() && !ls.toBool(_useLeapSeconds)) {
        _useLeapSeconds = true;
        CERR.verbose(u"invalid leap.seconds '%s' in %s", {ls, DuckConfigFile::Instance().fileName()});
    }
}


//----------------------------------------------------------------------------
// Reset the TSDuck context to initial configuration.
//----------------------------------------------------------------------------

void ts::DuckContext::reset()
{
    if (_outFile.is_open()) {
        _outFile.close();
    }

    _out = _initial_out;
    _charsetIn = _charsetOut = &DVBCharTableSingleByte::DVB_ISO_6937;
    _casId = CASID_NULL;
    _defaultPDS = 0;
    _cmdStandards = _accStandards = Standards::NONE;
    _hfDefaultRegion.clear();
    _timeReference = 0;
}


//----------------------------------------------------------------------------
// Set a new report for log and error messages.
//----------------------------------------------------------------------------

void ts::DuckContext::setReport(Report* report)
{
    _report = report != nullptr ? report : &CERR;
}


//----------------------------------------------------------------------------
// Set the DVB character sets (default DVB character set if null).
//----------------------------------------------------------------------------

void ts::DuckContext::setDefaultCharsetIn(const Charset* charset)
{
    _charsetIn = charset != nullptr ? charset : &DVBCharTableSingleByte::DVB_ISO_6937;
}

void ts::DuckContext::setDefaultCharsetOut(const Charset* charset)
{
    _charsetOut = charset != nullptr ? charset : &DVBCharTableSingleByte::DVB_ISO_6937;
}


//----------------------------------------------------------------------------
// Update the list of standards which are present in the context.
//----------------------------------------------------------------------------

void ts::DuckContext::addStandards(Standards mask)
{
    if (_report->debug() && (_accStandards | mask) != _accStandards) {
        _report->debug(u"adding standards %s to %s", {StandardsNames(mask), StandardsNames(_accStandards)});
    }
    _accStandards |= mask;
}

void ts::DuckContext::resetStandards(Standards mask)
{
    _accStandards = _cmdStandards | mask;

    if (_report->debug()) {
        _report->debug(u"resetting standards to %s", {StandardsNames(_accStandards)});
    }
}


//----------------------------------------------------------------------------
// The actual CAS id to use.
//----------------------------------------------------------------------------

void ts::DuckContext::setDefaultCASId(uint16_t cas)
{
    _casId = cas;
}

uint16_t ts::DuckContext::casId(uint16_t cas) const
{
    return cas == CASID_NULL ? _casId : cas;
}


//----------------------------------------------------------------------------
// The actual private data specifier to use.
//----------------------------------------------------------------------------

void ts::DuckContext::setDefaultPDS(PDS pds)
{
    _defaultPDS = pds;
}

ts::PDS ts::DuckContext::actualPDS(PDS pds) const
{
    if (pds != 0) {
        // Explicit PDS already defined.
        return pds;
    }
    else if (_defaultPDS != 0) {
        // A default PDS was specified.
        return _defaultPDS;
    }
    else if (bool(_accStandards & Standards::ATSC)) {
        // We have previously found ATSC signalization, use the fake PDS for ATSC.
        // This allows interpretation of ATSC descriptors in MPEG-defined tables (eg. PMT).
        return PDS_ATSC;
    }
    else if (bool(_accStandards & Standards::ISDB)) {
        // Same principle for ISDB.
        return PDS_ISDB;
    }
    else {
        // Really no PDS to use.
        return 0;
    }
}


//----------------------------------------------------------------------------
// Registration ids (from MPEG-defined registration_descriptor).
//----------------------------------------------------------------------------

void ts::DuckContext::addRegistrationId(uint32_t regid)
{
    if (regid != REGID_NULL) {
        _lastRegistrationId = regid;
        _registrationIds.insert(regid);
    }
}

void ts::DuckContext::resetRegistrationIds()
{
    _lastRegistrationId = REGID_NULL;
    _registrationIds.clear();
}


//----------------------------------------------------------------------------
// Name of the default region for UVH and VHF band frequency layout.
//----------------------------------------------------------------------------

void ts::DuckContext::setDefaultHFRegion(const UString& region)
{
    _hfDefaultRegion = region;
}

ts::UString ts::DuckContext::defaultHFRegion() const
{
    // If the region is empty, get the one for the TSDuck configuration file.
    if (!_hfDefaultRegion.empty()) {
        return _hfDefaultRegion;
    }
    else {
        return DuckConfigFile::Instance().value(u"default.region", u"europe");
    }
}

const ts::HFBand* ts::DuckContext::hfBand(const UString& name, bool silent_band) const
{
    return HFBand::GetBand(defaultHFRegion(), name, *_report, silent_band);
}

const ts::HFBand* ts::DuckContext::vhfBand() const
{
    return HFBand::GetBand(defaultHFRegion(), u"VHF", *_report);
}

const ts::HFBand* ts::DuckContext::uhfBand() const
{
    return HFBand::GetBand(defaultHFRegion(), u"UHF", *_report);
}


//----------------------------------------------------------------------------
// Set a non-standard time reference offset using a name.
//----------------------------------------------------------------------------

bool ts::DuckContext::setTimeReference(const UString& name)
{
    // Convert to uppercase without space.
    UString str(name);
    str.convertToUpper();
    str.remove(SPACE);

    if (str == u"UTC") {
        _timeReference = 0;
        return true;
    }
    else if (str == u"JST") {
        _timeReference = Time::JSTOffset;
        return true;
    }

    size_t count = 0;
    size_t last = 0;
    UChar sign = CHAR_NULL;
    SubSecond hours = 0;
    SubSecond minutes = 0;

    str.scan(count, last, u"UTC%c%d:%d", {&sign, &hours, &minutes});
    if ((count == 2 || count == 3) &&
        last == str.size() &&
        (sign == u'+' || sign == u'-') &&
        hours >= 0 && hours <= 12 &&
        minutes >= 0 && minutes <= 59)
    {
        _timeReference = (sign == u'+' ? +1 : -1) * (hours * MilliSecPerHour + minutes * MilliSecPerMin);
        return true;
    }
    else {
        // Incorrect name.
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the non-standard time reference offset as a string.
//----------------------------------------------------------------------------

ts::UString ts::DuckContext::timeReferenceName() const
{
    if (_timeReference == 0) {
        return u"UTC";  // no offset
    }
    else if (_timeReference == Time::JSTOffset) {
        return u"JST";
    }
    else {
        const UChar sign = _timeReference < 0 ? u'-' : u'+';
        const SubSecond hours = std::abs(_timeReference) / MilliSecPerHour;
        const SubSecond minutes = (std::abs(_timeReference) / MilliSecPerMin) % 60;
        if (minutes == 0) {
            return UString::Format(u"UTC%c%d", {sign, hours});
        }
        else {
            return UString::Format(u"UTC%c%d:%02d", {sign, hours, minutes});
        }
    }
}


//----------------------------------------------------------------------------
// Flush the text output.
//----------------------------------------------------------------------------

void ts::DuckContext::flush()
{
    // Flush the output.
    _out->flush();

    // On Unix, we must force the lower-level standard output.
#if defined(TS_UNIX)
    if (_out == &std::cout) {
        ::fflush(stdout);
        ::fsync(STDOUT_FILENO);
    }
    else if (_out == &std::cerr) {
        ::fflush(stderr);
        ::fsync(STDERR_FILENO);
    }
#endif
}


//----------------------------------------------------------------------------
// Redirect the output stream to a file.
//----------------------------------------------------------------------------

void ts::DuckContext::setOutput(std::ostream* stream, bool override)
{
    // Do not override output if not standard output (or explicit override).
    if (override || _out == &std::cout) {
        if (_out == &_outFile) {
            _outFile.close();
        }
        _out = stream == nullptr ? &std::cout : stream;
    }
}

bool ts::DuckContext::setOutput(const fs::path& fileName, bool override)
{
    // Do not override output if not standard output (or explicit override).
    if (override || _out == &std::cout) {
        // Close previous file, if any.
        if (_out == &_outFile) {
            _outFile.close();
            _out = &std::cout;
        }

        // Open new file if any.
        if (!fileName.empty() && fileName != u"-") {
            _report->verbose(u"creating %s", {fileName});
            _outFile.open(fileName, std::ios::out);
            if (!_outFile) {
                _report->error(u"cannot create %s", {fileName});
                return false;
            }
            _out = &_outFile;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Define several classes of command line options in an Args.
//----------------------------------------------------------------------------

void ts::DuckContext::defineOptions(Args& args, int cmdOptionsMask)
{
    // Remember defined command line options.
    _definedCmdOptions |= cmdOptionsMask;

    // Options relating to default PDS.
    if (cmdOptionsMask & CMD_PDS) {

        args.option(u"default-pds", 0, PrivateDataSpecifierEnum);
        args.help(u"default-pds",
                  u"Default private data specifier. This option is meaningful only when the "
                  u"signalization is incorrect, when private descriptors appear in tables "
                  u"without a preceding private_data_specifier_descriptor. The specified "
                  u"value is used as private data specifier to interpret private descriptors. "
                  u"The PDS value can be an integer or one of (not case-sensitive) names.");
    }

    // Options relating to default character sets.
    if (cmdOptionsMask & CMD_CHARSET) {

        args.option(u"default-charset", 0, Args::STRING);
        args.help(u"default-charset", u"name",
                  u"Default character set to use when interpreting strings from tables and descriptors. "
                  u"By default, DVB encoding using ISO-6937 as default table is used. "
                  u"The available table names are " +
                  UString::Join(DVBCharset::GetAllNames()) + u".");

        args.option(u"europe");
        args.help(u"europe",
                  u"A synonym for '--default-charset ISO-8859-15'. This is a handy shortcut "
                  u"for commonly incorrect signalization on some European satellites. In that "
                  u"signalization, the character encoding is ISO-8859-15, the most common "
                  u"encoding for Latin & Western Europe languages. However, this is not the "
                  u"default DVB character set and it should be properly specified in all "
                  u"strings, which is not the case with some operators. Using this option, "
                  u"all DVB strings without explicit table code are assumed to use ISO-8859-15 "
                  u"instead of the standard ISO-6937 encoding.");
    }

    // Options relating to default standards.
    if (cmdOptionsMask & CMD_STANDARDS) {

        args.option(u"abnt");
        args.help(u"abnt",
                  u"Assume that the transport stream is an ISDB one with ABNT-defined variants. "
                  u"ISDB streams are normally automatically detected from their signalization but "
                  u"there is no way to determine if this is an original ARIB-defined ISDB or "
                  u"an ABNT-defined variant.");

        args.option(u"atsc");
        args.help(u"atsc",
                  u"Assume that the transport stream is an ATSC one. ATSC streams are normally "
                  u"automatically detected from their signalization. This option is only "
                  u"useful when ATSC-related stuff are found in the TS before the first "
                  u"ATSC-specific table. For instance, when a PMT with ATSC-specific "
                  u"descriptors is found before the first ATSC MGT or VCT.");

        args.option(u"isdb");
        args.help(u"isdb",
                  u"Assume that the transport stream is an ISDB one. ISDB streams are normally "
                  u"automatically detected from their signalization. This option is only "
                  u"useful when ISDB-related stuff are found in the TS before the first "
                  u"ISDB-specific table.");

        args.option(u"ignore-leap-seconds");
        args.help(u"ignore-leap-seconds",
                  u"Do not include explicit leap seconds in some UTC computations. "
                  u"Currently, this applies to SCTE 35 splice_schedule() commands only.");
    }

    // Options relating to default UHF/VHF region.
    if (cmdOptionsMask & CMD_HF_REGION) {

        args.option(u"hf-band-region", 'r', Args::STRING);
        args.help(u"hf-band-region", u"name",
            u"Specify the region for UHF/VHF band frequency layout. "
            u"The available regions are " +
            UString::Join(HFBand::GetAllRegions(*_report)) + u".");
    }

    // Options relating to default CAS identification.
    if (cmdOptionsMask & CMD_CAS) {

        args.option(u"default-cas-id", 0, Args::UINT16);
        args.help(u"default-cas-id",
                  u"Interpret all EMM's and ECM's from unknown CAS as coming from "
                  u"the specified CA_System_Id. By default, EMM's and ECM's are "
                  u"interpreted according to the CA_descriptor which references their PID. "
                  u"This option is useful when analyzing partial transport streams without "
                  u"CAT or PMT to correctly identify the CA PID's.");

        // Predefined CAS options:
        for (const auto& cas : _predefined_cas) {
            args.option(cas.second);
            args.help(cas.second, UString::Format(u"Equivalent to --default-cas-id 0x%04X.", {cas.first}));
        }
    }

    // Options relating to non-standard time reference.
    if (cmdOptionsMask & CMD_TIMEREF) {

        args.option(u"time-reference", 0, Args::STRING);
        args.help(u"time-reference", u"name",
                  u"Use a non-standard (non-UTC) time reference in TDT/TOT. "
                  u"This is typically used in ARIB ISDB and ABNT ISDB-Tb standards. "
                  u"The specified name can be either 'UTC', 'JST' (Japan Standard Time) or 'UTC+|-hh[:mm]'. "
                  u"Examples: 'UTC+9' (same as 'JST' for ARIB ISDB), 'UTC-3' (for ABNT ISDB-Tb in Brazil).");
    }

    // Option --japan triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) {

        // Build help text for --japan option. It depends on which set of options is requested.
        // Use _definedCmdOptions instead of cmdOptionsMask to include previous options.
        UStringList options;
        if (_definedCmdOptions & CMD_STANDARDS) {
            options.push_back(u"--isdb");
        }
        if (_definedCmdOptions & CMD_CHARSET) {
            options.push_back(u"--default-charset ARIB-STD-B24");
        }
        if (_definedCmdOptions & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region japan");
        }
        if (_definedCmdOptions & CMD_TIMEREF) {
            options.push_back(u"--time-reference JST");
        }
        args.option(u"japan");
        args.help(u"japan",
                  u"A synonym for '" + UString::Join(options, u" ") + u"'. "
                  u"This is a handy shortcut when working on Japanese transport streams.");
    }

    // Option --philippines triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) {

        // Build help text. Same principle as --japan.
        UStringList options;
        if (_definedCmdOptions & CMD_STANDARDS) {
            options.push_back(u"--isdb");
            options.push_back(u"--abnt");
        }
        if (_definedCmdOptions & CMD_CHARSET) {
            options.push_back(u"--default-charset RAW-UTF-8");
        }
        if (_definedCmdOptions & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region philippines");
        }
        if (_definedCmdOptions & CMD_TIMEREF) {
            options.push_back(u"--time-reference UTC+8");
        }
        args.option(u"philippines");
        args.help(u"philippines",
                  u"A synonym for '" + UString::Join(options, u" ") + u"'. "
                  u"This is a handy shortcut when working on Philippines transport streams.");
    }

    // Option --brazil triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) {

        // Build help text. Same principle as --japan.
        UStringList options;
        if (_definedCmdOptions & CMD_STANDARDS) {
            options.push_back(u"--isdb");
            options.push_back(u"--abnt");
        }
        if (_definedCmdOptions & CMD_CHARSET) {
            options.push_back(u"--default-charset RAW-ISO-8859-15");
        }
        if (_definedCmdOptions & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region brazil");
        }
        if (_definedCmdOptions & CMD_TIMEREF) {
            options.push_back(u"--time-reference UTC-3");
        }
        args.option(u"brazil");
        args.help(u"brazil",
                  u"A synonym for '" + UString::Join(options, u" ") + u"'. "
                  u"This is a handy shortcut when working on South American ISDB-Tb transport streams.");
    }

    // Option --usa triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_STANDARDS | CMD_HF_REGION)) {

        // Build help text. Same principle as --japan.
        UStringList options;
        if (_definedCmdOptions & CMD_STANDARDS) {
            options.push_back(u"--atsc");
        }
        if (_definedCmdOptions & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region usa");
        }
        args.option(u"usa");
        args.help(u"usa",
                  u"A synonym for '" + UString::Join(options, u" ") + u"'. "
                  u"This is a handy shortcut when working on North American transport streams.");
    }
}


//----------------------------------------------------------------------------
// Load the values of all previously defined arguments from command line.
//----------------------------------------------------------------------------

bool ts::DuckContext::loadArgs(Args& args)
{
    // List of forced standards from the command line.
    _cmdStandards = Standards::NONE;

    // Options relating to default PDS.
    if (_definedCmdOptions & CMD_PDS) {
        // Keep previous value unchanged if unspecified.
        args.getIntValue(_defaultPDS, u"default-pds", _defaultPDS);
    }

    // Options relating to default DVB character sets.
    if (_definedCmdOptions & CMD_CHARSET) {
        const UString name(args.value(u"default-charset"));
        if (!name.empty()) {
            const Charset* cset = DVBCharTable::GetCharset(name);
            if (cset == nullptr) {
                args.error(u"invalid character set name '%s'", {name});
            }
            else {
                _charsetIn = _charsetOut = cset;
            }
        }
        else if (args.present(u"europe")) {
            _charsetIn = _charsetOut = &DVBCharTableSingleByte::DVB_ISO_8859_15;
        }
        else if (args.present(u"brazil")) {
            _charsetIn = _charsetOut = &DVBCharTableSingleByte::RAW_ISO_8859_15;
        }
        else if (args.present(u"philippines")) {
            _charsetIn = _charsetOut = &DVBCharTableUTF8::RAW_UTF_8;
        }
        else if (args.present(u"japan")) {
            _charsetIn = _charsetOut = &ARIBCharset::B24;
        }
    }

    // Options relating to default UHF/VHF region.
    if (_definedCmdOptions & CMD_HF_REGION) {
        if (args.present(u"hf-band-region")) {
            args.getValue(_hfDefaultRegion, u"hf-band-region", _hfDefaultRegion.c_str());
        }
        else if (args.present(u"japan")) {
            _hfDefaultRegion = u"japan";
        }
        else if (args.present(u"brazil")) {
            _hfDefaultRegion = u"brazil";
        }
        else if (args.present(u"philippines")) {
            _hfDefaultRegion = u"philippines";
        }
    }

    // Options relating to default standards.
    if (_definedCmdOptions & CMD_STANDARDS) {
        if (args.present(u"atsc")) {
            _cmdStandards |= Standards::ATSC;
        }
        if (args.present(u"isdb") || args.present(u"japan")) {
            _cmdStandards |= Standards::ISDB;
        }
        if (args.present(u"abnt") || args.present(u"brazil") || args.present(u"philippines")) {
            _cmdStandards |= Standards::ISDB | Standards::ABNT;
        }
        _useLeapSeconds = !args.present(u"ignore-leap-seconds");
    }
    if ((_definedCmdOptions & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) && args.present(u"japan")) {
        _cmdStandards |= Standards::JAPAN;
    }

    // Options relating to default CAS.
    if (_definedCmdOptions & CMD_CAS) {
        int count = 0;
        if (args.present(u"default-cas-id")) {
            _casId = args.intValue<uint16_t>(u"default-cas-id");
            count++;
        }
        // Predefined CAS options:
        for (const auto& cas : _predefined_cas) {
            if (args.present(cas.second)) {
                _casId = cas.first;
                count++;
            }
        }
        if (count > 1) {
            args.error(u"more than one default CAS defined");
        }
    }

    // Options relating to non-standard time reference.
    if (_definedCmdOptions & CMD_TIMEREF) {
        if (args.present(u"time-reference")) {
            const UString name(args.value(u"time-reference"));
            if (!setTimeReference(name)) {
                args.error(u"invalid time reference '%s'", {name});
            }
        }
        else if (args.present(u"japan")) {
            _timeReference = Time::JSTOffset;
        }
        else if (args.present(u"brazil")) {
            _timeReference = -3 * MilliSecPerHour; // UTC-3
        }
        else if (args.present(u"philippines")) {
            _timeReference = 8 * MilliSecPerHour; // UTC+8
        }
    }

    // Preset forced standards from the command line.
    _accStandards |= _cmdStandards;

    return args.valid();
}


//----------------------------------------------------------------------------
// An opaque class to save all command line options, as loaded by loadArgs().
//----------------------------------------------------------------------------

ts::DuckContext::SavedArgs::SavedArgs() :
    _definedCmdOptions(0),
    _cmdStandards(Standards::NONE),
    _charsetInName(),
    _charsetOutName(),
    _casId(CASID_NULL),
    _defaultPDS(0),
    _hfDefaultRegion(),
    _timeReference(0)
{
}

void ts::DuckContext::saveArgs(SavedArgs& args) const
{
    args._definedCmdOptions = _definedCmdOptions;
    args._cmdStandards = _cmdStandards;
    args._charsetInName = _charsetIn->name();
    args._charsetOutName = _charsetOut->name();
    args._casId = _casId;
    args._defaultPDS = _defaultPDS;
    args._hfDefaultRegion = _hfDefaultRegion;
    args._timeReference = _timeReference;
}

void ts::DuckContext::restoreArgs(const SavedArgs& args)
{
    if (args._definedCmdOptions & CMD_STANDARDS) {
        // Reset accumulated standards if a list of standards was saved.
        _accStandards = _cmdStandards = args._cmdStandards;
    }
    if (args._definedCmdOptions & CMD_CHARSET) {
        const Charset* in = DVBCharTable::GetCharset(args._charsetInName);
        const Charset* out = DVBCharTable::GetCharset(args._charsetOutName);
        if (in != nullptr) {
            _charsetIn = in;
        }
        if (out != nullptr) {
            _charsetOut = out;
        }
    }
    if (_definedCmdOptions & CMD_CAS) {
        _casId = args._casId;
    }
    if (_definedCmdOptions & CMD_PDS) {
        _defaultPDS = args._defaultPDS;
    }
    if (_definedCmdOptions & CMD_HF_REGION) {
        _hfDefaultRegion = args._hfDefaultRegion;
    }
    if (_definedCmdOptions & CMD_TIMEREF) {
        _timeReference = args._timeReference;
    }
}
