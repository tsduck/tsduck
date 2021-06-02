//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  EIT repetition profile.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! EIT sections repetition profile.
    //! @ingroup mpeg
    //!
    //! The EIT sections shall be repeated according to the type of EIT and the type of network.
    //!
    //! Standard EIT repetition rates
    //! -----------------------------
    //!
    //! | %EIT section type        | Sat/cable | Terrestrial
    //! | ------------------------ | --------- | -----------
    //! | EIT p/f actual           | 2 sec     | 2 sec
    //! | EIT p/f other            | 10 sec    | 20 sec
    //! | EIT sched prime days     | 8 days    | 1 day
    //! | EIT sched actual (prime) | 10 sec    | 10 sec
    //! | EIT sched other (prime)  | 10 sec    | 60 sec
    //! | EIT sched actual (later) | 30 sec    | 30 sec
    //! | EIT sched other (later)  | 30 sec    | 300 sec
    //!
    class TSDUCKDLL EITRepetitionProfile
    {
    public:
        //!
        //! Cycle time in seconds of EIT present/following actual.
        //!
        size_t eit_pf_actual_seconds;
        //!
        //! Cycle time in seconds of EIT present/following other.
        //!
        size_t eit_pf_actual_other;
        //!
        //! Duration in days of the "prime" period for EIT schedule.
        //! EIT schedule for events in the prime period (i.e. the next few days)
        //! are repeated more often than for later events.
        //!
        size_t eit_sched_prime_days;
        //!
        //! Cycle time in seconds of EIT schedule actual in the "prime" period.
        //!
        size_t eit_sched_actual_prime_seconds;
        //!
        //! Cycle time in seconds of EIT schedule other in the "prime" period.
        //!
        size_t eit_sched_other_prime_seconds;
        //!
        //! Cycle time in seconds of EIT schedule actual after the "prime" period.
        //!
        size_t eit_sched_actual_later_seconds;
        //!
        //! Cycle time in seconds of EIT schedule other after the "prime" period.
        //!
        size_t eit_sched_other_later_seconds;

        //!
        //! Standard EIT repetition profile for satellite and cable networks.
        //! @see ETSI TS 101 211, section 4.1.4
        //!
        static const EITRepetitionProfile SatelliteCable;

        //!
        //! Standard EIT repetition profile for terrestrial networks.
        //! @see ETSI TS 101 211, section 4.1.4
        //!
        static const EITRepetitionProfile Terrestrial;
    };
}
