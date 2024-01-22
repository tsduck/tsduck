//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of the latency monitor (command tslatencymonitor).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsLatencyMonitorArgs.h"
#include "tsTime.h"

namespace ts {

    // Used in private part.
    namespace tslatencymonitor {
        class InputExecutor;
    }

    //!
    //! Implementation of the latency monitor
    //! This class is used by the @a tslatencymonitor utility.
    //! @ingroup plugin
    //!
    class TSDUCKDLL LatencyMonitor
    {
        TS_NOBUILD_NOCOPY(LatencyMonitor);
    public:
        //!
        //! Constructor.
        //! The complete input comparing session is performed in this constructor.
        //! The constructor returns only when the PCR comparator session terminates or fails tp start.
        //! @param [in] args Arguments and options.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        LatencyMonitor(const LatencyMonitorArgs& args, Report& report);

        //!
        //! Start the PCR comparator session.
        //! @return True on success, false on failure to start.
        //!
        bool start();

        //!
        //! Called by an input plugin when it received input packets.
        //! @param [in] pkt Income TS packet.
        //! @param [in] metadata Metadata of income TS packet.
        //! @param [in] count TS packet count.
        //! @param [in] pluginIndex Index of the input plugin.
        //!
        void processPacket(const TSPacketVector& pkt, const TSPacketMetadataVector& metadata, size_t count, size_t pluginIndex);

    private:
        struct TimingData
        {
            uint64_t pcr;
            ts::pcr_units timestamp;
        };
        using TimingDataList = std::list<TimingData>;

        struct InputData
        {
            std::shared_ptr<tslatencymonitor::InputExecutor> inputExecutor;
            TimingDataList timingDataList;
        };
        using InputDataVector = std::vector<InputData>;

        Report&              _report;
        LatencyMonitorArgs   _args {};
        InputDataVector      _inputs {};
        std::recursive_mutex _mutex {};              // Global mutex, protect access to all subsequent fields.
        double               _max_latency = 0;       // Maximum latency between two inputs
        Time                 _last_output_time {};   // Timestamp to record last output time
        std::ofstream        _output_stream {};      // Output stream file
        std::ostream*        _output_file = nullptr; // Reference to actual output stream file

        // Generate csv header
        void csvHeader();

        // Calculate delta of two PCRs
        void calculatePCRDelta(InputDataVector& inputs);
    };
}
