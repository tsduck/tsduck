//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Regulate execution speed based on PCR's in one reference PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsTSPacket.h"
#include "tsMonotonic.h"

namespace ts {
    //!
    //! Regulate execution speed based on PCR's in one reference PID.
    //! @ingroup mpeg
    //! @see BitRateRegulator
    //!
    class TSDUCKDLL PCRRegulator
    {
        TS_NOCOPY(PCRRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] log_level Severity level for information messages.
        //!
        PCRRegulator(Report* report = nullptr, int log_level = Severity::Verbose);

        //!
        //! Set a new report.
        //! @param [in,out] report Where to report errors.
        //! @param [in] log_level Severity level for information messages.
        //!
        void setReport(Report* report = nullptr, int log_level = Severity::Verbose);

        //!
        //! Set the number of packets to burst at a time.
        //! @param [in] count Number of packets to burst at a time.
        //!
        void setBurstPacketCount(PacketCounter count)
        {
            _opt_burst = count;
        }

        //!
        //! Set the PCR reference PID.
        //! @param [in] pid Reference PID. If PID_NULL, use the first PID containing PCR's.
        //!
        void setReferencePID(PID pid);

        //!
        //! Get the current PCR reference PID.
        //! @return Current reference PID or PID_NULL if none was set or found.
        //!
        PID getReferencePID() const { return _pid; }

        //!
        //! Default minimum wait interval in nano-seconds.
        //!
        static const NanoSecond DEFAULT_MIN_WAIT_NS = 50 * NanoSecPerMilliSec;

        //!
        //! Set the minimum wait interval.
        //! @param [in] ns Minimum duration interval in nano-seconds.
        //!
        void setMinimimWait(NanoSecond ns = DEFAULT_MIN_WAIT_NS);

        //!
        //! Re-initialize state.
        //!
        void reset();

        //!
        //! Regulate the flow, to be called at each packet.
        //! Suspend the process when necessary.
        //! @param [in] pkt TS packet from the stream.
        //! @return True if all previously processed and buffered packets should be flushed.
        //!
        bool regulate(const TSPacket& pkt);

    private:
        Report*       _report;
        int           _log_level;
        PID           _user_pid;        // User-specified reference PID.
        PID           _pid;             // Current reference PID.
        PacketCounter _opt_burst;       // Number of packets to burst at a time
        PacketCounter _burst_pkt_cnt;   // Number of packets in current burst
        NanoSecond    _wait_min;        // Minimum delay between two waits (ns)
        bool          _started;         // First PCR found, regulation started.
        uint64_t      _pcr_first;       // First PCR value.
        uint64_t      _pcr_last;        // Last PCR value.
        uint64_t      _pcr_offset;      // Offset to add to PCR value, accumulate all PCR wrap-down sequences.
        Monotonic     _clock_first;     // System time at first PCR.
        Monotonic     _clock_last;      // System time at last wait
    };
}
