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
//!  Time stamp management per PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"

namespace ts {
    //!
    //! A demux which tracks time stamps per PID.
    //! @ingroup mpeg
    //!
    //! Typically used as a superclass by other demux which need time tracking
    //! in addition to other demux activities.
    //!
    class TimeTrackerDemux: public AbstractDemux
    {
        TS_NOBUILD_NOCOPY(TimeTrackerDemux);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef AbstractDemux SuperClass;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] pid_filter The initial set of PID's to demux.
        //!
        TimeTrackerDemux(DuckContext& duck, const PIDSet& pid_filter = NoPID);

        //!
        //! Get the number of milliseconds measured on a PID.
        //! @param [in] pid The PID to check.
        //! @return The number of milliseconds of the content of the specified @a pid,
        //! since the beginning of the stream, based on the PTS of this PID. If no PTS
        //! were found on this PID, use PCR's from another PID. Return zero is no
        //! timing information was found.
        //!
        MilliSecond pidDuration(PID pid) const;

        // Inherited methods
        virtual void feedPacket(const TSPacket& pkt) override;

    protected:
        // Inherited methods
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;

    private:
        //!
        //! This class tracks time stamps on one PID, either PCR, PTS or DTS.
        //!
        class TimeTracker
        {
        public:
            //!
            //! Default constructor.
            //! @param [in] scale Scale offset after wrapping up at max value. Default is appropriate for PTS/DTS.
            //!
            TimeTracker(uint64_t scale = PTS_DTS_SCALE);
            //!
            //! Check if values were set in the object and if we can collect info.
            //! @returb True if we can collect valid info.
            //!
            bool isValid() const { return _first < _scale; }
            //!
            //! Reset all value, forget collected time stamps.
            //!
            void reset();
            //!
            //! Set a new collected time stamp value.
            //! @param [in] value New collected time stamp value.
            //!
            void set(uint64_t value);
            //!
            //! Get the total duration, in time stamp units, between the first and last value.
            //! @return Total duration in time stamp units or zero if invalid.
            //!
            uint64_t duration() const;

        private:
            uint64_t _scale;  //!< Scale offset after wrapping up at max value.
            uint64_t _first;  //!< First value seen on PID (INVALID_PCR if none found).
            uint64_t _last;   //!< Last value seen on PID (INVALID_PCR if none found).
            uint64_t _offset; //!< Accumulated offsets after wrapping up at max value once or more.
        };

        typedef std::map<PID, TimeTracker> PIDContextMap;

        PID           _pcrPID;    //!< First detected PID with PCR's.
        TimeTracker   _pcrTime;   //!< PCR time tracker on _pcrPID.
        PIDContextMap _pids;      //!< PTS time tracker per demuxed PID.
    };
}
