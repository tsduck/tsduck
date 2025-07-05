//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Compute the duration of a stream, based on PCR or input timestamps.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsPCRAnalyzer.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Compute the duration of a stream, based on PCR or input timestamps.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DurationAnalyzer
    {
        TS_NOCOPY(DurationAnalyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report messages when switching sources.
        //! @param [in] severity Severity of the messages when switching sources.
        //!
        DurationAnalyzer(Report& report = NULLREP, int severity = Severity::Verbose);

        //!
        //! Reset all collected information.
        //!
        void reset();

        //!
        //! Select the prefered method for duration evaluation.
        //! @param [in] prefer_timestamps If true, use input timestamps when possible.
        //! Fallback to PCR analysis if the input timestamps are not valid or not monotonic.
        //! If false, always use PCR analysis, ignore timestamps.
        //!
        void useInputTimestamps(bool prefer_timestamps) { _prefer_timestamps = prefer_timestamps; }

        //!
        //! The following method feeds the analyzer with a TS packet.
        //! @param [in] pkt A new transport stream packet.
        //! @param [in] mdata Associated metadata, including the input timestamp.
        //!
        void feedPacket(const TSPacket& pkt, const TSPacketMetadata& mdata);

        //!
        //! Get the estimated playout duration in PCR units.
        //! @return The estimated playout duration in PCR units.
        //!
        PCR duration() const { return _total_duration; }

    private:
        Report&       _report;
        int           _severity;
        bool          _prefer_timestamps = false;  // Use timestamps when possible.
        bool          _use_timestamps = false;     // Currently use timestamps
        PCR           _total_duration {};          // Total duration since last reset.
        PCR           _switch_duration {};         // Value of _total_duration at last source switch.
        PCR           _switch_timestamp {};        // PCR or input timestamp at last source switch.
        TimeSource    _last_source = TimeSource::UNDEFINED;
        PCRAnalyzer   _pcr_analyzer {1, 1};
    };
}
