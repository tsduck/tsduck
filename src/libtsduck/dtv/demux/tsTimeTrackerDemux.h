//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            TimeTracker(uint64_t scale = PTS_DTS_SCALE) : _scale(scale) {}
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
            uint64_t _scale = PTS_DTS_SCALE; //!< Scale offset after wrapping up at max value.
            uint64_t _first = INVALID_PCR;   //!< First value seen on PID (INVALID_PCR if none found).
            uint64_t _last = INVALID_PCR;    //!< Last value seen on PID (INVALID_PCR if none found).
            uint64_t _offset = 0;            //!< Accumulated offsets after wrapping up at max value once or more.
        };

        typedef std::map<PID, TimeTracker> PIDContextMap;

        PID           _pcrPID = PID_NULL;    //!< First detected PID with PCR's.
        TimeTracker   _pcrTime {PCR_SCALE};  //!< PCR time tracker on _pcrPID.
        PIDContextMap _pids {};              //!< PTS time tracker per demuxed PID.
    };
}
