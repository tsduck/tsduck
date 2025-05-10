//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDuckContext.h"
#include "tsDuckConfigFile.h"
#include "tsDVBCharTableSingleByte.h"
#include "tsDVBCharTableUTF8.h"
#include "tsARIBCharset.h"
#include "tsHFBand.h"
#include "tsCerrReport.h"
#include "tsTime.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructor and destructors.
//----------------------------------------------------------------------------

ts::DuckContext::DuckContext(Report* report, std::ostream* output) :
    _report(report != nullptr ? report : &CERR),
    _initial_out(output != nullptr ? output : &std::cout),
    _out(_initial_out),
    _charset_in(&DVBCharset::DVB),  // default DVB charset
    _charset_out(&DVBCharset::DVB),
    _time_ref_config(DuckConfigFile::Instance().value(u"default.time")),
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
    if (!_time_ref_config.empty() && !setTimeReference(_time_ref_config)) {
        CERR.verbose(u"invalid default.time '%s' in %s", _time_ref_config, DuckConfigFile::Instance().fileName());
    }

    // Get leap.seconds initial value from configuration file. Default value is true.
    const UString ls(DuckConfigFile::Instance().value(u"leap.seconds"));
    if (!ls.empty() && !ls.toBool(_use_leap_seconds)) {
        _use_leap_seconds = true;
        CERR.verbose(u"invalid leap.seconds '%s' in %s", ls, DuckConfigFile::Instance().fileName());
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
    _charset_in = _charset_out = &DVBCharset::DVB;
    _casId = CASID_NULL;
    _fix_pds = false;
    _default_pds = 0;
    _default_regids.clear();
    _cmd_standards = _acc_standards = Standards::NONE;
    _hf_default_region.clear();
    _time_reference = cn::milliseconds::zero();
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
    _charset_in = charset != nullptr ? charset : &DVBCharset::DVB;
}

void ts::DuckContext::setDefaultCharsetOut(const Charset* charset)
{
    _charset_out = charset != nullptr ? charset : &DVBCharset::DVB;
}


//----------------------------------------------------------------------------
// Update the list of standards which are present in the context.
//----------------------------------------------------------------------------

void ts::DuckContext::addStandards(Standards mask)
{
    if (_report->debug() && (_acc_standards | mask) != _acc_standards) {
        _report->debug(u"adding standards %s to %s", StandardsNames(mask), StandardsNames(_acc_standards));
    }
    _acc_standards |= mask;
}

void ts::DuckContext::resetStandards(Standards mask)
{
    _acc_standards = _cmd_standards | mask;

    if (_report->debug()) {
        _report->debug(u"resetting standards to %s", StandardsNames(_acc_standards));
    }
}


//----------------------------------------------------------------------------
// The actual private data specifier to use.
//----------------------------------------------------------------------------

ts::PDS ts::DuckContext::actualPDS(PDS pds) const
{
    if (pds != 0 && pds != PDS_NULL) {
        // Explicit PDS already defined.
        return pds;
    }
    else if (_default_pds != 0 && _default_pds != PDS_NULL) {
        // A default PDS was specified.
        return _default_pds;
    }
    else {
        // Really no PDS to use.
        return 0;
    }
}


//----------------------------------------------------------------------------
// Name of the default region for UVH and VHF band frequency layout.
//----------------------------------------------------------------------------

