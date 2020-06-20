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

#include "tsDuckContext.h"
#include "tsDuckConfigFile.h"
#include "tsDVBCharTableSingleByte.h"
#include "tsARIBCharset.h"
#include "tsHFBand.h"
#include "tsCerrReport.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructors.
//----------------------------------------------------------------------------

ts::DuckContext::DuckContext(Report* report, std::ostream* output) :
    _report(report != nullptr ? report : CerrReport::Instance()),
    _initial_out(output != nullptr ? output : &std::cout),
    _out(_initial_out),
    _outFile(),
    _charsetIn(&DVBCharTableSingleByte::DVB_ISO_6937),  // default DVB charset
    _charsetOut(&DVBCharTableSingleByte::DVB_ISO_6937),
    _casId(CASID_NULL),
    _defaultPDS(0),
    _cmdStandards(Standards::NONE),
    _accStandards(Standards::NONE),
    _hfDefaultRegion(),
    _definedCmdOptions(0),
    _predefined_cas{{CASID_CONAX_MIN,      u"conax"},
                    {CASID_IRDETO_MIN,     u"irdeto"},
                    {CASID_MEDIAGUARD_MIN, u"mediaguard"},
                    {CASID_NAGRA_MIN,      u"nagravision"},
                    {CASID_NDS_MIN,        u"nds"},
                    {CASID_SAFEACCESS,     u"safeaccess"},
                    {CASID_VIACCESS_MIN,   u"viaccess"},
                    {CASID_WIDEVINE_MIN,   u"widevine"}}
{
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
}


//----------------------------------------------------------------------------
// Set a new report for log and error messages.
//----------------------------------------------------------------------------

void ts::DuckContext::setReport(Report* report)
{
    _report = report != nullptr ? report : CerrReport::Instance();
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
    else if ((_accStandards & Standards::ATSC) == Standards::ATSC) {
        // We have previously found ATSC signalization, use the fake PDS for ATSC.
        // This allows interpretation of ATSC descriptors in MPEG-defined tables (eg. PMT).
        return PDS_ATSC;
    }
    else if ((_accStandards & Standards::ISDB) == Standards::ISDB) {
        // Same principle for ISDB.
        return PDS_ISDB;
    }
    else {
        // Really no PDS to use.
        return 0;
    }
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
        return DuckConfigFile::Instance()->value(u"default.region", u"europe");
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
    // Do not override output is not standard output (or explicit override).
    if (override || _out == &std::cout) {
        if (_out == &_outFile) {
            _outFile.close();
        }
        _out = stream == nullptr ? &std::cout : stream;
    }
}

bool ts::DuckContext::setOutput(const UString& fileName, bool override)
{
    // Do not override output is not standard output (or explicit override).
    if (override || _out == &std::cout) {
        // Close previous file, if any.
        if (_out == &_outFile) {
            _outFile.close();
            _out = &std::cout;
        }

        // Open new file if any.
        if (!fileName.empty()) {
            _report->verbose(u"creating %s", {fileName});
            const std::string nameUTF8(fileName.toUTF8());
            _outFile.open(nameUTF8.c_str(), std::ios::out);
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
// A method to display data if it can be interpreted as an ASCII string.
//----------------------------------------------------------------------------

std::ostream& ts::DuckContext::displayIfASCII(const void *data, size_t size, const UString& prefix, const UString& suffix)
{
    const std::string ascii(toASCII(data, size));
    if (!ascii.empty()) {
        (*_out) << prefix << ascii << suffix;
    }
    return *_out;
}


//----------------------------------------------------------------------------
// A utility method to interpret data as an ASCII string.
//----------------------------------------------------------------------------

std::string ts::DuckContext::toASCII(const void *data, size_t size) const
{
    const char* str = reinterpret_cast<const char*>(data);
    size_t strSize = 0;

    for (size_t i = 0; i < size; ++i) {
        if (str[i] >= 0x20 && str[i] <= 0x7E) {
            // This is an ASCII character.
            if (i == strSize) {
                strSize++;
            }
            else {
                // But come after trailing zero.
                return std::string();
            }
        }
        else if (str[i] != 0) {
            // Not ASCII, not trailing zero, unusable string.
            return std::string();
        }
    }

    // Found an ASCII string.
    return std::string(str, strSize);
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

        args.option(u"europe", 0);
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

        args.option(u"atsc", 0);
        args.help(u"atsc",
                  u"Assume that the transport stream is an ATSC one. ATSC streams are normally "
                  u"automatically detected from their signalization. This option is only "
                  u"useful when ATSC-related stuff are found in the TS before the first "
                  u"ATSC-specific table. For instance, when a PMT with ATSC-specific "
                  u"descriptors is found before the first ATSC MGT or VCT.");

        args.option(u"isdb", 0);
        args.help(u"isdb",
                  u"Assume that the transport stream is an ISDB one. ISDB streams are normally "
                  u"automatically detected from their signalization. This option is only "
                  u"useful when ISDB-related stuff are found in the TS before the first "
                  u"ISDB-specific table.");
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
        for (auto cas = _predefined_cas.begin(); cas != _predefined_cas.end(); ++cas) {
            args.option(cas->second);
            args.help(cas->second, UString::Format(u"Equivalent to --default-cas-id 0x%04X.", {cas->first}));
        }
    }

    // Option --japan triggers different options in different sets of options.
    if (cmdOptionsMask & (CMD_CHARSET | CMD_STANDARDS | CMD_HF_REGION)) {

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
        UString japan(u"A synonym for '" + UString::Join(options, u" ") + u"'. ");
        if (_definedCmdOptions & CMD_STANDARDS) {
            japan.append(u"This option also activates some specificities for Japan such as the use of JST time instead of UTC. ");
        }
        japan.append(u"This is a handy shortcut when working on Japanese transport streams.");

        args.option(u"japan", 0);
        args.help(u"japan", japan);
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
        if (args.present(u"europe")) {
            _charsetIn = _charsetOut = &DVBCharTableSingleByte::DVB_ISO_8859_15;
        }
        else if (args.present(u"japan")) {
            _charsetIn = _charsetOut = &ARIBCharset::B24;
        }
        else {
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
        }
    }

    // Options relating to default UHF/VHF region.
    if (_definedCmdOptions & CMD_HF_REGION) {
        if (args.present(u"japan")) {
            _hfDefaultRegion = u"japan";
        }
        else if (args.present(u"hf-band-region")) {
            args.getValue(_hfDefaultRegion, u"hf-band-region");
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
    }
    if ((_definedCmdOptions & (CMD_STANDARDS | CMD_CHARSET)) && args.present(u"japan")) {
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
        for (auto cas = _predefined_cas.begin(); cas != _predefined_cas.end(); ++cas) {
            if (args.present(cas->second)) {
                _casId = cas->first;
                count++;
            }
        }
        if (count > 1) {
            args.error(u"more than one default CAS defined");
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
    _hfDefaultRegion()
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
    if (_definedCmdOptions & CMD_HF_REGION) {
        _hfDefaultRegion = args._hfDefaultRegion;
    }
}
