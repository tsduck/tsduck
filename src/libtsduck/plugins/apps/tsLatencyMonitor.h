//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Amos Cheung
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
//!
//!  @file
//!  Implementation of the latency monitor (command tslatencymonitor).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsLatencyMonitorArgs.h"
#include "tsMutex.h"
#include "tsTime.h"
#include <memory>

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
            uint64_t timestamp;
        };
        typedef std::list<TimingData> TimingDataList;

        struct InputData
        {

            std::shared_ptr<tslatencymonitor::InputExecutor> inputExecutor;
            TimingDataList timingDataList;
        };
        typedef std::vector<InputData> InputDataVector;

        Report&            _report;
        LatencyMonitorArgs _args;
        InputDataVector    _inputs;
        Mutex              _mutex;            // Global mutex, protect access to all subsequent fields.
        double             _max_latency;      // Maximum latency between two inputs
        Time               _last_output_time; // Timestamp to record last output time
        std::ofstream      _output_stream;    // Output stream file
        std::ostream*      _output_file;      // Reference to actual output stream file

        // Generate csv header
        void csvHeader();

        // Calculate delta of two PCRs
        void calculatePCRDelta(InputDataVector& inputs);
    };
}
