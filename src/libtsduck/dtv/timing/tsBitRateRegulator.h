//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Regulate execution speed based on a bitrate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsReport.h"
#include "tsMonotonic.h"

namespace ts {
    //!
    //! Regulate execution speed based on a bitrate.
    //! @ingroup mpeg
    //! @see PCRRegulator
    //!
    class TSDUCKDLL BitRateRegulator
    {
         TS_NOCOPY(BitRateRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] log_level Severity level for information messages.
        //!
        BitRateRegulator(Report* report = nullptr, int log_level = Severity::Verbose);

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
        //! Set a fixed bitrate for regulation, ignore current bitrate.
        //! @param [in] bitrate Fixed bitrate to use. When zero, use current bitrate.
        //!
        void setFixedBitRate(BitRate bitrate) { _opt_bitrate = bitrate; }

        //!
        //! Start regulation, initialize all timers.
        //!
        void start();

        //!
        //! Regulate the flow, to be called at each packet.
        //! Suspend the process when necessary.
        //! @param [in] current_bitrate Current bitrate. Ignored if a fixed bitrate was set.
        //! @param [out] flush Set to true if all previously processed and buffered packets should be flushed.
        //! @param [out] bitrate_changed Set to true if the bitrate has changed.
        //!
        void regulate(const BitRate& current_bitrate, bool& flush, bool& bitrate_changed);

        //!
        //! Regulate the flow, to be called at each packet.
        //! Suspend the process when necessary.
        //! This version is suitable for fixed bitrate.
        //!
        void regulate();

    private:
        // We accumulate the amount of passed bits over the last few seconds to evaluate if we have
        // to pass more or less packets. This is used to compensate for the fact that we pass entire
        // packets only and not the exact number of bits per second. Since we have to reevaluate this
        // periodically, we keep the last two periods to avoid restarting from nothing at the end of
        // a period. We use signed values for bits to allow credit.
        class Period
        {
        public:
            Monotonic start {};
            int64_t   bits = 0;
        };

        // Private members.
        Report*       _report = nullptr;
        int           _log_level = Severity::Info;
        bool          _starting = false;    // Starting, no packet processed so far
        bool          _regulated = false;   // Currently regulated at known bitrate
        PacketCounter _opt_burst = 0;       // Number of packets to burst at a time
        BitRate       _opt_bitrate = 0;     // Bitrate option, zero means use input
        BitRate       _cur_bitrate = 0;     // Current bitrate
        NanoSecond    _burst_min = 0;       // Minimum delay between two bursts
        NanoSecond    _burst_duration = 0;  // Delay between two bursts
        Monotonic     _burst_end {};        // End of current burst
        Period        _periods[2] {};       // Last two measurement periods, accumulating packets
        NanoSecond    _period_duration = NanoSecPerSec; // Duration of a period of packet measurement, default: 1 second
        size_t        _cur_period = 0;      // Current period index, 0 or 1

        // Current and other period.
        Period& currentPeriod() { return _periods[_cur_period & 1]; }
        Period& otherPeriod() { return _periods[(_cur_period & 1) ^ 1]; }

        // Compute burst duration (_burst_duration and _burst_pkt_max), based on
        // required packets/burst (command line option) and current bitrate.
        void handleNewBitrate();

        // Process one packet in a regulated burst. Wait at end of burst.
        void regulatePacket(bool& flush);
    };
}
