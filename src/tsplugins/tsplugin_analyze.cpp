//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Transport stream analyzer.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsTSAnalyzerReport.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class AnalyzePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        AnalyzePlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        UString           _output_name;
        std::ofstream     _output_stream;
        std::ostream*     _output;
        MilliSecond       _output_interval;
        bool              _multiple_output;
        PacketCounter     _current_packet;
        Time              _next_report_time;
        PacketCounter     _next_report_packet;
        TSAnalyzerReport  _analyzer;
        TSAnalyzerOptions _analyzer_options;

        bool openOutput();
        void closeOutput();
        bool produceReport();
        void computeNextReportTime (const Time& current_utc, MilliSecond interval);

        // Inaccessible operations
        AnalyzePlugin() = delete;
        AnalyzePlugin(const AnalyzePlugin&) = delete;
        AnalyzePlugin& operator=(const AnalyzePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(analyze, ts::AnalyzePlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AnalyzePlugin::AnalyzePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Transport Stream Analyzer.", u"[options]"),
    _output_name(),
    _output_stream(),
    _output(),
    _output_interval(0),
    _multiple_output(false),
    _current_packet(0),
    _next_report_time(Time::Epoch),
    _next_report_packet(0),
    _analyzer(),
    _analyzer_options()
{
    option(u"interval",       'i', POSITIVE);
    option(u"multiple-files", 'm');
    option(u"output-file",    'o', STRING);
    copyOptions(_analyzer_options);

    _analyzer_options.setHelp(
        u"Options:\n"
        u"\n"
        u"  --help\n"
        u"      Display this help text.\n"
        u"\n"
        u"  -i seconds\n"
        u"  --interval seconds\n"
        u"      Produce a new output file at regular intervals. After outputing a file,\n"
        u"      the analysis context is reset, ie. each output file contains a fully\n"
        u"      independent analysis.\n"
        u"\n"
        u"  -m\n"
        u"  --multiple-files\n"
        u"      When used with --interval and --output-file, create a new file for each\n"
        u"      analysis instead of rewriting the previous file. Assuming that the\n"
        u"      specified output file name has the form 'base.ext', each file is created\n"
        u"      with a time stamp in its name as 'base_YYYYMMDD_hhmmss.ext'.\n"
        u"\n"
        u"  -o filename\n"
        u"  --output-file filename\n"
        u"      Specify the output text file for the analysis result.\n"
        u"      By default, use the standard output.\n"
        u"\n"
        u"  --version\n"
        u"      Display the version number.\n");

    setHelp(_analyzer_options.getHelp());
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AnalyzePlugin::start()
{
    _output_name = value(u"output-file");
    _output_interval = MilliSecPerSec * intValue<MilliSecond>(u"interval", 0);
    _multiple_output = present(u"multiple-files");
    _output = _output_name.empty() ? &std::cout : &_output_stream;
    _analyzer_options.getOptions (*this);
    _analyzer.setAnalysisOptions (_analyzer_options);
    _current_packet = 0;

    // Create the output file. Note that this file is used only in the stop
    // method and could be created there. However, if the file cannot be
    // created, we do not want to wait all along the analysis and finally fail.
    if (_output_interval == 0 && !openOutput()) {
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
    UString name;
    if (_multiple_output) {
        const Time::Fields now(Time::CurrentLocalTime());
        name = UString::Format(u"%s_%04d%02d%02d_%02d%02d%02d%s", {PathPrefix(_output_name), now.year, now.month, now.day, now.hour, now.minute, now.second, PathSuffix(_output_name)});
    }
    else {
        name = _output_name;
    }

    // Create the file
    _output_stream.open(name.toUTF8().c_str());
    if (_output_stream) {
        return true;
    }
    else {
        tsp->error(u"cannot create file %s", {name});
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
        _analyzer.setBitrateHint(tsp->bitrate());

        // Produce the report
        _analyzer.report(*_output, _analyzer_options);
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
// Compute next time to produce a report
//----------------------------------------------------------------------------

void ts::AnalyzePlugin::computeNextReportTime (const Time& current_utc, MilliSecond interval)
{
    // Compute next report time
    _next_report_time = current_utc + interval;

    // To avoid getting system time every packet, try to guess the packet index for next report time
    const BitRate bitrate = tsp->bitrate();
    if (bitrate == 0) {
        // Unknown bitrate, we have to check current time every packet
        _next_report_packet = 0;
    }
    else {
        _next_report_packet = _current_packet + PacketDistance(bitrate, interval);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AnalyzePlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Count packets in transport stream
    _current_packet++;

    // Feed the analyzer with one packet
    _analyzer.feedPacket (pkt);

    // With --interval, check if it is time to produce a report
    if (_output_interval > 0) {
        if (_current_packet == 1) {
            // Initialize the repetition when the first packet arrives
            computeNextReportTime(Time::CurrentUTC(), _output_interval);
        }
        else if (_next_report_packet == 0 || (_next_report_packet > 0 && _current_packet >= _next_report_packet)) {
            // Check current time to see if this is time to produce a report
            const Time current_utc(Time::CurrentUTC());
            if (current_utc < _next_report_time) {
                // False alarm, we have to wait some more
                computeNextReportTime(current_utc, _next_report_time - current_utc);
            }
            else {
                // Time to produce a report
                if (!produceReport()) {
                    return TSP_END;
                }
                // Reset analysis context
                _analyzer.reset();
                computeNextReportTime (current_utc, _output_interval);
            }
        }
    }

    return TSP_OK;
}
