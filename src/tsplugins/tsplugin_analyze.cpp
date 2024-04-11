//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Transport stream analyzer.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSAnalyzerReport.h"
#include "tsTSSpeedMetrics.h"
#include "tsFileNameGenerator.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class AnalyzePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(AnalyzePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        fs::path          _output_name {};
        cn::nanoseconds   _output_interval {};
        bool              _multiple_output = false;
        bool              _cumulative = false;
        TSAnalyzerOptions _analyzer_options {};

        // Working data:
        std::ofstream     _output_stream {};
        std::ostream*     _output = nullptr;
        TSSpeedMetrics    _metrics {};
        cn::nanoseconds   _next_report {};
        TSAnalyzerReport  _analyzer {duck};
        FileNameGenerator _name_gen {};

        bool openOutput();
        void closeOutput();
        bool produceReport();
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"analyze", ts::AnalyzePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AnalyzePlugin::AnalyzePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze the structure of a transport stream", u"[options]")
{
    // Define all standard analysis options.
    duck.defineArgsForStandards(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForPDS(*this);
    _analyzer_options.defineArgs(*this);

    option(u"cumulative", 'c');
    help(u"cumulative",
         u"With --interval, accumulate analysis data of all intervals. "
         u"With this option, each new report is an analysis from the beginning of the stream. "
         u"By default, the analyzed data are reset after each report.");

    option<cn::seconds>(u"interval", 'i');
    help(u"interval",
         u"Produce a new output file at regular intervals. "
         u"The interval value is in seconds. "
         u"After outputting a file, the analysis context is reset, "
         u"ie. each output file contains a fully independent analysis.");

    option(u"multiple-files", 'm');
    help(u"multiple-files",
         u"When used with --interval and --output-file, create a new file for each "
         u"analysis instead of rewriting the previous file. Assuming that the "
         u"specified output file name has the form 'base.ext', each file is created "
         u"with a time stamp in its name as 'base-YYYYMMDD-hhmmss.ext'.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file",
         u"Specify the output text file for the analysis result. "
         u"By default, use the standard output.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::getOptions()
{
    duck.loadArgs(*this);
    _analyzer_options.loadArgs(duck, *this);
    getPathValue(_output_name, u"output-file");
    getChronoValue(_output_interval, u"interval");
    _multiple_output = present(u"multiple-files");
    _cumulative = present(u"cumulative");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::start()
{
    _output = _output_name.empty() ? &std::cout : &_output_stream;
    _analyzer.reset();
    _analyzer.setAnalysisOptions(_analyzer_options);
    _name_gen.initDateTime(_output_name);

    // For production of multiple reports at regular intervals.
    _metrics.start();
    _next_report = _output_interval;

    // Create the output file. Note that this file is used only in the stop
    // method and could be created there. However, if the file cannot be
    // created, we do not want to wait all along the analysis and finally fail.
    if (_output_interval <= cn::nanoseconds::zero() && !openOutput()) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Create an output file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::openOutput()
{
    // Standard output is always open. Also do not reopen an open file.
    if (_output_name.empty() || _output_stream.is_open()) {
        return true;
    }

    // Build file name in case of --multiple-files
    const fs::path name(_multiple_output ? _name_gen.newFileName() : _output_name);

    // Create the file
    _output_stream.open(name);
    if (_output_stream) {
        return true;
    }
    else {
        tsp->error(u"cannot create file %s", name);
        return false;
    }
}


//----------------------------------------------------------------------------
// Close current output file.
//----------------------------------------------------------------------------

void ts::AnalyzePlugin::closeOutput()
{
    if (!_output_name.empty() && _output_stream.is_open()) {
        _output_stream.close();
    }
}


//----------------------------------------------------------------------------
// Produce a report. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::produceReport()
{
    if (!openOutput()) {
        return false;
    }
    else {
        // Set last known input bitrate as hint
        _analyzer.setBitrateHint(tsp->bitrate(), tsp->bitrateConfidence());

        // Produce the report
        _analyzer.report(*_output, _analyzer_options, *tsp);
        closeOutput();
        return true;
    }
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::stop()
{
    produceReport();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AnalyzePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the analyzer with one packet
    _analyzer.feedPacket(pkt);

    // With --interval, check if it is time to produce a report
    if (_output_interval > cn::nanoseconds::zero() && _metrics.processedPacket() && _metrics.sessionNanoSeconds() >= _next_report) {
        // Time to produce a report.
        if (!produceReport()) {
            return TSP_END;
        }
        // Reset analysis context.
        if (!_cumulative) {
            _analyzer.reset();
        }
        // Compute next report time.
        _next_report += _output_interval;
    }

    return TSP_OK;
}