ts::UString ts::DuckContext::defaultHFRegion() const
{
    // If the region is empty, get the one for the TSDuck configuration file.
    if (!_hf_default_region.empty()) {
        return _hf_default_region;
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

    if (str.similar(u"UTC")) {
        _time_reference = cn::milliseconds::zero();
        return true;
    }
    else if (str.similar(u"JST")) {
        _time_reference = Time::JSTOffset;
        return true;
    }

    size_t count = 0;
    size_t last = 0;
    UChar sign = CHAR_NULL;
    cn::hours::rep hours = 0;
    cn::minutes::rep minutes = 0;

    str.scan(count, last, u"UTC%c%d:%d", &sign, &hours, &minutes);
    if ((count == 2 || count == 3) &&
        last == str.size() &&
        (sign == u'+' || sign == u'-') &&
        hours >= 0 && hours <= 12 &&
        minutes >= 0 && minutes <= 59)
    {
        _time_reference = (sign == u'+' ? +1 : -1) * (cn::hours(hours) + cn::minutes(minutes));
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
    if (_time_reference == cn::minutes::zero()) {
        return u"UTC";  // no offset
    }
    else if (_time_reference == Time::JSTOffset) {
        return u"JST";
    }
    else {
        const UChar sign = _time_reference < cn::minutes::zero() ? u'-' : u'+';
        const cn::minutes::rep minutes = std::abs(cn::duration_cast<cn::minutes>(_time_reference).count());
        if (minutes % 60 == 0) {
            return UString::Format(u"UTC%c%d", sign, minutes / 60);
        }
        else {
            return UString::Format(u"UTC%c%d:%02d", sign, minutes / 60, minutes % 60);
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
            _report->verbose(u"creating %s", fileName);
            _outFile.open(fileName, std::ios::out);
            if (!_outFile) {
                _report->error(u"cannot create %s", fileName);
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
    _defined_cmd_options |= cmdOptionsMask;

    // Options relating to default PDS.
    if (cmdOptionsMask & CMD_PDS) {

        args.option(u"default-pds", 0, PrivateDataSpecifierEnum());
        args.help(u"default-pds",
                  u"Default private data specifier. "
                  u"This option is meaningful only when the signalization is incorrect, "
                  u"when DVB private descriptors appear in tables without a preceding private_data_specifier_descriptor. "
                  u"The specified value is used as private data specifier to interpret DVB private descriptors. "
                  u"The PDS value can be an integer or one of (not case-sensitive) names.");

        args.option(u"default-registration", 0, Args::UINT32, 0, Args::UNLIMITED_COUNT);
        args.help(u"default-registration",
                  u"Default registration id. "
                  u"This option is meaningful only when the signalization is incorrect, "
                  u"when MPEG private descriptors appear in tables without a preceding registration_descriptor. "
                  u"The specified value is used as registration id (also know as format identifier) "
                  u"to interpret MPEG private descriptors or stream types.\n"
                  u"Several options --default-registration can be specified. "
                  u"Unlike DVB private data specifiers, several MPEG registration ids can be simultaneously defined.");
    }
    if (cmdOptionsMask & CMD_FIX_PDS) {

        args.option(u"fix-missing-pds");
        args.help(u"fix-missing-pds",
                  u"When serializing XML MPEG or DVB private descriptors, automatically add missing "
                  u"registration descriptors and private data specifier descriptors.");
    }

    // Options relating to default character sets.
    if (cmdOptionsMask & CMD_CHARSET) {

        args.option(u"default-charset", 0, Args::STRING);
        args.help(u"default-charset", u"name",
                  u"Default character set to use when interpreting strings from tables and descriptors. "
                  u"By default, DVB encoding using ISO-6937 as default table is used. "
                  u"The available table names are " +
                  UString::Join(DVBCharset::GetAllNames()) + u".");
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

        args.option(u"dvb");
        args.help(u"dvb",
                  u"Assume that the transport stream is a DVB one. "
                  u"DVB streams are normally automatically detected from their signalization. "
                  u"This option is only useful when possibly incorrect non-DVB stuff are found "
                  u"in the TS before the first DVB-specific table.");

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
            args.help(cas.second, UString::Format(u"Equivalent to --default-cas-id 0x%04X.", cas.first));
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

    // Option --europe triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS)) {

        // Build help text for --europe option. It depends on which set of options is requested.
        // Use _definedCmdOptions instead of cmdOptionsMask to include previous options.
        UStringList options;
        UString other;
        if (_defined_cmd_options & CMD_STANDARDS) {
            options.push_back(u"--dvb");
        }
        if (_defined_cmd_options & CMD_CHARSET) {
            options.push_back(u"--default-charset ISO-8859-15");
            other = u" This is a handy shortcut for commonly incorrect signalization on some European satellites. "
                    u"In that signalization, the character encoding is ISO-8859-15, "
                    u"the most common encoding for Latin & Western Europe languages. "
                    u"However, this is not the default DVB character set and it should be properly specified in all strings, "
                    u"which is not the case with some operators. "
                    u"Using this option, all DVB strings without explicit table code are assumed to use ISO-8859-15 "
                    u"instead of the standard ISO-6937 encoding.";
        }
        args.option(u"europe");
        args.help(u"europe",
                  u"A synonym for '" + UString::Join(options, u" ") + u"'." + other);
    }

    // Option --japan triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) {

        // Build help text for --japan option. It depends on which set of options is requested.
        // Use _definedCmdOptions instead of cmdOptionsMask to include previous options.
        UStringList options;
        if (_defined_cmd_options & CMD_STANDARDS) {
            options.push_back(u"--isdb");
        }
        if (_defined_cmd_options & CMD_CHARSET) {
            options.push_back(u"--default-charset ARIB-STD-B24");
        }
        if (_defined_cmd_options & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region japan");
        }
        if (_defined_cmd_options & CMD_TIMEREF) {
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
        if (_defined_cmd_options & CMD_STANDARDS) {
            options.push_back(u"--isdb");
            options.push_back(u"--abnt");
        }
        if (_defined_cmd_options & CMD_CHARSET) {
            options.push_back(u"--default-charset RAW-UTF-8");
        }
        if (_defined_cmd_options & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region philippines");
        }
        if (_defined_cmd_options & CMD_TIMEREF) {
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
        if (_defined_cmd_options & CMD_STANDARDS) {
            options.push_back(u"--isdb");
            options.push_back(u"--abnt");
        }
        if (_defined_cmd_options & CMD_CHARSET) {
            options.push_back(u"--default-charset RAW-ISO-8859-15");
        }
        if (_defined_cmd_options & CMD_HF_REGION) {
            options.push_back(u"--hf-band-region brazil");
        }
        if (_defined_cmd_options & CMD_TIMEREF) {
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
        if (_defined_cmd_options & CMD_STANDARDS) {
            options.push_back(u"--atsc");
        }
        if (_defined_cmd_options & CMD_HF_REGION) {
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
    _cmd_standards = Standards::NONE;

    // Options relating to default PDS.
    if (_defined_cmd_options & CMD_PDS) {
        // Keep previous value unchanged if unspecified.
        args.getIntValue(_default_pds, u"default-pds", _default_pds);
        if (args.present(u"default-registration")) {
            args.getIntValues(_default_regids, u"default-registration");
        }
    }
    if (_defined_cmd_options & CMD_FIX_PDS) {
        // Keep previous value unchanged if unspecified.
        if (args.present(u"fix-missing-pds")) {
            _fix_pds = true;
        }
    }

    // Options relating to default DVB character sets.
    if (_defined_cmd_options & CMD_CHARSET) {
        const UString name(args.value(u"default-charset"));
        if (!name.empty()) {
            const Charset* cset = DVBCharTable::GetCharset(name);
            if (cset == nullptr) {
                args.error(u"invalid character set name '%s'", name);
            }
            else {
                _charset_in = _charset_out = cset;
            }
        }
        else if (args.present(u"europe")) {
            _charset_in = _charset_out = &DVBCharTableSingleByte::DVB_ISO_8859_15;
        }
        else if (args.present(u"brazil")) {
            _charset_in = _charset_out = &DVBCharTableSingleByte::RAW_ISO_8859_15;
        }
        else if (args.present(u"philippines")) {
            _charset_in = _charset_out = &DVBCharTableUTF8::RAW_UTF_8;
        }
        else if (args.present(u"japan")) {
            _charset_in = _charset_out = &ARIBCharset::B24;
        }
    }

    // Options relating to default UHF/VHF region.
    if (_defined_cmd_options & CMD_HF_REGION) {
        if (args.present(u"hf-band-region")) {
            args.getValue(_hf_default_region, u"hf-band-region", _hf_default_region.c_str());
        }
        else if (args.present(u"japan")) {
            _hf_default_region = u"japan";
        }
        else if (args.present(u"brazil")) {
            _hf_default_region = u"brazil";
        }
        else if (args.present(u"philippines")) {
            _hf_default_region = u"philippines";
        }
        else if (args.present(u"usa")) {
            _hf_default_region = u"usa";
        }
    }

    // Options relating to default standards.
    if (_defined_cmd_options & CMD_STANDARDS) {
        if (args.present(u"dvb") || args.present(u"europe")) {
            // The additional flags DVBONLY means pure DVB, not compatible with ISDB.
            _cmd_standards |= Standards::DVB | Standards::DVBONLY;
        }
        if (args.present(u"atsc") || args.present(u"usa")) {
            _cmd_standards |= Standards::ATSC;
        }
        if (args.present(u"isdb") || args.present(u"japan")) {
            _cmd_standards |= Standards::ISDB;
        }
        if (args.present(u"abnt") || args.present(u"brazil") || args.present(u"philippines")) {
            _cmd_standards |= Standards::ISDB | Standards::ABNT;
        }
        _use_leap_seconds = !args.present(u"ignore-leap-seconds");
    }
    if ((_defined_cmd_options & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION | CMD_TIMEREF)) && args.present(u"japan")) {
        _cmd_standards |= Standards::JAPAN;
    }

    // Options relating to default CAS.
    if (_defined_cmd_options & CMD_CAS) {
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
    if (_defined_cmd_options & CMD_TIMEREF) {
        if (args.present(u"time-reference")) {
            const UString name(args.value(u"time-reference"));
            if (!setTimeReference(name)) {
                args.error(u"invalid time reference '%s'", name);
            }
        }
        else if (args.present(u"japan")) {
            _time_reference = Time::JSTOffset;
        }
        else if (args.present(u"brazil")) {
            _time_reference = cn::hours(-3); // UTC-3
        }
        else if (args.present(u"philippines")) {
            _time_reference = cn::hours(+8); // UTC+8
        }
    }

    // Preset forced standards from the command line.
    _acc_standards |= _cmd_standards;

    return args.valid();
}


//----------------------------------------------------------------------------
// An opaque class to save all command line options, as loaded by loadArgs().
//----------------------------------------------------------------------------

void ts::DuckContext::saveArgs(SavedArgs& args) const
{
    args._defined_cmd_options = _defined_cmd_options;
    args._cmd_standards = _cmd_standards;
    args._charset_in_name = _charset_in->name();
    args._charset_out_name = _charset_out->name();
    args._cas_id = _casId;
    args._fix_pds = _fix_pds;
    args._default_pds = _default_pds;
    args._default_regids = _default_regids;
    args._hf_default_region = _hf_default_region;
    args._time_reference = _time_reference;
}

void ts::DuckContext::restoreArgs(const SavedArgs& args)
{
    if (args._defined_cmd_options & CMD_STANDARDS) {
        // Reset accumulated standards if a list of standards was saved.
        _acc_standards = _cmd_standards = args._cmd_standards;
    }
    if (args._defined_cmd_options & CMD_CHARSET) {
        const Charset* in = DVBCharTable::GetCharset(args._charset_in_name);
        const Charset* out = DVBCharTable::GetCharset(args._charset_out_name);
        if (in != nullptr) {
            _charset_in = in;
        }
        if (out != nullptr) {
            _charset_out = out;
        }
    }
    if (_defined_cmd_options & CMD_CAS) {
        _casId = args._cas_id;
    }
    if (_defined_cmd_options & CMD_FIX_PDS) {
        _fix_pds = args._fix_pds;
    }
    if (_defined_cmd_options & CMD_PDS) {
        _default_pds = args._default_pds;
        _default_regids = args._default_regids;
    }
    if (_defined_cmd_options & CMD_HF_REGION) {
        _hf_default_region = args._hf_default_region;
    }
    if (_defined_cmd_options & CMD_TIMEREF) {
        _time_reference = args._time_reference;
    }
}
