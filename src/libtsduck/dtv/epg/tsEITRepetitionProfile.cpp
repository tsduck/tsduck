//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEITRepetitionProfile.h"


//----------------------------------------------------------------------------
// Predefined EIT repetition profiles.
// Thread-safe init-safe static data patterns.
//----------------------------------------------------------------------------

const ts::EITRepetitionProfile& ts::EITRepetitionProfile::SatelliteCable()
{
    static const EITRepetitionProfile data{
        cn::days(8),          // prime_days
        {                     // cycle_seconds
            cn::seconds(2),   // PF_ACTUAL
            cn::seconds(10),  // PF_OTHER
            cn::seconds(10),  // SCHED_ACTUAL_PRIME
            cn::seconds(10),  // SCHED_OTHER_PRIME
            cn::seconds(30),  // SCHED_ACTUAL_LATER
            cn::seconds(30)   // SCHED_OTHER_LATER
        }
    };
    return data;
}

const ts::EITRepetitionProfile& ts::EITRepetitionProfile::Terrestrial()
{
    static const EITRepetitionProfile data{
        cn::days(1),          // prime_days
        {                     // cycle_seconds
            cn::seconds(2),   // PF_ACTUAL
            cn::seconds(20),  // PF_OTHER
            cn::seconds(10),  // SCHED_ACTUAL_PRIME
            cn::seconds(60),  // SCHED_OTHER_PRIME
            cn::seconds(30),  // SCHED_ACTUAL_LATER
            cn::seconds(300)  // SCHED_OTHER_LATER
        }
    };
    return data;
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::EITRepetitionProfile::EITRepetitionProfile(cn::days days, std::initializer_list<cn::seconds> cycles) :
    prime_days(std::min(days, EIT::TOTAL_DAYS))
{
    auto it = cycles.begin();
    cn::seconds previous = cn::seconds(10);  // default cycle
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
    return now.thisDay() + std::min(prime_days, EIT::TOTAL_DAYS);
}

ts::TID ts::EITRepetitionProfile::laterTableId(bool actual) const
{
    return EIT::SegmentToTableId(actual, size_t(prime_days.count() * EIT::SEGMENTS_PER_DAY));
}

uint8_t ts::EITRepetitionProfile::laterSectionNumber() const
{
    return EIT::SegmentToSection(size_t(prime_days.count() * EIT::SEGMENTS_PER_DAY));
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

cn::seconds ts::EITRepetitionProfile::repetitionSeconds(const Section& section)
{
    return cycle_seconds[size_t(sectionToProfile(section))];
}
