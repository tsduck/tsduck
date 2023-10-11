//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  EIT sections repetition profile.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSection.h"
#include "tsTime.h"
#include "tsPSI.h"
#include "tsEIT.h"

namespace ts {
    //!
    //! List of EIT sections repetition profiles.
    //! @ingroup mpeg
    //!
    //! The EIT sections shall be repeated according to the type of EIT and the type of network.
    //!
    //! The enumeration values are sorted in order of importance. For instance, it is more important
    //! to reliably broadcast EIT p/f actual than others, EIT p/f than schedule, etc.
    //!
    //! EIT schedule are divided into two periods:
    //! - The "prime" period extends over the next few days. The repetition rate of those EIT's
    //!   is typically longer than EIT present/following but still reasonably fast. The duration
    //!   in days of the prime period depends on the type of network.
    //! - The "later" period includes all events after the prime period. The repetition rate of
    //!   those EIT's is typically longer that in the prime period.
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
    enum class EITProfile {
        // See ts::EITGenerator::provideSection before changing the values below.
        PF_ACTUAL          = 0,   //!< EIT present/following actual.
        PF_OTHER           = 1,   //!< EIT present/following other.
        SCHED_ACTUAL_PRIME = 2,   //!< EIT schedule actual in the "prime" period.
        SCHED_OTHER_PRIME  = 3,   //!< EIT schedule other in the "prime" period.
        SCHED_ACTUAL_LATER = 4,   //!< EIT schedule actual after the "prime" period.
        SCHED_OTHER_LATER  = 5,   //!< EIT schedule other after the "prime" period.
    };

    //!
    //! EIT sections repetition profile.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL EITRepetitionProfile
    {
    public:
        //!
        //! Number of EIT sections repetition profiles.
        //!
        static constexpr size_t PROFILE_COUNT = size_t(EITProfile::SCHED_OTHER_LATER) + 1;

        //!
        //! Duration in days of the "prime" period for EIT schedule.
        //! EIT schedule for events in the prime period (i.e. the next few days)
        //! are repeated more often than for later events.
        //!
        size_t prime_days {EIT::TOTAL_DAYS};

        //!
        //! Cycle time in seconds of each EIT sections repetition profile.
        //! The array is indexed by EITProfile.
        //!
        std::array <size_t, PROFILE_COUNT> cycle_seconds {};

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

        //!
        //! Constructor.
        //! @param [in] days Number of prime days.
        //! @param [in] cycles Up to 6 cycles in seconds. Missing values are replaced by the
        //! last value in the list. If the list is empty, all cycles are 10 seconds long.
        //!
        EITRepetitionProfile(size_t days = 1, std::initializer_list<size_t> cycles = std::initializer_list<size_t>());

        //!
        //! Compute the starting date of the "later" period.
        //! @param [in] now Current UTC time.
        //! @return Starting UTC time of the "later" period.
        //!
        Time laterPeriod(const Time& now) const;

        //!
        //! Compute the first EIT table id in the "later" period.
        //! If a table id is equal this one, this is not sufficient to determine if the corresponding
        //! section is in the "prime" or "later" period. Check the section number as well.
        //! @param [in] actual True for EIT actual, false for EIT other.
        //! @return First EIT table id in the "later" period.
        //! @see laterSectionNumber()
        //!
        TID laterTableId(bool actual) const;

        //!
        //! Compute the first section number in the "later" period.
        //! @return First section number in the "later" period.
        //!
        uint8_t laterSectionNumber() const;

        //!
        //! Determine the repetition profile of an EIT section.
        //! @param [in] section A valid EIT section.
        //! @return The repetition profile of @a section.
        //! Undefined if the section is not a valid EIT.
        //!
        EITProfile sectionToProfile(const Section& section);

        //!
        //! Determine the repetition cycle of an EIT section.
        //! @param [in] section A valid EIT section.
        //! @return The repetition cycle in seconds of @a section.
        //! Undefined if the section is not a valid EIT.
        //!
        size_t repetitionSeconds(const Section& section);
    };
}
