//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEITRepetitionProfile.h"


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
    return EIT::SegmentToTableId(actual, prime_days * EIT::SEGMENTS_PER_DAY);
}

uint8_t ts::EITRepetitionProfile::laterSectionNumber() const
{
    return EIT::SegmentToSection(prime_days * EIT::SEGMENTS_PER_DAY);
}


//----------------------------------------------------------------------------
// Determine the repetition profile of an EIT section.
//----------------------------------------------------------------------------

ts::EITProfile ts::EITRepetitionProfile::sectionToProfile(const Section& section)
{
    const TID tid = section.tableId();
    const bool actual = EIT::IsActual(tid);
    if (EIT::IsPresentFollowing(tid)) {
        return actual ? EITProfile::PF_ACTUAL : EITProfile::PF_OTHER;
    }
    const TID later_tid = laterTableId(actual);
    if (tid < later_tid || (tid == later_tid && section.sectionNumber() < laterSectionNumber())) {
        return actual ? EITProfile::SCHED_ACTUAL_PRIME : EITProfile::SCHED_OTHER_PRIME;
    }
    else {
        return actual ? EITProfile::SCHED_ACTUAL_LATER : EITProfile::SCHED_OTHER_LATER;
    }
}

size_t ts::EITRepetitionProfile::repetitionSeconds(const Section& section)
{
    return cycle_seconds[size_t(sectionToProfile(section))];
}
