//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsSectionFile.h"
#include "tsSignalizationDemux.h"
#include "tsSpliceInformationTable.h"
#include "tsSpliceSegmentationDescriptor.h"
#include "tsForkPipe.h"
#include "tsjsonObject.h"
#include "tsjsonOutputArgs.h"
#include "tsjsonRunningDocument.h"
#include "tsxmlJSONConverter.h"
#include "tsxmlAttribute.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceMonitorPlugin:
            public ProcessorPlugin,
            private TableHandlerInterface,
            private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceMonitorPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // SCTE-35 splice event.
        class SpliceEvent
        {
        public:
            SpliceEvent() = default;                // Constructor.
            PacketCounter first_cmd_packet = 0;     // Packet index of first occurence of splice command for signaled event.
            uint32_t      event_id = SpliceInsert::INVALID_EVENT_ID;  // Signaled event id.
            uint64_t      event_pts = INVALID_PTS;  // Signaled PTS (lowest PTS value in command).
            bool          event_out = false;        // Copy of splice_out for this event.
            size_t        event_count = 0;          // Number of occurences of same insert commands for this event.
        };

        // Context of a PID containing SCTE-35 splice commands.
        class SpliceContext
        {
        public:
            SpliceContext() = default;                        // Constructor.
            uint64_t      last_pts = INVALID_PTS;             // Last PTS value in audio/video PID's for that splice PID.
            PacketCounter last_pts_packet = 0;                // Packet index of last PTS.
            std::map<uint32_t,SpliceEvent> splice_events {};  // Map event id to splice event.
        };

        // Command line options:
        bool             _packet_index = false;   // Show packet index.
        bool             _use_log = false;        // Use tsp logger for messages.
        bool             _no_adjustment = false;  // Do not adjust PTS of splice command reception time.
        bool             _time_stamp = false;     // Display time stamps with each table and JSON structure.
        PID              _splice_pid = PID_NULL;  // The only splice PID to monitor.
        PID              _pts_pid = PID_NULL;     // The only PTS PID to use.
        fs::path         _output_file {};         // Output file name.
        UString          _alarm_command {};       // Alarm command name.
        size_t           _min_repetition = 0;     // Minimum number of occurrences per command.
        size_t           _max_repetition = 0;     // Maximum number of occurrences per command.
        cn::milliseconds _min_preroll {};         // Minimum pre-roll time in milliseconds.
        cn::milliseconds _max_preroll {};         // Maximum pre-roll time in milliseconds.
        json::OutputArgs _json_args {};           // JSON output.
        std::bitset<256> _log_cmds {};            // List of splice commands to display.
        BinaryTable::XMLOptions _xml_options {};  // Options to format XML and JSON tables.

        // Working data:
        TablesDisplay               _display {duck};             // Display engine for splice information tables.
        bool                        _displayed_table = false;    // Just displayed a table.
        std::map<PID,SpliceContext> _splice_contexts {};         // Map splice PID to splice context.
        std::map<PID,PID>           _splice_pids {};             // Map audio/video PID to splice PID.
        SectionDemux                _section_demux {duck, this}; // Section filter for splice information.
        SignalizationDemux          _sig_demux {duck, this};     // Signalization demux to get PMT's.
        xml::JSONConverter          _x2j_conv {*tsp};            // XML-to-JSON converter.
        json::RunningDocument       _json_doc {*tsp};            // JSON document, built on-the-fly.

        // Associate all audio/video PID's in a PMT to a splice PID.
        void setSplicePID(const PMT&, PID);

        // Process an event.
        void processEvent(PID splice_pid, uint32_t event_id, uint64_t event_pts, bool canceled, bool immediate, bool splice_out);

        // Build and report a one-line message or JSON structure.
        UString message(PID splice_pid, uint32_t event_id, const UChar* format, std::initializer_list<ArgMixIn> args = {});
        void display(const UString& line);
        void initJSON(json::Object& obj, PID splice_pid, uint32_t event_id, const UString& progress, const SpliceContext& ctx, const SpliceEvent* evt);

        // Compute time between current packet and event. Return false if not possible to compute.
        bool timeToEvent(cn::milliseconds& tte, uint64_t event_pts, const SpliceContext& ctx);

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
    ProcessorPlugin(tsp_, u"Monitor SCTE 35 splice information", u"[options]")
{
    _json_args.defineArgs(*this, true, u"Build a JSON report into the specified file. Using '-' means standard output.");

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
         u"This is equivalent to --select-commands 0-255. "
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

    option<cn::milliseconds>(u"min-pre-roll-time");
    help(u"min-pre-roll-time",
         u"Specify a minimum pre-roll time in milliseconds for splice commands. "
         u"See option --alarm-command for non-nominal cases.");

    option<cn::milliseconds>(u"max-pre-roll-time");
    help(u"max-pre-roll-time",
         u"Specify a maximum pre-roll time in milliseconds for splice commands. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"meta-sections");
    help(u"meta-sections", u"Add hexadecimal dump of each section in XML and JSON metadata.");

    option(u"min-repetition", 0, POSITIVE);
    help(u"min-repetition",
         u"Specify a minimum number of repetitions for each splice command. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"max-repetition", 0, POSITIVE);
    help(u"max-repetition",
         u"Specify a maximum number of repetitions for each splice command. "
         u"See option --alarm-command for non-nominal cases.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"file-name",
         u"Specify an output text file. "
         u"With --json, this will be a JSON file. "
         u"By default, use the message logging system (or standard output with --display-commands).");

    option(u"packet-index", 'i');
    help(u"packet-index",
         u"Display the current TS packet index for each message or event.");

    option(u"select-commands", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"select-commands", u"value1[-value2]",
         u"Same as --display-commands but display the specified SCTE-35 command types only. "
         u"By default, only display splice insert commands. "
         u"Several --select-commands can be specified.");

    option(u"splice-pid", 's', PIDVAL);
    help(u"splice-pid",
         u"Specify one PID carrying SCTE-35 sections to monitor. "
         u"By default, all SCTE-35 PID's are monitored.");

    option(u"time-pid", 't', PIDVAL);
    help(u"time-pid",
         u"Specify one video or audio PID containing PTS time stamps to link with SCTE-35 sections to monitor. "
         u"By default, the PMT's are used to link between PTS PID's and SCTE-35 PID's.");

    option(u"time-stamp");
    help(u"time-stamp",
         u"Add a time stamp (current local time) inside each JSON structure (tables and events).");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::getOptions()
{
    _json_args.loadArgs(duck, *this);
    _xml_options.setPID = true;
    _xml_options.setPackets = _packet_index = present(u"packet-index");
    _xml_options.setLocalTime = _time_stamp = present(u"time-stamp");
    _xml_options.setSections = present(u"meta-sections");
    _no_adjustment = present(u"no-adjustment");
    getIntValue(_splice_pid, u"splice-pid", PID_NULL);
    getIntValue(_pts_pid, u"time-pid", PID_NULL);
    getPathValue(_output_file, u"output-file");
    getValue(_alarm_command, u"alarm-command");
    getChronoValue(_min_preroll, u"min-pre-roll-time");
    getChronoValue(_max_preroll, u"max-pre-roll-time");
    getIntValue(_min_repetition, u"min-repetition");
    getIntValue(_max_repetition, u"max-repetition");
    getIntValues(_log_cmds, u"select-commands");
    if (present(u"all-commands")) {
        _log_cmds.set(); // Display all splice commands
    }
    else if (present(u"display-commands")) {
        _log_cmds.set(SPLICE_INSERT); // Display splice insert commands
    }
    _use_log = _log_cmds.none() && _output_file.empty();
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
    _sig_demux.addFilteredTableId(TID_PMT);
    _section_demux.reset();
    _section_demux.setPIDFilter(NoPID);
    _displayed_table = false;

    // Starting demuxing on the splice PID if specified on the command line.
    if (_splice_pid != PID_NULL) {
        _section_demux.addPID(_splice_pid);
        if (_pts_pid != PID_NULL) {
            _splice_pids[_pts_pid] = _splice_pid;
        }
    }

    // If splice commands shall be displayed in JSON format, load the PSI/SI model into the JSON converter.
    if (_json_args.useJSON() && _log_cmds.any() && !SectionFile::LoadModel(_x2j_conv)) {
        return false;
    }

    // Open the output file when required.
    if (_json_args.useFile()) {
        json::ValuePtr root;
        return _json_doc.open(root, _output_file, std::cout);
    }
    else {
        return duck.setOutput(_output_file);
    }
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::stop()
{
    // Close the output file when required and return to stdout.
    _json_doc.close();
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
        for (const auto& it : pmt.streams) {
            if (it.second.stream_type == ST_SCTE35_SPLICE) {
                // This is a PID carrying splice information.
                const PID spid = it.first;
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
    for (const auto& it : pmt.streams) {
        if (it.second.isAudio(duck) || it.second.isVideo(duck)) {
            _splice_pids[it.first] = splice_pid;
        }
    }
}


//----------------------------------------------------------------------------
// Build a one-line message.
//----------------------------------------------------------------------------

ts::UString ts::SpliceMonitorPlugin::message(PID splice_pid, uint32_t event_id, const UChar* format, std::initializer_list<ArgMixIn> args)
{
    UString line;
    if (_packet_index) {
        line.format(u"packet %'d, ", {tsp->pluginPackets()});
    }
    if (splice_pid != PID_NULL) {
        SpliceContext& ctx(_splice_contexts[splice_pid]);
        line.format(u"splice PID 0x%X (%<d), ", {splice_pid});
        if (event_id != SpliceInsert::INVALID_EVENT_ID) {
            const SpliceEvent& evt(ctx.splice_events[event_id]);
            line.format(u"event 0x%X (%<d) %d, ", {evt.event_id, evt.event_out ? u"out" : u"in"});
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
        if (_displayed_table) {
            _displayed_table = false;
            _display << std::endl;
        }
        _display << "* " << line << std::endl;
    }
}


//----------------------------------------------------------------------------
// Initialize a JSON structure.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::initJSON(json::Object& obj, PID splice_pid, uint32_t event_id, const UString& progress, const SpliceContext& ctx, const SpliceEvent* evt)
{
    const Time now(Time::CurrentLocalTime());
    obj.add(u"#name", u"event");
    obj.add(u"packet-index", int64_t(tsp->pluginPackets()));
    obj.add(u"progress", progress);
    if (_time_stamp) {
        // Make sure to sure the same time format as XML attributes.
        obj.add(u"time", xml::Attribute::DateTimeToString(now));
    }
    if (splice_pid != PID_NULL) {
        obj.add(u"splice-pid", int64_t(splice_pid));
    }
    if (event_id != SpliceInsert::INVALID_EVENT_ID) {
        obj.add(u"event-id", int64_t(event_id));
    }
    if (evt != nullptr) {
        obj.add(u"event-type", evt->event_out ? u"out" : u"in");
        obj.add(u"event-pts", int64_t(evt->event_pts));
        obj.add(u"count", int64_t(evt->event_count));
        cn::milliseconds time_to_event = cn::milliseconds::zero();
        if (timeToEvent(time_to_event, evt->event_pts, ctx)) {
            obj.add(u"time-to-event-ms", time_to_event.count());
            if (_time_stamp) {
                obj.add(u"event-time", xml::Attribute::DateTimeToString(now + time_to_event));
            }
        }
    }
}


//----------------------------------------------------------------------------
// Compute time between current packet and event.
//----------------------------------------------------------------------------

bool ts::SpliceMonitorPlugin::timeToEvent(cn::milliseconds& tte, uint64_t event_pts, const SpliceContext& ctx)
{
    if (ctx.last_pts == INVALID_PTS) {
        // No possible to compute a time to event.
        return false;
    }
    else {
        // Compute "current" PTS. We use the latest PTS found and adjust it by the distance to its packet.
        uint64_t current_pts = ctx.last_pts;
        if (!_no_adjustment) {
            const PacketCounter distance = tsp->pluginPackets() - ctx.last_pts_packet;
            const BitRate bitrate = tsp->bitrate();
            if (bitrate != 0 && distance != 0) {
                current_pts += ((distance * PKT_SIZE_BITS * SYSTEM_CLOCK_SUBFREQ) / bitrate).toInt();
            }
        }
        tte = cn::duration_cast<cn::milliseconds>(ts::pts_dts_units(event_pts - current_pts));
        return true;
    }
}


//----------------------------------------------------------------------------
// Process an event.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::processEvent(PID splice_pid, uint32_t event_id, uint64_t event_pts, bool canceled, bool immediate, bool splice_out)
{
    // Locate PID context and event description (if it exists).
    SpliceContext& ctx(_splice_contexts[splice_pid]);
    auto evt = ctx.splice_events.find(event_id);
    bool known_event = evt != ctx.splice_events.end();

    // Display event depending on canceled/immediate/pending.
    if (canceled) {
        if (_json_args.useJSON()) {
            json::Object obj;
            initJSON(obj, splice_pid, event_id, u"canceled", ctx, known_event ? &evt->second : nullptr);
            _json_args.report(obj, _json_doc, *tsp);
        }
        else {
            display(message(splice_pid, event_id, u"canceled"));
        }
        if (known_event) {
            // Canceled event -> remove it.
            ctx.splice_events.erase(evt);
        }
    }
    else if (immediate) {
        if (_json_args.useJSON()) {
            json::Object obj;
            initJSON(obj, splice_pid, event_id, u"immediate", ctx, known_event ? &evt->second : nullptr);
            obj.add(u"event-type", splice_out ? u"out" : u"in");
            _json_args.report(obj, _json_doc, *tsp);
        }
        else {
            display(message(splice_pid, event_id, u"immediately %s", {splice_out ? "OUT" : "IN"}));
        }
        if (known_event) {
            // Immediate event, won't reference it later -> remove it.
            ctx.splice_events.erase(evt);
        }
    }
    else {
        // This is a planned insert command. Is this a repetition or new event?
        if (known_event) {
            // Repetition of a previous event.
            evt->second.event_count++;
        }
        else {
            // First command about a new event.
            ctx.splice_events[event_id].event_id = event_id;
            evt = ctx.splice_events.find(event_id);
            evt->second.event_pts = event_pts;
            evt->second.event_out = splice_out;
            evt->second.event_count = 1;
            evt->second.first_cmd_packet = tsp->pluginPackets();
        }
        if (_json_args.useJSON()) {
            json::Object obj;
            initJSON(obj, splice_pid, event_id, u"pending", ctx, &evt->second);
            _json_args.report(obj, _json_doc, *tsp);
        }
        else {
            // Format time to event.
            UString time;
            cn::milliseconds time_to_event;
            if (timeToEvent(time_to_event, event_pts, ctx)) {
                if (time_to_event < cn::milliseconds::zero()) {
                    time.format(u", event is in the past by %'d ms", {-time_to_event.count()});
                }
                else {
                    time.format(u", time to event: %'d ms", {time_to_event.count()});
                }
            }
            display(message(splice_pid, event_id, u"occurrence #%d%s", {evt->second.event_count, time}));
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a splice information section is available.
//----------------------------------------------------------------------------

void ts::SpliceMonitorPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Convert to a Splice Information Table.
    SpliceInformationTable sit(duck, table);
    if (!sit.isValid()) {
        // Was not a Splice Information Table.
        return;
    }

    if (sit.splice_command_type == SPLICE_TIME_SIGNAL && sit.time_signal.has_value()) {
        sit.adjustPTS();
        for (size_t di = 0; di < sit.descs.count(); ++di) {
            if (sit.descs[di]->tag() == DID_SPLICE_SEGMENT) {
                // SCTE 35 SIT segmentation_descriptor.
                const SpliceSegmentationDescriptor ssd(duck, *sit.descs[di]);
                if (ssd.isValid() && (ssd.isIn() || ssd.isOut())) {
                    processEvent(table.sourcePID(), ssd.segmentation_event_id, sit.time_signal.value(), ssd.segmentation_event_cancel, false, ssd.isOut());
                }
            }
        }
    }
    else if (sit.splice_command_type == SPLICE_INSERT) {
        // Get a copy of the splice insert command and adjust all PTS to actual time value.
        SpliceInsert si(sit.splice_insert);
        si.adjustPTS(sit.pts_adjustment);
        processEvent(table.sourcePID(), si.event_id, si.lowestPTS(), si.canceled, si.immediate, si.splice_out);
    }

    // Finally, display the SCTE-35 table.
    if (_log_cmds.test(sit.splice_command_type)) {
        if (_json_args.useJSON()) {
            // Format the SCTE-35 table using JSON. First, build an XML document with the table.
            xml::Document doc(*tsp);
            doc.initialize(u"tsduck");
            table.toXML(duck, doc.rootElement(), _xml_options);
            // Convert the XML document into JSON and get the first (and only) table.
            _json_args.report(_x2j_conv.convertToJSON(doc, true)->query(u"#nodes[0]"), _json_doc, *tsp);
        }
        else {
            // Human-readable display of the SCTE-35 table.
            if (_displayed_table) {
                _display << std::endl;
            }
            _display.displayTable(table);
            _displayed_table = true;
        }
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

        for (auto it = ctx.splice_events.begin(); it != ctx.splice_events.end(); ) {
            SpliceEvent& evt(it->second);

            // Look for event occurrence.
            if (evt.event_id != SpliceInsert::INVALID_EVENT_ID && evt.event_pts != INVALID_PTS && ctx.last_pts >= evt.event_pts) {

                // Evaluate time since first command. Assume constant bitrate since then.
                const cn::milliseconds preroll = PacketInterval(tsp->bitrate(), tsp->pluginPackets() - evt.first_cmd_packet);

                // Check if outside nominal range.
                const bool alarm =
                    (_min_preroll != cn::milliseconds::zero() && preroll != cn::milliseconds::zero() && preroll < _min_preroll) ||
                    (_max_preroll != cn::milliseconds::zero() && preroll > _max_preroll) ||
                    (_min_repetition != 0 && evt.event_count < _min_repetition) ||
                    (_max_repetition != 0 && evt.event_count > _max_repetition);

                // Build a one-line message.
                UString line(message(spid, evt.event_id, u"occurred"));
                if (preroll > cn::milliseconds::zero()) {
                    line.format(u", actual pre-roll time: %'d ms", {preroll.count()});
                }

                // Display the event.
                if (_json_args.useJSON()) {
                    json::Object obj;
                    initJSON(obj, spid, evt.event_id, u"occurred", ctx, &evt);
                    obj.add(u"status", alarm ? u"alarm" : u"normal");
                    obj.add(u"pre-roll-ms", preroll.count());
                    _json_args.report(obj, _json_doc, *tsp);
                }
                else {
                    display(line);
                }

                // Raise alarm if outside nominal range.
                if (!_alarm_command.empty() && alarm) {
                    UString command;
                    command.format(u"%s \"%s\" %d %d %s %d %d %d",
                                   {_alarm_command, line, spid, evt.event_id, evt.event_out ? u"out" : u"in", evt.event_pts, preroll.count(), evt.event_count});
                    ForkPipe::Launch(command, *tsp, ForkPipe::STDERR_ONLY, ForkPipe::STDIN_NONE);
                }

                // Forget about this event, it is now in the past.
                it = ctx.splice_events.erase(it);
            }
            else {
                // This is still a future event, move to next event.
                ++it;
            }
        }
    }

    return TSP_OK;
}
