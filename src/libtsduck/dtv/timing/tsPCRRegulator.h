//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        void setBurstPacketCount(PacketCounter count) { _opt_burst = count; }

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
        //! Default minimum wait interval.
        //!
        static constexpr cn::milliseconds DEFAULT_MIN_WAIT = cn::milliseconds(50);

        //!
        //! Set the minimum wait interval.
        //! @param [in] d Minimum duration interval.
        //!
        template <class Rep, class Period>
        void setMinimimWait(const cn::duration<Rep,Period>& d);

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
        Report*          _report = nullptr;
        int              _log_level = Severity::Info;
        PID              _user_pid = PID_NULL;      // User-specified reference PID.
        PID              _pid = PID_NULL;           // Current reference PID.
        PacketCounter    _opt_burst = 0;            // Number of packets to burst at a time
        PacketCounter    _burst_pkt_cnt = 0;        // Number of packets in current burst
        cn::microseconds _wait_min {};              // Minimum delay between two waits
        bool             _started = false;          // First PCR found, regulation started.
        uint64_t         _pcr_first = INVALID_PCR;  // First PCR value.
        uint64_t         _pcr_last = INVALID_PCR;   // Last PCR value.
        uint64_t         _pcr_offset = 0;           // Offset to add to PCR value, accumulate all PCR wrap-down sequences.
        monotonic_time   _clock_first {};           // System time at first PCR.
        monotonic_time   _clock_last {};            // System time at last wait
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Set the minimum wait interval.
template <class Rep, class Period>
void ts::PCRRegulator::setMinimimWait(const cn::duration<Rep,Period>& d)
{
    using duration = cn::duration<Rep,Period>;
    if (d != _wait_min && d > duration::zero()) {
        cn::microseconds precision = cn::milliseconds(2);
        SetTimersPrecision(precision);
        _wait_min = std::max(cn::duration_cast<cn::microseconds>(d), precision);
        _report->log(_log_level, u"minimum wait: %s, using %s", {precision, _wait_min});
    }
}

#endif // DOXYGEN
