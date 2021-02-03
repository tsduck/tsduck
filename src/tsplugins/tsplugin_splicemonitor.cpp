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
            uint32_t      event_id;         // Latest signaled event.
            uint64_t      event_pts;        // Latest signaled PTS (lowest PTS value in command).
            bool          event_out;        // Copy of splice_out for this event.
            size_t        event_count;      // Number of occurences of same insert commands for this event.
        };

        // Command line options:
        bool    _display_commands;  // Display the content of splice commands.
        bool    _all_commands;      // Display all splice commands.
        bool    _packet_index;      // Show packet index.
        bool    _use_log;           // Use tsp logger for messages.
        PID     _splice_pid;        // The only splice PID to monitor.
        PID     _pts_pid;           // The only PTS PID to use.
        UString _output_file;       // Output file name.

        // Working data:
        TablesDisplay               _display;          // Display engine for splice information tables.
        std::map<PID,SpliceContext> _splice_contexts;  // Map splice PID to splice context.
        std::map<PID,PID>           _splice_pids;      // Map audio/video PID to splice PID.
        SectionDemux                _section_demux;    // Section filter for splice information.
        SignalizationDemux          _sig_demux;        // Signalization demux to get PMT's.

        // Associate all audio/video PID's in a PMT to a splice PID.
        void setSplicePID(const PMT&, PID);

        // Report a one-line message.
        void message(PID splice_pid, const UChar* format, const std::initializer_list<ArgMixIn>& args = {});

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
    _splice_pid(PID_NULL),
    _pts_pid(PID_NULL),
    _output_file(),
    _display(duck),
    _splice_contexts(),
    _splice_pids(),
    _section_demux(duck, this),
    _sig_demux(duck, this)
{
    option(u"all-commands", 'a');
    help(u"all-commands",
         u"Same as --display-commands but display all SCTE-35 splice information commands. "
         u"By default, only display splice insert commands.");

    option(u"display-commands", 'd');
    help(u"display-commands",
         u"Display the content of SCTE-35 splice insert commands. "
         u"By default, only log a short event description.");

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
    getIntValue(_splice_pid, u"splice-pid", PID_NULL);
    getIntValue(_pts_pid, u"time-pid", PID_NULL);
    getValue(_output_file, u"output-file");
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

void ts::SpliceMonitorPlugin::message(PID splice_pid, const UChar* format, const std::initializer_list<ArgMixIn>& args)
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
            message(spid, u"canceled");
        }
        else if (si.immediate) {
            message(spid, u"immediately %s", {si.splice_out ? "OUT" : "IN"});
        }
        else {
            // This is a planned insert command. Is this a repetition or new event?
            if (si.event_id == ctx.event_id && event_pts == ctx.event_pts && si.splice_out == ctx.event_out) {
                ctx.event_count++;
            }
            else {
                ctx.event_id = si.event_id;
                ctx.event_pts = event_pts;
                ctx.event_out = si.splice_out;
                ctx.event_count = 1;
            }
            // Format time to event.
            UString time;
            if (ctx.last_pts != INVALID_PTS) {
                // Compute "current" PTS. We use the latest PTS found and adjust it by the distance to its packet.
                uint64_t current_pts = ctx.last_pts;
                const PacketCounter distance = tsp->pluginPackets() - ctx.last_pts_packet;
                const BitRate bitrate = tsp->bitrate();
                if (bitrate != 0 && distance != 0) {
                    current_pts += (distance * 8 * PKT_SIZE * SYSTEM_CLOCK_SUBFREQ) / bitrate;
                }
                if (current_pts > ctx.event_pts) {
                    time.format(u", event is in the past by %'d ms", {PTSToMilliSecond(current_pts - ctx.event_pts)});
                }
                else {
                    time.format(u", time to event: %'d ms", {PTSToMilliSecond(ctx.event_pts - current_pts)});
                }
            }
            message(spid, u"occurrence #%d%s", {ctx.event_count, time});
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
            message(spid, u"occurred");
            // Forget about this event, it is now in the past.
            ctx.event_id = SpliceInsert::INVALID_EVENT_ID;
            ctx.event_pts = INVALID_PTS;
        }
   }

    return TSP_OK;
}
