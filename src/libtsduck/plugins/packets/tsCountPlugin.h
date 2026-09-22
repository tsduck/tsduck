//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to count TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to count TS packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL CountPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(CountPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // This structure is used at each --interval.
        struct IntervalReport
        {
            Time          start {};             // Interval start time in UTC.
            PacketCounter counted_packets = 0;  // Total counted packets.
            PacketCounter total_packets = 0;    // Total TS packets.

            // Constructor.
            IntervalReport() = default;
        };

        // Command  line options:
        UString        _tag {};                  // Message tag
        bool           _negate = false;          // Negate filter (exclude selected packets)
        PIDSet         _pids {};                 // PID values to filter
        bool           _brief_report = false;    // Display biref report, values but not comments
        bool           _report_all = false;      // Report packet index and PID of each packet
        bool           _report_summary = false;  // Report summary
        bool           _report_total = false;    // Report total of all PIDs
        PacketCounter  _report_interval = 0;     // If non-zero, report timestamp at this packet interval
        fs::path       _outfile_name {};         // Output file name.

        // Working data:
        std::ofstream  _outfile {};              // User-specified output file
        IntervalReport _last_report {};          // Last report content
        PacketCounter  _counters[PID_MAX] {};    // Packet counter per PID

        // Report a line
        template <class... Args>
        void report(const UChar* fmt, Args&&... args)
        {
            if (_outfile.is_open()) {
                _outfile << UString::Format(fmt, std::forward<ArgMixIn>(args)...) << std::endl;
            }
            else {
                info(fmt, std::forward<ArgMixIn>(args)...);
            }
        }
    };
}
