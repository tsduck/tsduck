//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsMPEG.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Time stamp management per PID.
    //!
    //! Return a current time reference in millisecond for a given PID.
    //! The first time reference found is zero.
    //! Use the PTS from the PID if some are found.
    //! Otherwise, use the global PCR from the TS (the first PID with PCR is used as reference).
    //!
    class TimeStamper
    {
    public:
        //!
        //! Constructor.
        //! @param [in] referencePID PID for which time stamps are returned.
        //!
        TimeStamper(PID referencePID = PID_NULL);

        //!
        //! Reset the stamper, back to constructor state.
        //!
        void reset();

        //!
        //! The following method feeds the time stamper with a TS packet.
        //! @param [in] pkt A TS packet. If the PID is not yet established, the PID of this packet is used.
        //! Afterwards, packets from other PID's are ignored.
        //!
        void feedPacket(const TSPacket& pkt);

        //!
        //! Get the last timestamp in milliseconds, starting with zero.
        //! @return The last timestamp in milliseconds.
        //!
        MilliSecond lastTimeStamp();

        //!
        //! Get the reference PID.
        //! @return The reference PID or PID_NULL if unknown.
        //!
        PID getPID() const { return _pid; }

    private:
        //!
        //! Our source of time reference.
        //!
        enum TimeSource {PTS, PCR, UNDEFINED};

        //!
        //! Process a new clock value in millisecond.
        //! @param [in] clock New clock value in millisecond.
        //!
        void processClock(qint64 clock);

        PID         _pid;            //!< Reference PID for timestamps.
        TimeSource  _source;         //!< Where do we get the time reference from.
        MilliSecond _lastTimeStamp;  //!< Last known timestamp.
        MilliSecond _previousClock;  //!< Previous time clock value.
        MilliSecond _delta;          //!< Adjustment value between time clock and time stamps.
    };
}
