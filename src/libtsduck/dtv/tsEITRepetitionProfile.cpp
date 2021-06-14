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

#include "tsEITRepetitionProfile.h"
#include "tsEIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Predefined EIT repetition profiles.
//----------------------------------------------------------------------------

const ts::EITRepetitionProfile ts::EITRepetitionProfile::SatelliteCable{
    8,       // prime_days
    {        // cycle_seconds
        2,   // PF_ACTUAL
        10,  // PF_OTHER
        10,  // SCHED_ACTUAL_PRIME
        10,  // SCHED_OTHER_PRIME
        30,  // SCHED_ACTUAL_LATER
        30   // SCHED_OTHER_LATER
    }
};

const ts::EITRepetitionProfile ts::EITRepetitionProfile::Terrestrial{
    1,       // prime_days
    {        // cycle_seconds
        2,   // PF_ACTUAL
        20,  // PF_OTHER
        10,  // SCHED_ACTUAL_PRIME
        60,  // SCHED_OTHER_PRIME
        30,  // SCHED_ACTUAL_LATER
        300  // SCHED_OTHER_LATER
    }
};


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITRepetitionProfile::EITRepetitionProfile(size_t days, std::initializer_list<size_t> cycles) :
    prime_days(std::min(days, EIT::TOTAL_DAYS)),
    cycle_seconds()
{
    auto it = cycles.begin();
    size_t previous = 10;  // default cycle
    for (size_t index = 0; index < cycle_seconds.size(); ++index) {
        if (it != cycles.end()) {
            previous = *it++;
        }
        cycle_seconds[index] = previous;
    }
}


//----------------------------------------------------------------------------
// Starting date and first table id of the "later" period.
//----------------------------------------------------------------------------

ts::Time ts::EITRepetitionProfile::laterPeriod(const Time& now) const
{
    return now.thisDay() + std::min(prime_days, EIT::TOTAL_DAYS) * MilliSecPerDay;
}

ts::TID ts::EITRepetitionProfile::laterTableId(bool actual) const
{
    return EIT::SegmentToTableId(actual, prime_days / EIT::SEGMENTS_PER_DAY);
}
