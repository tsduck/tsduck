//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definitions for ETSI TR 101 290.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Definitions for ETSI TR 101 290.
    //! @ingroup mpeg
    //! @see ETSI TR 101 290 V1.4.1 (2020-06): Digital Video Broadcasting(DVB); Measurement guidelines for DVB systems.
    //!
    //! The document ETSI TR 101 290 defines measurement criteria for MPEG/DVB transport streams.
    //! It is used by TV operators to monitor the stability of their network. It also used as a
    //! commonly accepted criteria of quality for broadcast networks.
    //!
    namespace tr101290 {
        //!
        //! Pseudo-severity for informational (non-error) data.
        //! ETSI TR 101 290 defines three types of error severity, from 1 (most severe, section 5.2.1)
        //! to 3 (less severe, section 5.2.3). We add a 4th level of severity for informational counters.
        //!
        constexpr int INFO_SEVERITY = 4;

        //!
        //! List of error counters as defined in ETSI TR 101 290.
        //! The names are the same as the counter names in ETSI TR 101 290.
        //!
        enum ErrorCounter {
            // Severity 1: Section 5.2.1 - First priority: necessary for de-codability (basic monitoring)
            TS_sync_loss = 0,        //!< No 1.1 (maybe unreliable, depending on input plugin)
            Sync_byte_error,         //!< No 1.2 (maybe unreliable, depending on input plugin)
            PAT_error,               //!< No 1.3
            PAT_error_2,             //!< No 1.3.a
            Continuity_count_error,  //!< No 1.4
            PMT_error,               //!< No 1.5
            PMT_error_2,             //!< No 1.5.a
            PID_error,               //!< No 1.6

            // Severity 2: Section 5.2.2 - Second priority: recommended for continuous or periodic monitoring
            Transport_error,         //!< No 2.1
            CRC_error,               //!< No 2.2
            CRC_error_2,             //!< CRC error in all other cases than CRC_error.
            PCR_error,               //!< No 2.3
            PCR_repetition_error,    //!< No 2.3.a
            PCR_discontinuity_indicator_error, //!< No 2.3b
            PCR_accuracy_error,      //!< No 2.4 (TODO)
            PTS_error,               //!< No 2.5
            CAT_error,               //!< No 2.6

            // Severity 3: Section 5.2.3 - Third priority: application dependant monitoring
            NIT_error,               //!< No 3.1
            NIT_actual_error,        //!< No 3.1.a
            NIT_other_error,         //!< No 3.1.b
            SI_repetition_error,     //!< No 3.2
            SI_PID_error,            //!< No 3.2/2U (added)
            Buffer_error,            //!< No 3.3 (unimplemented)
            Unreferenced_PID,        //!< No 3.4
            SDT_error,               //!< No 3.5
            SDT_actual_error,        //!< No 3.5.a
            SDT_other_error,         //!< No 3.5.b
            EIT_error,               //!< No 3.6
            EIT_actual_error,        //!< No 3.6.a
            EIT_other_error,         //!< No 3.6.b
            EIT_PF_error,            //!< No 3.6.c
            RST_error,               //!< No 3.7
            TDT_error,               //!< No 3.8
            Empty_buffer_error,      //!< No 3.9 (unimplemented)
            Data_delay_error,        //!< No 3.10 (unimplemented)

            // Severity 4: Informational, not in ETSI TR 101 290.
            packet_count,            //!< Number of TS packets during measurement interval.

            COUNTER_COUNT            //!< Number defined ETSI TR 101 290 counters.
        };

        //!
        //! Description of one TR 101 290 counter.
        //!
        class TSDUCKDLL CounterDescription
        {
        public:
            int     severity;  //!< Severity: 1, 2, 3, same as in ETSI TR 101 290.
            UString name;      //!< Counter name, same as in ETSI TR 101 290.
        };

        //!
        //! Get the description of one ETSI TR 101 290 error counter.
        //! @param [in] counter The counter to search.
        //! @return A constant reference to the counter's description.
        //!
        TSDUCKDLL const CounterDescription& GetCounterDescription(ErrorCounter counter);

        //!
        //! Get the description of all ETSI TR 101 290 error counters.
        //! @return A constant reference to an array of descriptions, indexed by ErrorCounter value.
        //!
        TSDUCKDLL const std::array<CounterDescription, COUNTER_COUNT>& GetCounterDescriptions();

        //!
        //! Array of error counters as defined in ETSI TR 101 290.
        //!
        class TSDUCKDLL Counters: public std::array<size_t, COUNTER_COUNT>
        {
        public:
            //!
            //! Default contructor.
            //!
            Counters() { clear(); }
            //!
            //! Reset all counters.
            //!
            void clear() { fill(0); }
            //!
            //! Get the total number of errors.
            //! @return The total number of errors. This can be less than the sum of all fields because
            //! an error can be included in several counters.
            //!
            size_t errorCount() const;
        };

        //!
        //! A map of error counters, as defined in ETSI TR 101 290, indexed by PID.
        //!
        using CountersByPID = std::map<PID, Counters>;

        //!
        //! Maximum interval between two PAT.
        //!
        constexpr cn::milliseconds MAX_PAT_INTERVAL = cn::milliseconds(500);
        //!
        //! Maximum interval between two PMT.
        //!
        constexpr cn::milliseconds MAX_PMT_INTERVAL = cn::milliseconds(500);
        //!
        //! Minimum interval between two RST.
        //!
        constexpr cn::milliseconds MIN_RST_INTERVAL = cn::milliseconds(25);
        //!
        //! Maximum interval between two NIT sections, regardless of type.
        //!
        constexpr cn::seconds MAX_NIT_INTERVAL = cn::seconds(10);
        //!
        //! Minimum interval between two NIT Actual.
        //!
        constexpr cn::milliseconds MIN_NIT_ACTUAL_INTERVAL = cn::milliseconds(25);
        //!
        //! Maximum interval between two NIT Actual.
        //!
        constexpr cn::seconds MAX_NIT_ACTUAL_INTERVAL = cn::seconds(10);
        //!
        //! Maximum interval between two NIT Other.
        //!
        constexpr cn::seconds MAX_NIT_OTHER_INTERVAL = cn::seconds(10);
        //!
        //! Minimum interval between two SDT Actual.
        //!
        constexpr cn::milliseconds MIN_SDT_ACTUAL_INTERVAL = cn::milliseconds(25);
        //!
        //! Maximum interval between two SDT Actual.
        //!
        constexpr cn::seconds MAX_SDT_ACTUAL_INTERVAL = cn::seconds(2);
        //!
        //! Maximum interval between two SDT Other.
        //!
        constexpr cn::seconds MAX_SDT_OTHER_INTERVAL = cn::seconds(10);
        //!
        //! Maximum interval between two BAT.
        //!
        constexpr cn::seconds MAX_BAT_INTERVAL = cn::seconds(10);
        //!
        //! Minimum interval between two EIT p/f Actual.
        //!
        constexpr cn::milliseconds MIN_EIT_PF_ACTUAL_INTERVAL = cn::milliseconds(25);
        //!
        //! Maximum interval between two EIT p/f Actual.
        //!
        constexpr cn::seconds MAX_EIT_PF_ACTUAL_INTERVAL = cn::seconds(2);
        //!
        //! Maximum interval between two EIT p/f Other.
        //!
        constexpr cn::seconds MAX_EIT_PF_OTHER_INTERVAL = cn::seconds(10);
        //!
        //! Minimum interval between two TDT.
        //!
        constexpr cn::milliseconds MIN_TDT_INTERVAL = cn::milliseconds(25);
        //!
        //! Maximum interval between two TDT.
        //!
        constexpr cn::seconds MAX_TDT_INTERVAL = cn::seconds(30);
        //!
        //! Maximum interval between two TDT.
        //!
        constexpr cn::seconds MAX_TOT_INTERVAL = cn::seconds(30);
        //!
        //! Maximum interval between two PTS in the same PID.
        //!
        constexpr cn::milliseconds MAX_PTS_INTERVAL = cn::milliseconds(700);
        //!
        //! Maximum interval between two PCR in the same PID.
        //!
        constexpr cn::milliseconds MAX_PCR_INTERVAL = cn::milliseconds(100);
        //!
        //! Maximum difference of value between two PCR in the same PID.
        //!
        constexpr cn::milliseconds MAX_PCR_DIFFERENCE = cn::milliseconds(100);
        //!
        //! Maximum interval between the first packet of a PID and the time it is referenced.
        //! When jumping into a transport stream, we get audio, video, etc. packets possibly
        //! before the corresponding PMT. The PID is initially unreferenced but we need to
        //! find the reference (in the PMT) within that interval.
        //!
        constexpr cn::milliseconds MAX_PID_REFERENCE_INTERVAL = cn::milliseconds(500);
        //!
        //! Default maximum packet interval in "user PID's" as defined by PID_error.
        //!
        constexpr cn::seconds DEFAULT_MAX_PID_INTERVAL = cn::seconds(5);
        //!
        //! Default number of consecutive invalid TS sync bytes before declaring TS sync loss.
        //!
        constexpr size_t DEFAULT_TS_SYNC_LOST = 5;
    }
}
