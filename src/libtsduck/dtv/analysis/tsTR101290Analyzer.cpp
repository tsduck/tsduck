//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// ETSI TR 101 290 rules to implement
// ----------------------------------
// Notes:
// - In [[x.y]], "x.y" is the rule number in ETSI TR 101 290 section 5.2.
// - In [[x.y/z]], "z" is a sequence number when the same rule includes distinct checks.
// - In [[x.y/Uz]], "U" means unreferenced, a new rule which seems necessary.
// - The prefix "(x)" means that the rule is not yet implemented.
//
//     [[1.1]] TS_sync_loss: Loss of synchronization with consideration of hysteresis parameters
//     [[1.2]] Sync_byte_error: Sync_byte not equal 0x47.
//     [[1.3/1]] PAT_error: PID 0x0000 does not occur at least every 0,5 s
//     [[1.3/2]] PAT_error: a PID 0x0000 does not contain a table_id 0x00 (i.e. a PAT).
//     [[1.3/3]] PAT_error: Scrambling_control_field is not 00 for PID 0x0000
//     [[1.3.a/1]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
//     [[1.3.a/2]] PAT_error_2: Section with table_id other than 0x00 found on PID 0x0000.
//     [[1.3.a/3]] PAT_error_2: Scrambling_control_field is not 00 for PID 0x0000.
//     [[1.3.a/U4]] PAT_error_2: A PAT section is present on PID other than 0x0000.
//     [[1.3.a/U5]] PAT_error_2: A PAT table is syntactically incorrect.
//     [[1.4]] Continuity_count_error: Incorrect packet order, a packet occurs more than twice, lost packet.
// (x) [[1.5/1]] PMT_error: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on the PID which is referred to in the PAT.
// (x) [[1.5/2]] PMT_error: Scrambling_control_field is not 00 for all PIDs containing sections with table_id 0x02 (i.e. a PMT).
// (x) [[1.5.a/1]] PMT_error_2: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on each program_map_PID which is referred to in the PAT.
// (x) [[1.5.a/2]] PMT_error_2: Scrambling_control_field is not 00 for all packets containing information of sections with table_id 0x02 (i.e. a PMT) on each program_map_PID which is referred to in the PAT.
//     [[1.5.a/U3]] PMT_error_2: A PMT table is syntactically incorrect.
// (x) [[1.6]] PID_error: Referred PID does not occur for a user specified period.
//     [[2.1]] Transport_error: Transport_error_indicator in the TS-Header is set to "1".
//     [[2.2/1]] CRC_error: CRC error occurred in CAT, PAT, PMT, NIT, EIT, BAT, SDT or TOT table.
//     [[2.2/U2]] CRC_error_2: CRC error occurred in other table id than specified in CRC_error.
// (x) [[2.3/1]] PCR_error: PCR discontinuity of more than 100 ms occurring without specific indication.
// (x) [[2.3/2]] PCR_error: Time interval between two consecutive PCR values more than 100 ms.
// (x) [[2.3.a]] PCR_repetition_error: Time interval between two consecutive PCR values more than 100 ms.
// (x) [[2.3.b]] PCR_discontinuity_indicator_error: The difference between two consecutive PCR values (PCRi+1 – PCRi) is outside the range of 0...100 ms without the discontinuity_indicator set.
// (x) [[2.4]] PCR_accuracy_error: PCR accuracy of selected programme is not within +/- 500 ns.
// (x) [[2.5]] PTS_error: PTS repetition period more than 700 ms.
//     [[2.6/1]] CAT_error: Packets with transport_scrambling_control not 00 present, but no section with table_id = 0x01 (i.e. a CAT) present.
//     [[2.6/2]] CAT_error: Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001.
//     [[2.6/U3]] CAT_error: A CAT section is present on PID other than 0x0001.
//     [[2.6/U4]] CAT_error: A CAT table is syntactically incorrect.
// (x) [[3.1/1]] NIT_error: Section with table_id other than 0x40 or 0x41 or 0x72 (i. e. not an NIT or ST) found on PID 0x0010.
// (x) [[3.1/2]] NIT_error: No section with table_id 0x40 or 0x41 (i.e. an NIT) in PID value 0x0010 for more than 10 s.
// (x) [[3.1.a/1]] NIT_actual_error: Section with table_id other than 0x40 or 0x41 or 0x72 (i.e. not an NIT or ST) found on PID 0x0010.
// (x) [[3.1.a/2]] NIT_actual_error: No section with table_id 0x40 (i.e. an NIT_actual) in PID value 0x0010 for more than 10 s.
// (x) [[3.1.a/3]] NIT_actual_error: Any two sections with table_id = 0x40 (NIT_actual) occur on PID 0x0010 within a specified value (25 ms or lower).
// (x) [[3.1.b]] NIT_other_error: Interval between sections with the same section_number and table_id = 0x41 (NIT_other) on PID 0x0010 longer than a specified value (10 s or higher).
// (x) [[3.2]] SI_repetition_error: Repetition rate of SI tables outside of specified limits.
// (x) [[3.3/1]] Buffer_error: TB_buffering_error: overflow of transport buffer (TBn).
// (x) [[3.3/2]] Buffer_error: TBsys_buffering_error: overflow of transport buffer for system information (Tbsys).
// (x) [[3.3/3]] Buffer_error: MB_buffering_error: overflow of multiplexing buffer (MBn) or if the vbv_delay method is used: underflow of multiplexing buffer (Mbn).
// (x) [[3.3/4]] Buffer_error: EB_buffering_error: overflow of elementary stream buffer (EBn) or if the leak method is used: underflow of elementary stream buffer (EBn) though low_delay_flag and DSM_trick_mode_flag are set to 0 else (vbv_delay method) underflow of elementary stream buffer (EBn).
// (x) [[3.3/5]] Buffer_error: B_buffering_error: overflow or underflow of main buffer (Bn).
// (x) [[3.3/6]] Buffer_error: Bsys_buffering_error: overflow of PSI input buffer (Bsys).
//     [[3.4]] Unreferenced_PID: PID (other than PAT, CAT, CAT_PIDs, PMT_PIDs, NIT_PID, SDT_PID, TDT_PID, EIT_PID, RST_PID, reserved_for_future_use PIDs, or PIDs user defined as private data streams) not referred to by a PMT within 0,5 s.
//     [[3.4.a]] Unreferenced_PID: PID (other than PMT_PIDs, PIDs with numbers between 0x00 and 0x1F or PIDs user defined as private data streams) not referred to by a PMT or a CAT within 0,5 s.
// (x) [[3.5/1]] SDT_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
// (x) [[3.5/2]] SDT_error: Sections with table_ids other than 0x42, 0x46, 0x4A or 0x72 found on PID 0x0011.
// (x) [[3.5.a/1]] SDT_actual_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
// (x) [[3.5.a/2]] SDT_actual_error: Sections with table_ids other than 0x42, 0x46, 0x4A or 0x72 found on PID 0x0011.
// (x) [[3.5.a/3]] SDT_actual_error: Any two sections with table_id = 0x42 (SDT_actual) occur on PID 0x0011 within a specified value (25 ms or lower).
// (x) [[3.5.b]] SDT_other_error: Interval between sections with the same section_number and table_id = 0x46 (SDT, other TS) on PID 0x0011 longer than a specified value (10s or higher).
// (x) [[3.6/1]] EIT_error: Sections with table_id = 0x4E (EIT-P/F, actual TS) not present on PID 0x0012 for more than 2 s.
// (x) [[3.6/2]] EIT_error: Sections with table_ids other than in the range 0x4E - 0x6F or 0x72 found on PID 0x0012.
// (x) [[3.6.a/1]] EIT_actual_error: Section '0' with table_id = 0x4E (EIT-P, actual TS) not present on PID 0x0012 for more than 2 s.
// (x) [[3.6.a/2]] EIT_actual_error: Section '1' with table_id = 0x4E (EIT-F, actual TS) not present on PID 0x0012 for more than 2 s.
// (x) [[3.6.a/3]] EIT_actual_error: Sections with table_ids other than in the range 0x4E - 0x6F or 0x72 found on PID 0x0012.
// (x) [[3.6.a/4]] EIT_actual_error: Any two sections with table_id = 0x4E (EIT-P/F, actual TS) occur on PID 0x0012 within a specified value (25 ms or lower).
// (x) [[3.6.b/1]] EIT_other_error: Interval between sections '0' with table_id = 0x4F (EIT-P, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
// (x) [[3.6.b/2]] EIT_other_error: Interval between sections '1' with table_id = 0x4F (EIT-F, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
// (x) [[3.6.c]] EIT_PF_error: If either section ('0' or '1') of each EIT P/F sub table is present both should exist. Otherwise EIT_PF_error should be indicated.
//     [[3.7/1]] RST_error: Sections with table_id other than 0x71 or 0x72 found on PID 0x0013.
//     [[3.7/2]] RST_error: Any two sections with table_id = 0x71 (RST) occur on PID 0x0013 within a specified value (25 ms or lower).
//     [[3.7/U3]] RST_error: A RST section is present on PID other than 0x0013.
// (x) [[3.8/1]] TDT_error: Sections with table_id = 0x70 (TDT) not present on PID 0x0014 for more than 30 s.
// (x) [[3.8/2]] TDT_error: Sections with table_id other than 0x70, 0x72 (ST) or 0x73 (TOT) found on PID 0x0014.
// (x) [[3.8/3]] TDT_error: Any two sections with table_id = 0x70 (TDT) occur on PID 0x0014 within a specified value (25 ms or lower).
// (x) [[3.9]] Empty_buffer_error: Transport buffer (TBn) not empty at least once per second or transport buffer for system information (TBsys) not empty at least once per second or if the leak method is used multiplexing buffer (MBn) not empty at least once per second.
// (x) [[3.10/1]] Data_delay_error: Delay of data (except still picture video data) through the TSTD buffers superior to 1 second.
// (x) [[3.10/2]] Data_delay_error: Delay of still picture video data through the TSTD buffers superior to 60 s.
//
//----------------------------------------------------------------------------

