//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to report various statistics on PID's and labels.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTSSpeedMetrics.h"
#include "tsSingleDataStatistics.h"
#include "tsFileNameGenerator.h"
#include "tsFileUtils.h"

namespace ts {
    //!
    //! Plugin to report various statistics on PID's and labels.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL StatsPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(StatsPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each category of packets (PID or lable) is described by a structure like this.
        // The map is indexed by PID or label.
        class Context;
        using ContextPtr = std::shared_ptr<Context>;
        using ContextMap = std::map<size_t, ContextPtr>;

        // Command line options.
        bool             _track_pids = true;       // Track PID's, not labels.
        bool             _log = false;             // Report statistics through the logger, not files.
        bool             _csv = false;             // Use CSV format for statistics.
        bool             _header = false;          // Display header lines.
        bool             _multiple_output = false; // Don't rewrite output files with --interval.
        UString          _csv_separator {DEFAULT_CSV_SEPARATOR}; // Separator character in CSV lines.
        fs::path         _output_name {};          // Output file name.
        cn::nanoseconds  _output_interval {};      // Recreate output at this time interval.
        PIDSet           _pids {};                 // List of PID's to track.
        TSPacketLabelSet _labels {};               // List of labels to track.

        // Working data.
        std::ofstream     _output_stream {};  // Output file stream.
        std::ostream*     _output = nullptr;  // Point to actual output stream.
        ContextMap        _ctx_map {};        // Description of all tracked categories of packets.
        TSSpeedMetrics    _metrics {};        // Timing to synchronize next output files.
        cn::nanoseconds   _next_report {};   // Next time to create next output.
        FileNameGenerator _name_gen {};       // Generate multiple output file names.

        // Get or create the description of a tracked PID or label.
        ContextPtr getContext(size_t index);

        // Open, close and create statistics report.
        bool openOutput();
        void closeOutput();
        bool produceReport();

        // Description of a tracked category of packet (PID or label).
        class Context
        {
        public:
            Context() = default;         // Constructor.
            uint64_t total_pkt = 0;      // Total number of packets in that category.
            uint64_t last_ts_index = 0;  // Index in TS of last packet of the category.
            SingleDataStatistics<uint64_t> ipkt {}; // Inter-packet distance statistics.

            // Add packet data to the context.
            void addPacketData(PacketCounter, const TSPacket&);
        };
    };
}
