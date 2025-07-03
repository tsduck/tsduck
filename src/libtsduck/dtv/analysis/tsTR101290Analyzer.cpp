//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Remaining rules to implement:
//
// [[1.5]] PMT_error: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on the PID which is referred to in the PAT.
// [[1.5]] PMT_error: Scrambling_control_field is not 00 for all PIDs containing sections with table_id 0x02 (i.e. a PMT).
// [[1.5.a]] PMT_error_2: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on each program_map_PID which is referred to in the PAT.
// [[1.5.a]] PMT_error_2: Scrambling_control_field is not 00 for all packets containing information of sections with table_id 0x02 (i.e. a PMT) on each program_map_PID which is referred to in the PAT.
// [[1.6]] PID_error: Referred PID does not occur for a user specified period.
// [[2.1]] Transport_error: Transport_error_indicator in the TS-Header is set to "1".
// [[2.2]] CRC_error: CRC error occurred in CAT, PAT, PMT, NIT, EIT, BAT, SDT or TOT table.
// [[2.3]] PCR_error: PCR discontinuity of more than 100 ms occurring without specific indication.
// [[2.3]] PCR_error: Time interval between two consecutive PCR values more than 100 ms.
// [[2.3a]] PCR_repetition_error: Time interval between two consecutive PCR values more than 100 ms.
// [[2.3b]] PCR_discontinuity_indicator_error: The difference between two consecutive PCR values (PCRi+1 – PCRi) is outside the range of 0...100 ms without the discontinuity_indicator set.
// [[2.4]] PCR_accuracy_error: PCR accuracy of selected programme is not within +/- 500 ns.
// [[2.5]] PTS_error: PTS repetition period more than 700 ms.
// [[2.6]] CAT_error: Packets with transport_scrambling_control not 00 present, but no section with table_id = 0x01 (i.e. a CAT) present.
// [[2.6]] CAT_error: Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001.
//
//@@@ TODO: Add 3.x rules
//
//----------------------------------------------------------------------------

#include "tsTR101290Analyzer.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TR101290Analyzer::TR101290Analyzer(DuckContext& duck) :
    _duck(duck)
{
    _demux.setInvalidSectionHandler(this);
    reset();
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
    _demux.setPIDFilter(AllPIDs());
    _continuity.reset();
    _pids.clear();
    _xtids.clear();
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
// Update error counters at most once per TS packet.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::CounterFlags::update(Counters& counters)
{
    // Equivalence between CounterFlags booleans and Counters integers (using pointers to members).
    static const std::vector<std::pair<bool CounterFlags::*, size_t Counters::*>> _counter_pairs {
        {&CounterFlags::PAT_error,   &Counters::PAT_error},
        {&CounterFlags::PAT_error_2, &Counters::PAT_error_2},
    };

    for (const auto& it : _counter_pairs) {
        if (this->*it.first) {
            (counters.*it.second)++;
        }
    }
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::feedPacket(const PCR& timestamp, const TSPacket& pkt)
{
    const PID pid = pkt.getPID();
    auto& pidctx(_pids[pid]);

    // Must be set first. During execution of the various handlers, synchronously called from here,
    // _last_pcr < _current_pcr. Upon return from feedPacket(), _last_pcr == _current_pcr.
    _current_pcr = timestamp;

    // Clear flags which indicate errors during the processing of this packet.
    _counters_flags.clear();

    // Check sync bytes errors.
    if (pkt.hasValidSync()) {
        _bad_sync_count = 0;
    }
    else {
        // [[1.2]] Sync_byte_error: Sync_byte not equal 0x47.
        _counters.Sync_byte_error++;
        // Count TS_sync_loss exactly once per sequence of sync byte errors.
        if (++_bad_sync_count == _bad_sync_max) {
            // [[1.1]] TS_sync_loss: Loss of synchronization with consideration of hysteresis parameters
            _counters.TS_sync_loss++;
        }
    }

    // Check continuity errors.
    if (!_continuity.feedPacket(pkt)) {
        // [[1.4]] Continuity_count_error: Incorrect packet order, a packet occurs more than twice, lost packet.
        _counters.Continuity_count_error++;
    }

    // Check PID's that shouldn't be scrambled.
    if (pkt.isScrambled()) {
        if (pid == PID_PAT) {
            // [[1.3]] PAT_error: Scrambling_control_field is not 00 for PID 0x0000
            // [[1.3.a]] PAT_error_2: Scrambling_control_field is not 00 for PID 0x0000.
            _counters_flags.PAT_error = _counters_flags.PAT_error_2 = true;
        }
    }

    // Check max interval between packets of a PID.
    if (pidctx._last_pcr >= PCR::zero() && (_current_pcr - pidctx._last_pcr) > _max_pat_interval) {
        // [[1.3]] PAT_error: PID 0x0000 does not occur at least every 0,5 s
        _counters_flags.PAT_error = true;
    }

    // Check PSI/SI validity.
    _demux.feedPacket(pkt);

    //@@@ TODO

    // Increment each error at most once per packet.
    _counters_flags.update(_counters);

    // Must be set last.
    pidctx._last_pcr = timestamp;
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
    const TID tid = section.tableId();
    const PID pid = section.sourcePID();

    if (pid == PID_PAT && tid != TID_PAT) {
        // [[1.3]] PAT_error: a PID 0x0000 does not contain a table_id 0x00 (i.e. a PAT).
        // [[1.3.a]] PAT_error_2: Section with table_id other than 0x00 found on PID 0x0000.
        _counters_flags.PAT_error = _counters_flags.PAT_error_2 = true;
    }

    switch (section.tableId()) {
        case TID_PAT: {
            auto& ctx(_xtids[section.xtid()]);
            if (pid != PID_PAT ||
                (ctx._last_pcr >= PCR::zero() && (_current_pcr - ctx._last_pcr) > _max_pat_interval))
            {
                // [[new]] a PAT section is present on PID other than 0x0000.
                // [[1.3.a]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
                _counters_flags.PAT_error_2 = true;
            }
            ctx._last_pcr = _current_pcr;
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when an invalid section is detected.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status)
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
