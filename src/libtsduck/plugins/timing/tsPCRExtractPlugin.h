//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to extract PCR's from TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSpliceInformationTable.h"

namespace ts {
    //!
    //! Plugin to extract PCR's from TS packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCRExtractPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(PCRExtractPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID,PIDContextPtr>;

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext;
        using SpliceContextPtr = std::shared_ptr<SpliceContext>;
        using SpliceContextMap = std::map<PID,SpliceContextPtr>;

        // Command line options:
        fs::path         _output_name {};         // Output file name (empty means stderr)
        PIDSet           _pids {};                // List of PID's to analyze
        UString          _separator {};           // Field separator
        bool             _all_pids = false;       // Analyze all PID's
        bool             _noheader = false;       // Suppress header
        bool             _good_pts_only = false;  // Keep "good" PTS only
        bool             _get_pcr = false;        // Get PCR
        bool             _get_opcr = false;       // Get OPCR
        bool             _get_pts = false;        // Get PTS
        bool             _get_dts = false;        // Get DTS
        bool             _csv_format = false;     // Output in CSV format
        bool             _log_format = false;     // Output in log format
        bool             _evaluate_pcr = false;   // Evaluate PCR offset for packets with PTS/DTS without PCR
        bool             _scte35 = false;         // Detect SCTE 35 PTS values
        bool             _input_time = false;     // Add an input timestamp of the TS packet

        // Working data:
        std::ofstream    _output_stream {};       // Output stream file
        std::ostream*    _output = nullptr;       // Reference to actual output stream file
        PIDContextMap    _stats {};               // Per-PID statistics
        SpliceContextMap _splices {};             // Per-PID splice information
        SectionDemux     _demux {duck, this};     // Section demux for service and SCTE 35 analysis

        // Description of one type of data in a PID: PCR, OPCR, PTS, DTS.
        template <TimeSource T>
        class PIDData
        {
            TS_NOCOPY(PIDData);
        public:
            PIDData() = default;                      // Constructor.
            PacketCounter count = 0;                  // Number of data of this type in this PID.
            uint64_t      first_value = INVALID_PCR;  // First data value of this type in this PID.
            uint64_t      last_value = INVALID_PCR;   // First data value of this type in this PID.
            PacketCounter last_packet = 0;            // Packet index in TS of last value.
        };

        // Description of one PID carrying PCR, PTS or DTS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID p) : pid(p) {}                  // Constructor.
            const PID                 pid;                 // PID value.
            PacketCounter             packet_count = 0;    // Number of packets in this PID.
            PID                       pcr_pid = PID_NULL;  // PID containing PCR in the same service.
            uint64_t                  last_good_pts = PTSTraits::INVALID;
            PIDData<TimeSource::PCR>  pcr {};
            PIDData<TimeSource::OPCR> opcr {};
            PIDData<TimeSource::PTS>  pts {};
            PIDData<TimeSource::DTS>  dts {};
        };

        // Description of one PID carrying SCTE 35 splice information.
        class SpliceContext
        {
            TS_NOCOPY(SpliceContext);
        public:
            SpliceContext() = default;
            PIDSet components {};  // All service components for this slice info PID.
        };

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific types of tables.
        void processPAT(const PAT&);
        void processPMT(const PMT&);
        void processSpliceCommand(PID pid, SpliceInformationTable&);

        // Get info context for a PID.
        PIDContextPtr getPIDContext(PID);
        SpliceContextPtr getSpliceContext(PID);

        // Report a value in csv or log format.
        void csvHeader();
        template <TimeSource T> void processValue(PIDContext&, PIDData<T> PIDContext::*, uint64_t value, uint64_t pcr, bool report_it, const TSPacketMetadata& mdata);
    };
}
