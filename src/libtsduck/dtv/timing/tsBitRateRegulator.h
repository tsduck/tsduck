//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Regulate execution speed based on a bitrate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractRegulator.h"

namespace ts {
    //!
    //! Regulate execution speed based on a bitrate.
    //! @ingroup libtsduck mpeg
    //! @see PCRRegulator
    //! @see TimeStampRegulator
    //!
    class TSDUCKDLL BitRateRegulator: public AbstractRegulator
    {
        TS_NOBUILD_NOCOPY(BitRateRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit BitRateRegulator(Report* report, int log_level = Severity::Verbose) : AbstractRegulator(report, log_level) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit BitRateRegulator(ReporterBase* delegate, int log_level = Severity::Verbose) : AbstractRegulator(delegate, log_level) {}

        //!
        //! Destructor.
        //!
        virtual ~BitRateRegulator() override;

        // Inherited methods.
        virtual bool reset() override;
        virtual void regulate(TSPacket& pkt, TSPacketMetadata& mdata, const BitRate& current_bitrate) override;

    private:
        // We accumulate the amount of passed bits over the last few seconds to evaluate if we have
        // to pass more or less packets. This is used to compensate for the fact that we pass entire
        // packets only and not the exact number of bits per second. Since we have to reevaluate this
        // periodically, we keep the last two periods to avoid restarting from nothing at the end of
        // a period. We use signed values for bits to allow credit.
        class Period
        {
        public:
            monotonic_time start {};
            int64_t        bits = 0;
        };

        // Private members.
        bool            _starting = false;    // Starting, no packet processed so far
        bool            _regulated = false;   // Currently regulated at known bitrate
        BitRate         _cur_bitrate = 0;     // Current bitrate
        cn::nanoseconds _burst_min {0};       // Minimum delay between two bursts
        cn::nanoseconds _burst_duration {0};  // Delay between two bursts
        monotonic_time  _burst_end {};        // End of current burst
        Period          _periods[2] {};       // Last two measurement periods, accumulating packets
        cn::nanoseconds _period_duration {cn::seconds(1)}; // Duration of a period of packet measurement, default: 1 second
        size_t          _cur_period = 0;      // Current period index, 0 or 1

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
