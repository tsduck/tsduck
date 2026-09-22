//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to monitor SCTE-35 splice information.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsSpliceInsert.h"
#include "tsTSClock.h"
#include "tsInfluxSender.h"
#include "tsjsonObject.h"
#include "tsjsonOutputArgs.h"
#include "tsjsonRunningDocument.h"
#include "tsxmlJSONConverter.h"

namespace ts {
    //!
    //! Plugin to monitor SCTE-35 splice information.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL SpliceMonitorPlugin: public ProcessorPlugin, private TableHandlerInterface, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceMonitorPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // SCTE-35 splice event.
        class SpliceEvent
        {
        public:
            SpliceEvent() = default;                   // Constructor.
            PacketCounter first_cmd_packet = 0;        // Packet index of first occurrence of splice command for signaled event.
            uint32_t      event_id = SpliceInsert::INVALID_EVENT_ID;  // Signaled event id.
            uint64_t      event_pts = INVALID_PTS;     // Signaled PTS (lowest PTS value in command).
            uint64_t      duration_pts = INVALID_PTS;  // Event duration in PTS units.
            size_t        event_count = 0;             // Number of occurrences of same insert commands for this event.
            bool          event_out = false;           // Copy of splice_out for this event.
        };

        // Context of a PID containing SCTE-35 splice commands.
        class SpliceContext
        {
        public:
            SpliceContext() = default;                        // Constructor.
            uint64_t      last_pts = INVALID_PTS;             // Last PTS value in audio/video PID's for that splice PID.
            PacketCounter last_pts_packet = 0;                // Packet index of last PTS.
            uint64_t      last_pcr = INVALID_PTS;             // Last PCR value in audio/video PID's for that splice PID.
            Time          last_pcr_clock {};                  // TSClock value of last PCR.
            std::map<uint32_t,SpliceEvent> splice_events {};  // Map event id to splice event.
        };

        // State of an event, can be used as a bitmask to select several of them.
        enum EventState {
            EV_NONE      = 0x0000,
            EV_SIGNALLED = 0x0001,
            EV_IMMEDIATE = 0x0002,
            EV_CANCELLED = 0x0004,
            EV_OCCURRED  = 0x0008,
            EV_ALL       = 0x000F
        };
        const Names _eventStateEnum{
            {u"none",      EV_NONE},
            {u"signalled", EV_SIGNALLED},
            {u"immediate", EV_IMMEDIATE},
            {u"cancelled", EV_CANCELLED},
            {u"occurred",  EV_OCCURRED},
            {u"all",       EV_ALL},
        };

        // Command line options:
        bool             _packet_index = false;    // Show packet index.
        bool             _use_log = false;         // Use tsp logger for messages.
        bool             _no_adjustment = false;   // Do not adjust PTS of splice command reception time.
        bool             _time_stamp = false;      // Display time stamps with each table and JSON structure.
        PID              _splice_pid = PID_NULL;   // The only splice PID to monitor.
        PID              _pts_pid = PID_NULL;      // The only PTS PID to use.
        fs::path         _output_file {};          // Output file name.
        UString          _alarm_command {};        // Alarm command name.
        UString          _tag {};                  // Message tag.
        EventState       _influx_states = EV_NONE; // Send to InfluxDB the specified states (bitmask).
        size_t           _min_repetition = 0;      // Minimum number of occurrences per command.
        size_t           _max_repetition = 0;      // Maximum number of occurrences per command.
        cn::milliseconds _min_preroll {};          // Minimum pre-roll time in milliseconds.
        cn::milliseconds _max_preroll {};          // Maximum pre-roll time in milliseconds.
        json::OutputArgs _json_args {this};        // JSON output.
        std::bitset<256> _log_cmds {};             // List of splice commands to display.
        TSClockArgs      _ts_clock_args {u"influx"};
        InfluxArgs       _influx_args {true, false};
        BinaryTable::XMLOptions _xml_options {};   // Options to format XML and JSON tables.

        // Working data:
        TablesDisplay               _display {duck};             // Display engine for splice information tables.
        bool                        _displayed_table = false;    // Just displayed a table.
        std::map<PID,SpliceContext> _splice_contexts {};         // Map splice PID to splice context.
        std::map<PID,std::set<PID>> _splice_pids {};             // Map audio/video PID to a set of splice PIDs.
        SectionDemux                _section_demux {duck, this}; // Section filter for splice information.
        SignalizationDemux          _sig_demux {duck, this};     // Signalization demux to get PMT's.
        TSClock                     _ts_clock {duck};            // Compute playout time based on real time, PCR or input timestamps.
        InfluxSender                _influx_server {this};       // Send requests to InfluxDB server.
        xml::JSONConverter          _x2j_conv {*this};           // XML-to-JSON converter.
        json::RunningDocument       _json_doc {*this};           // JSON document, built on-the-fly.

        // Associate all audio/video PID's in a PMT to a splice PID.
        void setSplicePID(const PMT&, PID);

        // Process an event.
        void processEvent(PID splice_pid, uint32_t event_id, uint64_t event_pts, uint64_t duration_pts, bool canceled, bool immediate, bool splice_out);

        // Report an event to InfluxDB if necessary.
        void sendInflux(PID splice_pid, const SpliceEvent& event, EventState state, cn::milliseconds preroll);

        // Build and report a one-line message or JSON structure.
        void display(const UString& line);
        void initJSON(json::Object& obj, PID splice_pid, uint32_t event_id, const UString& progress, const SpliceContext& ctx, const SpliceEvent* evt);
        UString header(PID splice_pid, uint32_t event_id);

        template <class... Args>
        UString message(PID splice_pid, uint32_t event_id, const UChar* format, Args&&... args)
        {
            UString line(header(splice_pid, event_id));
            line.format(format, std::forward<ArgMixIn>(args)...);
            return line;
        }

        // Compute time between current packet and event. Return false if not possible to compute.
        bool timeToEvent(cn::milliseconds& tte, uint64_t event_pts, const SpliceContext& ctx);

        // Implementation of interfaces.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
        virtual void handlePMT(const PMT&, PID) override;
    };
}
