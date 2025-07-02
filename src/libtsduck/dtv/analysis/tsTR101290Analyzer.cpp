//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTR101290Analyzer.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TR101290Analyzer::TR101290Analyzer(DuckContext& duck) :
    _duck(duck)
{
}


//----------------------------------------------------------------------------
// Reset the analyzer.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::reset()
{
    _current_pkt = 0;
    _bad_sync_count = 0;
    _current_pcr = _last_pcr = PCR(-1);
    _counters.clear();
    _demux.reset();
    _continuity.reset();
}


//----------------------------------------------------------------------------
// Get the total number of errors in a Counters instance.
//----------------------------------------------------------------------------

size_t ts::TR101290Analyzer::Counters::errorCount() const
{
    // Warning: carefully select the relevant counters because an error can be included in several counters.
    return Sync_byte_error + PAT_error_2 + Continuity_count_error + PMT_error_2 + PID_error + Transport_error +
           CRC_error + PCR_error + PTS_error + CAT_error + NIT_error + SI_repetition_error + Buffer_error +
           Unreferenced_PID + SDT_error + EIT_error + RST_error + TDT_error + Empty_buffer_error + Data_delay_error;
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::feedPacket(const PCR& timestamp, const TSPacket& pkt)
{
    // Must be set first. During execution of the various handlers, synchronously called from here,
    // _last_pcr < _current_pcr. Upon return from feedPacket(), _last_pcr == _current_pcr.
    _current_pcr = timestamp;

    // Check sync bytes errors.
    if (pkt.hasValidSync()) {
        _bad_sync_count = 0;
    }
    else {
        _counters.Sync_byte_error++;
        // Count TS_sync_loss exactly once per sequence of sync byte errors.
        if (++_bad_sync_count == _bad_sync_max) {
            _counters.TS_sync_loss++;
        }
    }

    // Check continuity errors.
    if (!_continuity.feedPacket(pkt)) {
        _counters.Continuity_count_error++;
    }

    // Check PSI/SI validity.
    _demux.feedPacket(pkt);

    //@@@ TODO

    // Must be set last.
    _last_pcr = timestamp;
    _current_pkt++;
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete table is available.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    //@@@ TODO
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete section is available.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::handleSection(SectionDemux& demux, const Section& section)
{
    //@@@ TODO
}


//----------------------------------------------------------------------------
// Get the error counters since start or the last getCountersRestart().
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::getCountersRestart(Counters& counters)
{
    getCounters(counters);
    _counters.clear();
}

void ts::TR101290Analyzer::getCounters(Counters& counters)
{
    counters = _counters;

    //@@@ TODO: add all timeouts which are triggered on next event.
}
