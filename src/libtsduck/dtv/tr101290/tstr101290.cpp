//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstr101290.h"


//----------------------------------------------------------------------------
// Get the description of one ETSI TR 101 290 error counter.
//----------------------------------------------------------------------------

const ts::tr101290::CounterDescription& ts::tr101290::GetCounterDescription(ErrorCounter counter)
{
    static const CounterDescription invalid {INFO_SEVERITY, u""};
    return int(counter) >= 0 && counter < COUNTER_COUNT ? GetCounterDescriptions()[counter] : invalid;
}


//----------------------------------------------------------------------------
// Get the description of all ETSI TR 101 290 error counters.
//----------------------------------------------------------------------------

const std::array<ts::tr101290::CounterDescription, ts::tr101290::COUNTER_COUNT>& ts::tr101290::GetCounterDescriptions()
{
    static const std::array<CounterDescription, COUNTER_COUNT> data { CounterDescription
        {1, u"TS_sync_loss"},
        {1, u"Sync_byte_error"},
        {1, u"PAT_error"},
        {1, u"PAT_error_2"},
        {1, u"Continuity_count_error"},
        {1, u"PMT_error"},
        {1, u"pmt_error_2"},
        {1, u"PID_error"},

        {2, u"Transport_error"},
        {2, u"CRC_error"},
        {2, u"CRC_error_2"},
        {2, u"PCR_error"},
        {2, u"PCR_repetition_error"},
        {2, u"PCR_discontinuity_indicator_error"},
        {2, u"PCR_accuracy_error"},
        {2, u"PTS_error"},
        {2, u"CAT_error"},

        {3, u"NIT_error"},
        {3, u"NIT_actual_error"},
        {3, u"NIT_other_error"},
        {3, u"SI_repetition_error"},
        {3, u"SI_PID_error"},
        {3, u"Buffer_error"},
        {3, u"Unreferenced_PID"},
        {3, u"SDT_error"},
        {3, u"SDT_actual_error"},
        {3, u"SDT_other_error"},
        {3, u"EIT_error"},
        {3, u"EIT_actual_error"},
        {3, u"EIT_other_error"},
        {3, u"EIT_PF_error"},
        {3, u"RST_error"},
        {3, u"TDT_error"},
        {3, u"Empty_buffer_error"},
        {3, u"Data_delay_error"},

        {4, u"packet_count"},
    };

    // Verify that all members are initialized.
    // The right order cannot be enforced, just take care when adding new values...
    assert(data[data.size()-1].severity == INFO_SEVERITY);

    return data;
}


//----------------------------------------------------------------------------
// Get the total number of errors in a Counters.
//----------------------------------------------------------------------------

size_t ts::tr101290::Counters::errorCount() const
{
    // Warning: carefully select the relevant counters because an error can be included in several counters.
    // The result won't be precisely exact, but still better than the sum of all of them.
    return
        // (*this)[TS_sync_loss] +
           (*this)[Sync_byte_error] +
        // (*this)[PAT_error] +
           (*this)[PAT_error_2] +
           (*this)[Continuity_count_error] +
        // (*this)[PMT_error] +
           (*this)[PMT_error_2] +
           (*this)[PID_error] +
           (*this)[Transport_error] +
           (*this)[CRC_error] +
           (*this)[CRC_error_2] +
        // (*this)[PCR_error] +
           (*this)[PCR_repetition_error] +
           (*this)[PCR_discontinuity_indicator_error] +
           (*this)[PCR_accuracy_error] +
           (*this)[PTS_error] +
           (*this)[CAT_error] +
        // (*this)[NIT_error] +
           (*this)[NIT_actual_error] +
           (*this)[NIT_other_error] +
           (*this)[SI_repetition_error] +
           (*this)[SI_PID_error] +
           (*this)[Buffer_error] +
           (*this)[Unreferenced_PID] +
        // (*this)[SDT_error] +
           (*this)[SDT_actual_error] +
           (*this)[SDT_other_error] +
        // (*this)[EIT_error] +
           (*this)[EIT_actual_error] +
           (*this)[EIT_other_error] +
           (*this)[EIT_PF_error] +
           (*this)[RST_error] +
           (*this)[TDT_error] +
           (*this)[Empty_buffer_error] +
           (*this)[Data_delay_error];
}
