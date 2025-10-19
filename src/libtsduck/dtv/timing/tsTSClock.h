//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Compute a clock, based real time, TS time, PCR or input timestamps.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSClockArgs.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsPCRAnalyzer.h"
#include "tsSectionDemux.h"

namespace ts {
    //!
    //! Compute a clock, based real time, TS time, PCR or input timestamps.
    //! Depending on parameters, the clock can be:
    //! - Real time (system clock)
    //! - Time from the first TDT, TOT ot ATSC STT.
    //! - Explicit start time from parameters.
    //! The progression of the clock is based on TS packets in the last two cases.
    //! By default, the clock is based on real UTC time and TS packets are unused.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSClock : private TableHandlerInterface, SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TSClock);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context.
        //! A reference is kept inside this object.
        //! @param [in] severity Severity of the messages when switching sources.
        //!
        TSClock(DuckContext& duck, int severity = Severity::Verbose);

        //!
        //! Reset all collected information.
        //! @param [in] args Clock parameters.
        //!
        void reset(const TSClockArgs& args);

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //! @param [in] mdata Associated metadata, including the input timestamp.
        //!
        void feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata);

        //!
        //! Check if the clock is valid.
        //! The clock is invalid when based on the TS clock, before the first TDT, TOT or STT.
        //! @return True if the clock is valid, false otherwise.
        //!
        bool isValid() const { return _first_time != Time::Epoch; }

        //!
        //! Get the clock in UTC time of the first packet.
        //! @return The clock of the first packet in UTC time or Time::Epoch if the clock is not yet valid.
        //!
        Time initialClockUTC() const { return _first_time; }

        //!
        //! Get the clock of the first packet, in UTC or local time, depending on TSClockArgs.
        //! @return The clock of the first packet or Time::Epoch if the clock is not yet valid.
        //!
        Time initialClock() const;

        //!
        //! Get the current clock in UTC time.
        //! @return The computed clock in UTC or Time::Epoch if the clock is not yet valid.
        //!
        Time clockUTC() const;

        //!
        //! Get the current clock, in UTC or local time, depending on TSClockArgs.
        //! @return The computed clock or Time::Epoch if the clock is not yet valid.
        //!
        Time clock() const;

        //!
        //! Get the estimated playout duration since first packets in milliseconds.
        //! The returned duration depends on the TSClockArgs. If real time is used, this is the number
        //! of actual milliseconds since the first packet. Otherwise, this is the same as durationPCR(),
        //! converted in milliseconds.
        //! @return The estimated playout duration in milliseconds.
        //!
        cn::milliseconds durationMS() const;

        //!
        //! Get the estimated playout duration since first packets in PCR units.
        //! The returned duration is always based on the timestamps from the stream, never on real time.
        //! @return The estimated playout duration in PCR units.
        //!
        PCR durationPCR() const { return _total_duration; }

    private:
        DuckContext& _duck;
        int          _severity;
        TSClockArgs  _args {};
        Time         _first_time {};              // UTC time of first packet.
        bool         _use_timestamps = false;     // Currently use timestamps
        PCR          _total_duration {};          // Total duration since last reset.
        PCR          _switch_duration {};         // Value of _total_duration at last source switch.
        PCR          _switch_timestamp {};        // PCR or input timestamp at last source switch.
        TimeSource   _last_source = TimeSource::UNDEFINED;
        PCRAnalyzer  _pcr_analyzer {1, 1};
        SectionDemux _demux {_duck, this, this};

        // Implementation of interfaces to handle UTC time from the stream.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
        void handleUTC(const Time& time);
    };
}