#include "tsTR101290Analyzer.h"
#include "tsAlgorithm.h"
#include "tsBinaryTable.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsCADescriptor.h"


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
    return Sync_byte_error + PAT_error_2 + Continuity_count_error + PMT_error_2 + PID_error +
           Transport_error + CRC_error + CRC_error_2 + PCR_error + PTS_error + CAT_error + NIT_error +
           SI_repetition_error + Buffer_error + Unreferenced_PID + SDT_error + EIT_error + RST_error +
           TDT_error + Empty_buffer_error + Data_delay_error;
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
        {&CounterFlags::PMT_error,   &Counters::PMT_error},
        {&CounterFlags::PMT_error_2, &Counters::PMT_error_2},
        {&CounterFlags::CAT_error,   &Counters::CAT_error},
        {&CounterFlags::CRC_error,   &Counters::CRC_error},
        {&CounterFlags::CRC_error_2, &Counters::CRC_error_2},
        {&CounterFlags::RST_error,   &Counters::RST_error},
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
    // Must be set first. During execution of the various handlers, synchronously called from here,
    // _last_pcr < _current_pcr. Upon return from feedPacket(), _last_pcr == _current_pcr.
    _current_pcr = timestamp;

    // Clear flags which indicate errors during the processing of this packet.
    _counters_flags.clear();

    // Get/initialize PID context for that packet.
    const PID pid = pkt.getPID();
    auto& pidctx(_pids[pid]);
    if (pidctx.first_pcr < PCR::zero()) {
        // This is the first packet in that PID.
        pidctx.first_pcr = _current_pcr;
    }

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

    // [[2.1]] Transport_error: Transport_error_indicator in the TS-Header is set to "1".
    if (pkt.getTEI()) {
        _counters.Transport_error++;
    }

    // [[1.4]] Continuity_count_error: Incorrect packet order, a packet occurs more than twice, lost packet.
    if (!_continuity.feedPacket(pkt)) {
        _counters.Continuity_count_error++;
    }

    // Check PID's that shouldn't be scrambled.
    if (pkt.isScrambled()) {
        if (pid == PID_PAT) {
            // [[1.3/3]] PAT_error: Scrambling_control_field is not 00 for PID 0x0000
            // [[1.3.a/3]] PAT_error_2: Scrambling_control_field is not 00 for PID 0x0000.
            _counters_flags.PAT_error = _counters_flags.PAT_error_2 = true;
        }
        if (pid == PID_CAT) {
            // [[2.6/1]] CAT_error: Packets with transport_scrambling_control not 00 present, but no section with table_id = 0x01 (i.e. a CAT) present.
            _counters_flags.CAT_error = true;
        }
    }

    // Check max interval between packets of a PID.
    if (pidctx.last_pcr >= PCR::zero() && (_current_pcr - pidctx.last_pcr) > _max_pat_interval) {
        // [[1.3/1]] PAT_error: PID 0x0000 does not occur at least every 0,5 s
        _counters_flags.PAT_error = true;
    }

    // Check PSI/SI validity.
    _demux.feedPacket(pkt);

    //@@@ TODO

    // Increment each error at most once per packet.
    _counters_flags.update(_counters);

    // Must be set last.
    pidctx.last_pcr = timestamp;
    _last_pcr = timestamp;
    _current_pkt++;
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete table is available.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();

    switch (table.tableId()) {
        case TID_PAT: {
            if (pid == PID_PAT) {
                const PAT pat(_duck, table);
                if (pat.isValid()) {
                    std::set<uint16_t> services;
                    // Declare all PMT PID's as part of their service.
                    for (const auto& it : pat.pmts) {
                        auto& ctx(_pids[it.second]);
                        services.insert(it.first);
                        ctx.services.insert(it.first);
                        ctx.type = PIDClass::PSI;
                    }
                    // Remove all references to undefined services in all PID's.
                    for (auto& it1 : _pids) {
                        for (auto it2 = it1.second.services.begin(); it2 != it1.second.services.end(); ) {
                            if (services.contains(*it2)) {
                                it2++;
                            }
                            else {
                                it2 = it1.second.services.erase(it2);
                            }
                        }
                    }
                }
                else {
                    // [[1.3.a/U5]] PAT_error_2: A PAT table is syntactically incorrect.
                    _counters_flags.PAT_error_2 = true;
                }
            }
            break;
        }
        case TID_CAT: {
            if (pid == PID_CAT) {
                const CAT cat(_duck, table);
                if (cat.isValid()) {
                    // Look for EMM PID to reference.
                    for (const auto& desc : cat.descs) {
                        // If the descriptor is not a CA_descriptor, isValid() will be false.
                        const CADescriptor ca(_duck, desc);
                        if (ca.isValid()) {
                            _pids[ca.ca_pid].type = PIDClass::ECM;
                        }
                    }
                }
                else {
                    // [[2.6/U4]] CAT_error: A CAT table is syntactically incorrect.
                    _counters_flags.CAT_error = true;
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                // Type of the PMT PID.
                _pids[pid].type = PIDClass::PSI;
                // Declare the PCR PID as part of the service, in case it is not otherwise referenced (eg. not the video PID).
                if (pmt.pcr_pid != PID_NULL) {
                    auto& ctx(_pids[pmt.pcr_pid]);
                    ctx.services.insert(pmt.service_id);
                    if (ctx.type == PIDClass::UNDEFINED) {
                        ctx.type = PIDClass::PCR_ONLY;
                    }
                }
                // Declare all components of the service.
                searchECMPIDs(pmt.descs, pmt.service_id);
                for (const auto& it : pmt.streams) {
                    auto& ctx(_pids[it.first]);
                    ctx.type = it.second.getClass(_duck);
                    ctx.services.insert(pmt.service_id);
                    searchECMPIDs(it.second.descs, pmt.service_id);
                }
            }
            else {
                // [[1.5.a/U3]] PMT_error_2: A PMT table is syntactically incorrect.
                _counters_flags.PMT_error_2 = true;
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Declare ECM PID's in a descriptor list as part of a service.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::searchECMPIDs(const DescriptorList& descs, uint16_t service_id)
{
    for (const auto& desc : descs) {
        // If the descriptor is not a CA_descriptor, isValid() will be false.
        const CADescriptor ca(_duck, desc);
        if (ca.isValid()) {
            auto& ctx(_pids[ca.ca_pid]);
            ctx.services.insert(service_id);
            ctx.type = PIDClass::ECM;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete section is available.
//----------------------------------------------------------------------------

void ts::TR101290Analyzer::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();
    const PID pid = section.sourcePID();

    if (pid == PID_PAT && tid != TID_PAT) {
        // [[1.3/2]] PAT_error: a PID 0x0000 does not contain a table_id 0x00 (i.e. a PAT).
        // [[1.3.a/2]] PAT_error_2: Section with table_id other than 0x00 found on PID 0x0000.
        _counters_flags.PAT_error = _counters_flags.PAT_error_2 = true;
    }
    else if (pid == PID_CAT && tid != TID_CAT) {
        // [[2.6/2]] CAT_error: Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001.
        _counters_flags.CAT_error = true;
    }
    else if (pid == PID_RST && tid != TID_RST && tid != TID_ST) {
        // [[3.7/1]] RST_error: Sections with table_id other than 0x71 or 0x72 found on PID 0x0013.
        _counters_flags.RST_error = true;
    }

    switch (section.tableId()) {
        case TID_PAT: {
            auto& ctx(_xtids[section.xtid()]);
            if (pid != PID_PAT || (ctx.last_pcr >= PCR::zero() && (_current_pcr - ctx.last_pcr) > _max_pat_interval)) {
                // [[1.3.a/U4]] a PAT section is present on PID other than 0x0000.
                // [[1.3.a/1]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
                _counters_flags.PAT_error_2 = true;
            }
            if (pid == PID_PAT) {
                ctx.last_pcr = _current_pcr;
            }
            break;
        }
        case TID_CAT: {
            if (pid != PID_CAT) {
                // [[2.6/U3]] CAT_error: A CAT section is present on PID other than 0x0001.
                _counters_flags.CAT_error = true;
            }
            break;
        }
        case TID_RST: {
            auto& ctx(_xtids[section.xtid()]);
            if (pid != PID_RST || (ctx.last_pcr >= PCR::zero() && (_current_pcr - ctx.last_pcr) < _min_rst_interval)) {
                // [[3.7/U3]] RST_error: A RST section is present on PID other than 0x0013.
                // [[3.7/2]] RST_error: Any two sections with table_id = 0x71 (RST) occur on PID 0x0013 within a specified value (25 ms or lower).
                _counters_flags.PAT_error_2 = true;
            }
            if (pid == PID_RST) {
                ctx.last_pcr = _current_pcr;
            }
            break;
        }
        case TID_TOT: {
            // The CRC32 of the TOT shall be manually computed because it is a short section.
            if (section.size() >= 4 && CRC32(section.content(), section.size() - 4) != GetUInt32(section.content() + section.size() - 4)) {
                // [[2.2/1]] CRC_error: CRC error occurred in CAT, PAT, PMT, NIT, EIT, BAT, SDT or TOT table.
                _counters_flags.CRC_error = true;
            }
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
    if (status == Section::INV_CRC32) {
        const TID tid = data.size() < 1 ? TID_NULL : *data.content();
        static const std::set<TID> check_tids {
            TID_CAT, TID_PAT, TID_PMT, TID_NIT_ACT, TID_NIT_OTH,
            TID_EIT_PF_ACT, TID_EIT_PF_OTH,
            TID_EIT_S_ACT_MIN + 0x00, TID_EIT_S_ACT_MIN + 0x01, TID_EIT_S_ACT_MIN + 0x02, TID_EIT_S_ACT_MIN + 0x03,
            TID_EIT_S_ACT_MIN + 0x04, TID_EIT_S_ACT_MIN + 0x05, TID_EIT_S_ACT_MIN + 0x06, TID_EIT_S_ACT_MIN + 0x07,
            TID_EIT_S_ACT_MIN + 0x08, TID_EIT_S_ACT_MIN + 0x09, TID_EIT_S_ACT_MIN + 0x0A, TID_EIT_S_ACT_MIN + 0x0B,
            TID_EIT_S_ACT_MIN + 0x0C, TID_EIT_S_ACT_MIN + 0x0D, TID_EIT_S_ACT_MIN + 0x0E, TID_EIT_S_ACT_MIN + 0x0F,
            TID_EIT_S_OTH_MIN + 0x00, TID_EIT_S_OTH_MIN + 0x01, TID_EIT_S_OTH_MIN + 0x02, TID_EIT_S_OTH_MIN + 0x03,
            TID_EIT_S_OTH_MIN + 0x04, TID_EIT_S_OTH_MIN + 0x05, TID_EIT_S_OTH_MIN + 0x06, TID_EIT_S_OTH_MIN + 0x07,
            TID_EIT_S_OTH_MIN + 0x08, TID_EIT_S_OTH_MIN + 0x09, TID_EIT_S_OTH_MIN + 0x0A, TID_EIT_S_OTH_MIN + 0x0B,
            TID_EIT_S_OTH_MIN + 0x0C, TID_EIT_S_OTH_MIN + 0x0D, TID_EIT_S_OTH_MIN + 0x0E, TID_EIT_S_OTH_MIN + 0x0F,
            TID_BAT, TID_SDT_ACT, TID_SDT_OTH
        };
        if (check_tids.contains(tid)) {
            // [[2.2/1]] CRC_error: CRC error occurred in CAT, PAT, PMT, NIT, EIT, BAT, SDT or TOT table.
            // Note: TOT is separately tested because it is a short section.
            _counters_flags.CRC_error = true;
        }
        else {
            // [[2.2/U2]] CRC_error_2: CRC error occurred in other table id than specified in CRC_error.
            _counters_flags.CRC_error_2 = true;
        }
    }
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

    // Detection of errors which are based on time, for which we need the time of last packet.
    if (_last_pcr >= PCR::zero()) {
        // Unreferenced PID's are detected here. The error can be persistent over cycles
        // of getCountersRestart() and should not be reset.
        counters.Unreferenced_PID = 0;
        for (const auto& it : _pids) {
            // Check if PID is unreferenced and present for more than the max time.
            if (it.first > PID_DVB_LAST &&
                it.first < PID_NULL &&
                it.second.type != PIDClass::EMM &&
                it.second.services.empty() &&
                it.second.first_pcr >= PCR::zero() &&
                (_last_pcr - it.second.first_pcr) > _max_pid_reference_interval)
            {
                // [[3.4]] Unreferenced_PID: PID (other than PAT, CAT, CAT_PIDs, PMT_PIDs, NIT_PID, SDT_PID, TDT_PID, EIT_PID, RST_PID, reserved_for_future_use PIDs, or PIDs user defined as private data streams) not referred to by a PMT within 0,5 s.
                // [[3.4.a]] Unreferenced_PID: PID (other than PMT_PIDs, PIDs with numbers between 0x00 and 0x1F or PIDs user defined as private data streams) not referred to by a PMT or a CAT within 0,5 s.
                // Note: 3.4.a is a superset of 3.4 but they both refer to the same error Unreferenced_PID.
                counters.Unreferenced_PID++;
            }
        }

        // Add timeouts on PID's or TID's. We add them only on the returned counters, not in the instance counters
        // because we will continue to get packets and we will count the error when the next PID or TID is found
        // and we do not want to count the error twice.
        for (const auto& it : _xtids) {
            const TID tid = it.first.tid();
            if (tid == TID_PAT && it.second.last_pcr >= PCR::zero() && (_last_pcr - it.second.last_pcr) > _max_pat_interval) {
                // [[1.3.a/1]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
                counters.PAT_error_2++;
            }
        }
    }
}
