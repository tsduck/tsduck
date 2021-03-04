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
//
//  Transport stream processor shared library:
//  Monitor SCTE 35 splice information.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsSpliceInformationTable.h"
#include "tsForkPipe.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceMonitorPlugin:
            public ProcessorPlugin,
            private TableHandlerInterface,
            private SignalizationHandlerInterface
    {
        TS_NOBUILD_NOCOPY(SpliceMonitorPlugin);
    public:
        // Implementation of plugin API
        SpliceMonitorPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Context of a PID containing SCTE-35 splice commands.
        class SpliceContext
        {
        public:
            SpliceContext();                // Constructor.
            uint64_t      last_pts;         // Last PTS value in audio/video PID's for that splice PID.
            PacketCounter last_pts_packet;  // Packet index of last PTS.
            PacketCounter first_cmd_packet; // Packet index of first occurence of splice command for latest signaled event.
            uint32_t      event_id;         // Latest signaled event.
            uint64_t      event_pts;        // Latest signaled PTS (lowest PTS value in command).
            bool          event_out;        // Copy of splice_out for this event.
            size_t        event_count;      // Number of occurences of same insert commands for this event.
        };

        // Command line options:
        bool        _display_commands;  // Display the content of splice commands.
        bool        _all_commands;      // Display all splice commands.
        bool        _packet_index;      // Show packet index.
        bool        _use_log;           // Use tsp logger for messages.
        bool        _no_adjustment;     // Do not adjust PTS of splice command reception time.
        PID         _splice_pid;        // The only splice PID to monitor.
        PID         _pts_pid;           // The only PTS PID to use.
        UString     _output_file;       // Output file name.
        UString     _alarm_command;     // Alarm command name.
        size_t      _min_repetition;    // Minimum number of occurrences per command.
        size_t      _max_repetition;    // Maximum number of occurrences per command.
        MilliSecond _min_preroll;       // Minimum pre-roll time in milliseconds.
        MilliSecond _max_preroll;       // Maximum pre-roll time in milliseconds.

        // Working data:
        TablesDisplay               _display;          // Display engine for splice information tables.
        std::map<PID,SpliceContext> _splice_contexts;  // Map splice PID to splice context.
        std::map<PID,PID>           _splice_pids;      // Map audio/video PID to splice PID.
        SectionDemux                _section_demux;    // Section filter for splice information.
        SignalizationDemux          _sig_demux;        // Signalization demux to get PMT's.

        // Associate all audio/video PID's in a PMT to a splice PID.
        void setSplicePID(const PMT&, PID);

        // Build and report a one-line message.
        UString message(PID splice_pid, const UChar* format, const std::initializer_list<ArgMixIn>& args = {});
        void display(const UString& line);

        // Implementation of interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handlePMT(const PMT&, PID) override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"splicemonitor", ts::SpliceMonitorPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SpliceMonitorPlugin::SpliceMonitorPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Monitor SCTE 35 splice information", u"[options]"),
    _display_commands(false),
    _all_commands(false),
    _packet_index(false),
    _use_log(false),
    _no_adjustment(false),
    _splice_pid(PID_NULL),
    _pts_pid(PID_NULL),
    _output_file(),
    _alarm_command(),
    _min_repetition(0),
    _max_repetition(0),
    _min_preroll(0),
    _max_preroll(0),
    _display(duck),
    _splice_contexts(),
    _splice_pids(),
    _section_demux(duck, this),
    _sig_demux(duck, this)
{
    option(u"alarm-command", 0, STRING);
    help(u"alarm-command", u"'command'",
         u"Command to run when a splice event is outside the nominal range as specified by other --min and --max options. "
         u"The command receives seven additional parameters:\n\n"
         u"1. A human-readable message, the same as logged by the plugin.\n"
         u"2. The PID of the splice command.\n"
         u"3. The event id.\n"
         u"4. The string \"in\" or \"out\" for splice in / splice out command.\n"
         u"5. The adjusted PTS value in the splice command.\n"
         u"6. Pre-roll time in milliseconds.\n"
         u"7. Number of occurences of the command before the event.");

    option(u"all-commands", 'a');
    help(u"all-commands",
         u"Same as --display-commands but display all SCTE-35 splice information commands. "
         u"By default, only display splice insert commands.");

    option(u"display-commands", 'd');
    help(u"display-commands",
         u"Display the content of SCTE-35 splice insert commands. "
         u"By default, only log a short event description.");

    option(u"no-adjustment", 'n');
    help(u"no-adjustment",
         u"When computing the anticipated pre-roll time at reception of a splice command, "
         u"do not try to adjust the time using the distance between the last PTS and the splice command. "
         u"By default, use the bitrate to adjust the supposed PTS of the splice command itself.");

    option(u"min-pre-roll-time", 0, POSITIVE);
    help(u"min-pre-roll-time",
         u"Specify a minimum pre-roll time in milliseconds for splice commands. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"max-pre-roll-time", 0, POSITIVE);
    help(u"max-pre-roll-time",
         u"Specify a maximum pre-roll time in milliseconds for splice commands. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"min-repetition", 0, POSITIVE);
    help(u"min-repetition",
         u"Specify a minimum number of repetitions for each splice command. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"max-repetition", 0, POSITIVE);
    help(u"max-repetition",
         u"Specify a maximum number of repetitions for each splice command. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"output-file", 'o', STRING);
    help(u"output-file", u"file-name",
         u"Specify an output text file. "
         u"By default, use the message logging system (or standard output with --display-commands).");

    option(u"packet-index", 'i');
    help(u"packet-index",
         u"Display the current TS packet index for each message or event.");

    option(u"splice-pid", 's', PIDVAL);
    help(u"splice-pid",
         u"Specify one PID carrying SCTE-35 sections to monitor. "
         u"By default, all SCTE-35 PID's are monitored.");

    option(u"time-pid", 't', PIDVAL);
    help(u"time-pid",
         u"Specify one video or audio PID containing PTS time stamps to link with SCTE-35 sections to monitor. "
         u"By default, the PMT's are used to link between PTS PID's and SCTE-35 PID's.");
}


//----------------------------------------------------------------------------
// Context of a PID containing SCTE-35 splice commands.
//----------------------------------------------------------------------------

ts::SpliceMonitorPlugin::SpliceContext::SpliceContext() :
    last_pts(INVALID_PTS),
    last_pts_packet(0),
    first_cmd_packet(0),
    event_id(SpliceInsert::INVALID_EVENT_ID),
    event_pts(INVALID_PTS),
    event_out(false),
    event_count(0)
{
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::getOptions()
{
    _all_commands = present(u"all-commands");
    _display_commands = _all_commands || present(u"display-commands");
    _packet_index = present(u"packet-index");
    _no_adjustment = present(u"no-adjustment");
    getIntValue(_splice_pid, u"splice-pid", PID_NULL);
    getIntValue(_pts_pid, u"time-pid", PID_NULL);
    getValue(_output_file, u"output-file");
    getValue(_alarm_command, u"alarm-command");
    getIntValue(_min_preroll, u"min-pre-roll-time");
    getIntValue(_max_preroll, u"max-pre-roll-time");
    getIntValue(_min_repetition, u"min-repetition");
    getIntValue(_max_repetition, u"max-repetition");
    _use_log = !_display_commands && _output_file.empty();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::start()
{
    // Cleanup state.
    _splice_contexts.clear();
    _splice_pids.clear();
    _sig_demux.reset();
    _sig_demux.addTableId(TID_PMT);
    _section_demux.reset();
    _section_demux.setPIDFilter(NoPID);

    // Starting demuxing on the splice PID if specified on the command line.
    if (_splice_pid != PID_NULL) {
        _section_demux.addPID(_splice_pid);
        if (_pts_pid != PID_NULL) {
            _splice_pids[_pts_pid] = _splice_pid;
        }
    }

    // Open the output file when required.
    return duck.setOutput(_output_file);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::stop()
{
    // Close the output file when required and return to stdout.
    return duck.setOutput(u"");
}


//----------------------------------------------------------------------------
// Invoked by the signalization demux when a PMT is found.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::handlePMT(const PMT& pmt, PID)
{
    if (_splice_pid != PID_NULL && _pts_pid == PID_NULL) {
        // All audio/video PID's point to the same user-defined splice PID.
        setSplicePID(pmt, _splice_pid);
    }
    else {
        // Analyze all components in the PMT, looking for splice PID's.
        for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
            if (it->second.stream_type == ST_SCTE35_SPLICE) {
                // This is a PID carrying splice information.
                const PID spid = it->first;
                if (_splice_pid == PID_NULL || _splice_pid == spid) {
                    // This is a splice PID to monitor.
                    tsp->verbose(u"starting monitoring splice PID 0x%X (%<d)", {spid});
                    _section_demux.addPID(spid);
                    if (_pts_pid != PID_NULL) {
                        // One single user-defined audio/video PID.
                        _splice_pids[_pts_pid] = spid;
                    }
                    else {
                        // Associate audio/video PID's in this service with this splice PID.
                        setSplicePID(pmt, spid);
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Associate all audio/video PID's in a PMT to a splice PID.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::setSplicePID(const PMT& pmt, PID splice_pid)
{
    for (auto it = pmt.streams.begin(); it != pmt.streams.end(); ++it) {
        if (it->second.isAudio(duck) || it->second.isVideo(duck)) {
            _splice_pids[it->first] = splice_pid;
        }
    }
}


//----------------------------------------------------------------------------
// Report a one-line message.
//----------------------------------------------------------------------------

ts::UString ts::SpliceMonitorPlugin::message(PID splice_pid, const UChar* format, const std::initializer_list<ArgMixIn>& args)
{
    UString line;
    if (_packet_index) {
        line.format(u"packet %'d, ", {tsp->pluginPackets()});
    }
    if (splice_pid != PID_NULL) {
        const SpliceContext& ctx(_splice_contexts[splice_pid]);
        line.format(u"splice PID 0x%X (%<d), ", {splice_pid});
        if (ctx.event_id != SpliceInsert::INVALID_EVENT_ID) {
            line.format(u"event 0x%X (%<d) %d, ", {ctx.event_id, ctx.event_out ? u"out" : u"in"});
        }
    }
    line.format(format, args);
    return line;
}


//----------------------------------------------------------------------------
// Report a one-line message.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::display(const UString& line)
{
    if (_use_log) {
        tsp->info(line);
    }
    else {
        if (_display_commands) {
            _display << std::endl;
        }
        _display << "* " << line << std::endl;
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a splice information section is available.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Convert to a Splice Information Table.
    const SpliceInformationTable sit(duck, table);
    if (!sit.isValid()) {
        // Was not a Splice Information Table.
        return;
    }
    const bool is_insert = sit.splice_command_type == SPLICE_INSERT;

    // Give up now if there is nothing to display.
    if (!is_insert && !_all_commands) {
        return;
    }

    // Splice context.
    const PID spid = table.sourcePID();
    SpliceContext& ctx(_splice_contexts[spid]);

    if (!is_insert) {
        // Not an insert command, just display it without initial message.
        _display << std::endl;
    }
    else {
        // Get a copy of the splice insert command and adjust all PTS to actual time value.
        SpliceInsert si(sit.splice_insert);
        si.adjustPTS(sit.pts_adjustment);
        const uint64_t event_pts = si.lowestPTS();

        if (si.canceled) {
            display(message(spid, u"canceled"));
        }
        else if (si.immediate) {
            display(message(spid, u"immediately %s", {si.splice_out ? "OUT" : "IN"}));
        }
        else {
            // This is a planned insert command. Is this a repetition or new event?
            if (si.event_id == ctx.event_id && event_pts == ctx.event_pts && si.splice_out == ctx.event_out) {
                // Repetition of a previous event.
                ctx.event_count++;
            }
            else {
                // First command about a new event.
                ctx.event_id = si.event_id;
                ctx.event_pts = event_pts;
                ctx.event_out = si.splice_out;
                ctx.event_count = 1;
                ctx.first_cmd_packet = tsp->pluginPackets();
            }
            // Format time to event.
            UString time;
            if (ctx.last_pts != INVALID_PTS) {
                // Compute "current" PTS. We use the latest PTS found and adjust it by the distance to its packet.
                uint64_t current_pts = ctx.last_pts;
                if (!_no_adjustment) {
                    const PacketCounter distance = tsp->pluginPackets() - ctx.last_pts_packet;
                    const BitRate bitrate = tsp->bitrate();
                    if (bitrate != 0 && distance != 0) {
                        current_pts += (distance * 8 * PKT_SIZE * SYSTEM_CLOCK_SUBFREQ) / bitrate;
                    }
                }
                if (current_pts > ctx.event_pts) {
                    time.format(u", event is in the past by %'d ms", {PTSToMilliSecond(current_pts - ctx.event_pts)});
                }
                else {
                    time.format(u", time to event: %'d ms", {PTSToMilliSecond(ctx.event_pts - current_pts)});
                }
            }
            display(message(spid, u"occurrence #%d%s", {ctx.event_count, time}));
        }
    }

    // Finally, display the SCTE-35 table.
    if (_display_commands) {
        _display.displayTable(table);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SpliceMonitorPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Feed the various analyzers with the packet.
    _section_demux.feedPacket(pkt);
    _sig_demux.feedPacket(pkt);

    // Is this a video/audio PID which is associated to a splicing PID?
    if (pkt.hasPTS() && _splice_pids.find(pid) != _splice_pids.end()) {
        // Remember the latest PTS value for this splice PID.
        const PID spid = _splice_pids[pid];
        SpliceContext& ctx(_splice_contexts[spid]);
        ctx.last_pts = pkt.getPTS();
        ctx.last_pts_packet = tsp->pluginPackets();

        // Look for event occurrence.
        if (ctx.event_id != SpliceInsert::INVALID_EVENT_ID && ctx.event_pts != INVALID_PTS && ctx.last_pts >= ctx.event_pts) {
            // Evaluate time since first command. Assume constant bitrate since then.
            const MilliSecond preroll = PacketInterval(tsp->bitrate(), tsp->pluginPackets() - ctx.first_cmd_packet);
            UString line(message(spid, u"occurred"));
            if (preroll > 0) {
                line.format(u", actual pre-roll time: %'d ms", {preroll});
            }
            display(line);
            // Raise alarm if outside nominal range.
            if (!_alarm_command.empty() &&
                ((_min_preroll != 0 && preroll != 0 && preroll < _min_preroll) ||
                 (_max_preroll != 0 && preroll > _max_preroll) ||
                 (_min_repetition != 0 && ctx.event_count < _min_repetition) ||
                 (_max_repetition != 0 && ctx.event_count > _max_repetition)))
            {
                UString command;
                command.format(u"%s \"%s\" %d %d %s %d %d %d",
                               {_alarm_command, line, spid, ctx.event_id, ctx.event_out ? u"out" : u"in", ctx.event_pts, preroll, ctx.event_count});
                ForkPipe::Launch(command, *tsp, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
            }
            // Forget about this event, it is now in the past.
            ctx.event_id = SpliceInsert::INVALID_EVENT_ID;
            ctx.event_pts = INVALID_PTS;
        }
   }

    return TSP_OK;
}
