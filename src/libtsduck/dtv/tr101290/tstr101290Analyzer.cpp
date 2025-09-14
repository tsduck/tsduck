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
// All rules are listed in the reference documentation for the influx plugin.
// See the Asciidoctor source file tsduck/doc/user/plugins/influx.adoc.
// Each rule has a reference "[[x.y/z]]" which is copied from ETSI TR 101 290
// section 5.2.
//
// Notes:
// - In [[x.y]], "x.y" is the rule number in ETSI TR 101 290 section 5.2.
// - In [[x.y/z]], "z" is a sequence number when the same rule includes distinct checks.
// - In [[x.y/zU]], "U" means unreferenced in ETSI TR 101 290, a new rule which seems necessary.
// - The prefix "(x)" means that the rule is not yet implemented.
//
// Example: [[1.2]] Sync_byte_error: Sync_byte not equal 0x47.
//
//----------------------------------------------------------------------------

#include "tstr101290Analyzer.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTID.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsEIT.h"
#include "tsCADescriptor.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::tr101290::Analyzer::Analyzer(DuckContext& duck, ErrorHandlerInterface* handler) :
    _duck(duck),
    _error_handler(handler)
{
    _demux.setInvalidSectionHandler(this);
    reset();
}


//----------------------------------------------------------------------------
// Reset the analyzer.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::reset()
{
    _bad_sync_count = 0;
    _current_timestamp = _last_timestamp = _last_nit_timestamp = PCR(-1);
    _counters.clear();
    _counters_by_pid.clear();
    _demux.reset();
    _demux.setPIDFilter(AllPIDs());
    _continuity.reset();
    _pids.clear();
    _xtids.clear();
}


//----------------------------------------------------------------------------
// Processing of detected errors.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::addErrorOnce(const UString& reference, ErrorCounter error, PID pid)
{
    addErrorOnce(reference, error, pid, UString::Format(u"PID %n", pid));
}

void ts::tr101290::Analyzer::addErrorOnce(const UString& reference, ErrorCounter error, PID pid, const UString& context)
{
    assert(int(error) >= 0);
    assert(error < COUNTER_COUNT);

    // Count the error only once.
    if (!_counters_flags[error]) {
        _counters_flags[error] = true;
        addError(_counters, _counters_by_pid, reference, error, pid, context);
    }
}

void ts::tr101290::Analyzer::addError(Counters& global, CountersByPID& by_pid, const UString& reference, ErrorCounter error, PID pid)
{
    addError(global, by_pid, reference, error, pid, UString::Format(u"PID %n", pid));
}

void ts::tr101290::Analyzer::addError(Counters& global, CountersByPID& by_pid, const UString& reference, ErrorCounter error, PID pid, const UString& context)
{
    assert(int(error) >= 0);
    assert(error < COUNTER_COUNT);

    _duck.report().debug(u"raise %s (%s) in %s", GetCounterDescription(error).name, reference, context);

    // Increment error counters.
    global[error]++;
    if (_collect_by_pid && pid != PID_NULL) {
        by_pid[pid][error]++;
    }

    // Notify the application.
    if (_error_handler != nullptr) {
        _error_handler->handleTR101290Error(*this, error, reference, context, pid);
    }
}


//----------------------------------------------------------------------------
// The following method feeds the analyzer with a TS packet.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::feedPacket(const PCR& timestamp, const TSPacket& pkt)
{
    // Must be set first. During execution of the various handlers, synchronously called from here,
    // _last_pcr < _current_pcr. Upon return from feedPacket(), _last_pcr == _current_pcr.
    _current_timestamp = timestamp;

    // Clear flags which indicate errors during the processing of this packet.
    _counters_flags.fill(false);

    // Get/initialize PID context for that packet.
    const PID pid = pkt.getPID();
    auto& pidctx(_pids[pid]);
    if (pidctx.first_timestamp < PCR::zero()) {
        // This is the first packet in that PID.
        pidctx.first_timestamp = _current_timestamp;
    }

    // Check sync bytes errors.
    if (pkt.hasValidSync()) {
        _bad_sync_count = 0;
    }
    else {
        // [[1.2]] Sync_byte_error: Sync_byte not equal 0x47.
        addErrorOnce(u"1.2", Sync_byte_error, pid);
        // Count TS_sync_loss exactly once per sequence of sync byte errors.
        if (++_bad_sync_count == _bad_sync_max) {
            // [[1.1]] TS_sync_loss: Loss of synchronization with consideration of hysteresis parameters.
            addErrorOnce(u"1.1", TS_sync_loss, pid);
        }
    }

    // [[2.1]] Transport_error: Transport_error_indicator in the TS-Header is set to "1".
    if (pkt.getTEI()) {
        addErrorOnce(u"2.1", Transport_error, pid);
    }

    // [[1.4]] Continuity_count_error: Incorrect packet order, a packet occurs more than twice, lost packet.
    if (!_continuity.feedPacket(pkt)) {
        addErrorOnce(u"1.4", Continuity_count_error, pid);
    }

    // Track explicit discontinuity indicator which invalidates the PCR.
    if (pkt.getDiscontinuityIndicator()) {
        pidctx.last_disc_timestamp = _current_timestamp;
    }

    // Check PTS repetition.
    if (pkt.hasPTS()) {
        if (pidctx.last_pts_timestamp >= PCR::zero() && (_current_timestamp - pidctx.last_pts_timestamp) > _max_pts_interval) {
            // [[2.5]] PTS_error: PTS repetition period more than 700 ms.
            addErrorOnce(u"2.5", PTS_error, pid);
        }
        pidctx.last_pts_timestamp = _current_timestamp;
    }

    // Check PCR repetition and continuity.
    if (pkt.hasPCR()) {
        if (pidctx.last_pcr_timestamp >= PCR::zero() && (_current_timestamp - pidctx.last_pcr_timestamp) > _max_pcr_interval) {
            // [[2.3/2]] PCR_error: Time interval between two consecutive PCR values more than 100 ms.
            addErrorOnce(u"2.3/2", PCR_error, pid);
            // [[2.3.a]] PCR_repetition_error: Time interval between two consecutive PCR values more than 100 ms.
            addErrorOnce(u"2.3.a", PCR_repetition_error, pid);
        }
        // Explicit PCR discontinuity since last PCR:
        const bool exp_disc = pidctx.last_disc_timestamp >= PCR::zero() && (pidctx.last_pcr_timestamp < PCR::zero() || pidctx.last_disc_timestamp > pidctx.last_pcr_timestamp);
        const uint64_t pcr = pkt.getPCR();
        if (!exp_disc && pidctx.last_pcr_value != INVALID_PCR && PCR(DiffPCR(pidctx.last_pcr_value, pcr)) > _max_pcr_difference) {
            // [[2.3/1]] PCR_error: PCR discontinuity of more than 100 ms occurring without specific indication.
            addErrorOnce(u"2.3/1", PCR_error, pid);
            // [[2.3.b]] PCR_discontinuity_indicator_error: The difference between two consecutive PCR values (PCRi+1 – PCRi) is outside the range of 0...100 ms without the discontinuity_indicator set.
            addErrorOnce(u"2.3.b", PCR_discontinuity_indicator_error, pid);
        }

        pidctx.last_pcr_timestamp = _current_timestamp;
        pidctx.last_pcr_value = pcr;
    }

    // Check PID's that shouldn't be scrambled.
    if (pkt.isScrambled()) {
        if (pid == PID_PAT) {
            // [[1.3/3]] PAT_error: Scrambling_control_field is not 00 for PID 0x0000
            addErrorOnce(u"1.3/3", PAT_error, pid);
            // [[1.3.a/3]] PAT_error_2: Scrambling_control_field is not 00 for PID 0x0000.
            addErrorOnce(u"1.3.a/3", PAT_error_2, pid);
        }
        if (pid == PID_CAT) {
            // [[2.6/1]] CAT_error: Packets with transport_scrambling_control not 00 present, but no section with table_id = 0x01 (i.e. a CAT) present.
            addErrorOnce(u"2.6/1", CAT_error, pid);
        }
        if (pidctx.is_pmt) {
            // [[1.5/2]] PMT_error: Scrambling_control_field is not 00 for all PIDs containing sections with table_id 0x02 (i.e. a PMT).
            addErrorOnce(u"1.5/2", PMT_error, pid);
            // [[1.5.a/2]] PMT_error_2: Scrambling_control_field is not 00 for all packets containing information of sections with table_id 0x02 (i.e. a PMT) on each program_map_PID which is referred to in the PAT.
            addErrorOnce(u"1.5.a/2", PMT_error_2, pid);
        }
    }

    // Check max interval between packets of a PID.
    if (pidctx.last_timestamp >= PCR::zero()) {
        if (pid == PID_PAT && (_current_timestamp - pidctx.last_timestamp) > _max_pat_interval) {
            // [[1.3/1]] PAT_error: PID 0x0000 does not occur at least every 0,5 s
            addErrorOnce(u"1.3/1", PAT_error, pid);
        }
        else if (pidctx.user_pid && (_current_timestamp - pidctx.last_timestamp) > _max_pid_interval) {
            // [[1.6]] PID_error: Referred PID does not occur for a user specified period.
            addErrorOnce(u"1.6", PID_error, pid);
        }
    }

    // Check PSI/SI validity.
    _demux.feedPacket(pkt);

    // Non-standard packet counter.
    _counters[packet_count]++;
    if (_collect_by_pid) {
        _counters_by_pid[pid][packet_count]++;
    }

    // Must be set last.
    pidctx.last_timestamp = timestamp;
    _last_timestamp = timestamp;
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete table is available.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    const PID pid = table.sourcePID();

    switch (table.tableId()) {
        case TID_PAT: {
            if (pid == PID_PAT) {
                const PAT pat(_duck, table);
                if (pat.isValid()) {
                    handlePAT(pat, pid);
                }
                else {
                    // [[1.3.a/5U]] PAT_error_2: A PAT table is syntactically incorrect.
                    addErrorOnce(u"1.3.a/5U", PAT_error_2, pid);
                }
            }
            break;
        }
        case TID_CAT: {
            if (pid == PID_CAT) {
                const CAT cat(_duck, table);
                if (cat.isValid()) {
                    handleCAT(cat, pid);
                }
                else {
                    // [[2.6/4U]] CAT_error: A CAT table is syntactically incorrect.
                    addErrorOnce(u"2.6/4U", CAT_error, pid);
                }
            }
            break;
        }
        case TID_PMT: {
            const PMT pmt(_duck, table);
            if (pmt.isValid()) {
                handlePMT(pmt, pid);
            }
            else {
                // [[1.5.a/3U]] PMT_error_2: A PMT table is syntactically incorrect.
                addErrorOnce(u"1.5.a/3U", PMT_error_2, pid);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Process a new PAT.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handlePAT(const PAT& pat, PID pid)
{
    std::set<uint16_t> services;

    // Declare all PMT PID's as part of their service.
    // Also build a set of all service ids in the PAT.
    for (const auto& it : pat.pmts) {
        auto& ctx(_pids[it.second]);
        services.insert(it.first);
        ctx.services.insert(it.first);
        ctx.type = PIDClass::PSI;
        ctx.is_pmt = true;
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


//----------------------------------------------------------------------------
// Process a new CAT.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handleCAT(const CAT& cat, PID pid)
{
    // Look for EMM PID to reference.
    for (const auto& desc : cat.descs) {
        // If the descriptor is not a CA_descriptor, isValid() will be false.
        const CADescriptor ca(_duck, desc);
        if (ca.isValid()) {
            _pids[ca.ca_pid].type = PIDClass::EMM;
        }
    }
}


//----------------------------------------------------------------------------
// Process a new PMT.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handlePMT(const PMT& pmt, PID pid)
{
    // Type of the PMT PID.
    _pids[pid].type = PIDClass::PSI;
    _pids[pid].is_pmt = true;

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
        // PID_error check currently applies to video and audio PID's only.
        ctx.user_pid = ctx.user_pid || ctx.type == PIDClass::VIDEO || ctx.type == PIDClass::AUDIO;
    }
}


//----------------------------------------------------------------------------
// Declare ECM PID's in a descriptor list as part of a service.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::searchECMPIDs(const DescriptorList& descs, uint16_t service_id)
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
// Check if current timestamp is at an invalid distance from last_timestamp.
//----------------------------------------------------------------------------

bool ts::tr101290::Analyzer::XTIDContext::invMin(PCR current, PCR min) const
{
    return current >= PCR::zero() && last_timestamp >= PCR::zero() && (current - last_timestamp) < min;
}

bool ts::tr101290::Analyzer::XTIDContext::invMax(PCR current, PCR max) const
{
    return current >= PCR::zero() && last_timestamp >= PCR::zero() && (current - last_timestamp) > max;
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when a complete section is available.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();
    const PID pid = section.sourcePID();

    // Section rules by PID.
    if (pid == PID_PAT && tid != TID_PAT) {
        // [[1.3/2]] PAT_error: a PID 0x0000 does not contain a table_id 0x00 (i.e. a PAT).
        addErrorOnce(u"1.3/2", PAT_error, pid);
        // [[1.3.a/2]] PAT_error_2: Section with table_id other than 0x00 found on PID 0x0000.
        addErrorOnce(u"1.3.a/2", PAT_error_2, pid);
    }
    else if (pid == PID_CAT && tid != TID_CAT) {
        // [[2.6/2]] CAT_error: Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001.
        addErrorOnce(u"2.6/2", CAT_error, pid);
    }
    else if (pid == PID_RST && tid != TID_RST && tid != TID_ST) {
        // [[3.7/1]] RST_error: Sections with table_id other than 0x71 or 0x72 found on PID 0x0013.
        addErrorOnce(u"3.7/1", RST_error, pid);
    }
    else if (pid == PID_TDT && tid != TID_TDT && tid != TID_TOT && tid != TID_ST) {
        // [[3.8/2]] TDT_error: Sections with table_id other than 0x70, 0x72 (ST) or 0x73 (TOT) found on PID 0x0014.
        addErrorOnce(u"3.8/2", TDT_error, pid);
    }
    else if (pid == PID_NIT && tid != TID_NIT_ACT && tid != TID_NIT_OTH && tid != TID_ST) {
        // [[3.1/1]] NIT_error: Section with table_id other than 0x40 or 0x41 or 0x72 (i. e. not an NIT or ST) found on PID 0x0010.
        addErrorOnce(u"3.1/1", NIT_error, pid);
        // [[3.1.a/1]] NIT_actual_error: Section with table_id other than 0x40 or 0x41 or 0x72 (i.e. not an NIT or ST) found on PID 0x0010.
        addErrorOnce(u"[3.1.a/1", NIT_actual_error, pid);
    }
    else if (pid == PID_SDT && tid != TID_SDT_ACT && tid != TID_SDT_OTH && tid != TID_BAT && tid != TID_ST) {
        // [[3.5/2]] SDT_error: Sections with table_ids other than 0x42, 0x46, 0x4A or 0x72 found on PID 0x0011.
        addErrorOnce(u"3.5/2", SDT_error, pid);
        // [[3.5.a/2]] SDT_actual_error: Sections with table_ids other than 0x42, 0x46, 0x4A or 0x72 found on PID 0x0011.
        addErrorOnce(u"3.5.a/2", SDT_actual_error, pid);
    }
    else if (pid == PID_EIT && !EIT::IsEIT(tid) && tid != TID_ST) {
        // [[3.6/2]] EIT_error: Sections with table_ids other than in the range 0x4E - 0x6F or 0x72 found on PID 0x0012.
        addErrorOnce(u"3.6/2", EIT_error, pid);
        // [[3.6.a/3]] EIT_actual_error: Sections with table_ids other than in the range 0x4E - 0x6F or 0x72 found on PID 0x0012.
        addErrorOnce(u"3.6.a/3", EIT_actual_error, pid);
    }

    // Section rules by table id.
    if (tid == TID_PAT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_PAT) {
            // [[1.3.a/4U]] PAT_error_2: a PAT section is present on PID other than 0x0000.
            addErrorOnce(u"1.3.a/4U", PAT_error_2, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_pat_interval)) {
            // [[1.3.a/1]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
            addErrorOnce(u"1.3.a/1", PAT_error_2, pid, u"PAT");
        }
        if (pid == PID_PAT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_PMT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (ctx.invMax(_current_timestamp, _max_pmt_interval)) {
            // [[1.5/1]] PMT_error: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on the PID which is referred to in the PAT.
            addErrorOnce(u"1.5/1", PMT_error, pid);
            // [[1.5.a/1]] PMT_error_2: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on each program_map_PID which is referred to in the PAT.
            addErrorOnce(u"1.5.a/1", PMT_error_2, pid);
        }
        ctx.last_timestamp = _current_timestamp;
    }
    else if (tid == TID_CAT && pid != PID_CAT) {
        // [[2.6/3U]] CAT_error: A CAT section is present on PID other than 0x0001.
        addErrorOnce(u"2.6/3U", CAT_error, pid);
    }
    else if (tid == TID_RST) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_RST) {
            // [[3.7/3U]] RST_error: A RST section is present on PID other than 0x0013.
            addErrorOnce(u"3.7/3U", RST_error, pid);
        }
        else if (ctx.invMin(_current_timestamp, _min_rst_interval)) {
            // [[3.7/2]] RST_error: Any two sections with table_id = 0x71 (RST) occur on PID 0x0013 within a specified value (25 ms or lower).
            addErrorOnce(u"3.7/2", RST_error, pid);
        }
        if (pid == PID_RST) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_TDT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_TDT) {
            // [[3.8/4U]] TDT_error: A TDT section is present on PID other than 0x0014.
            addErrorOnce(u"3.8/4U", TDT_error, pid);
        }
        else if (ctx.invMin(_current_timestamp, _min_tdt_interval)) {
            // [[3.8/3]] TDT_error: Any two sections with table_id = 0x70 (TDT) occur on PID 0x0014 within a specified value (25 ms or lower).
            addErrorOnce(u"3.8/3", TDT_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_tdt_interval)) {
            // [[3.8/1]] TDT_error: Sections with table_id = 0x70 (TDT) not present on PID 0x0014 for more than 30 s.
            addErrorOnce(u"[3.8/1", TDT_error, pid);
        }
        if (pid == PID_TDT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_TOT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_TOT) {
            // [[3.2/2U]] SI_PID_error: A SI sections is present on PID other than its allocated PID.
            addErrorOnce(u"3.2/2U", SI_PID_error, pid);
        }
        else if (section.size() >= 4 && CRC32(section.content(), section.size() - 4) != GetUInt32(section.content() + section.size() - 4)) {
            // The CRC32 of the TOT shall be manually computed because it is a short section.
            // [[2.2/1]] CRC_error: CRC error occurred in CAT, PAT, PMT, NIT, EIT, BAT, SDT or TOT table.
            addErrorOnce(u"2.2/1", CRC_error, pid, u"TOT");
        }
        else if (ctx.invMax(_current_timestamp, _max_tot_interval)) {
            // [[3.2/1]] SI_repetition_error: Repetition rate of SI tables outside of specified limits.
            addErrorOnce(u"3.2/1", SI_repetition_error, pid, u"TOT");
        }
        if (pid == PID_TOT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_NIT_ACT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_NIT) {
            // [[3.1.a/4U]] NIT_actual_error: A NIT_actual section is present on PID other than 0x0010.
            addErrorOnce(u"3.1.a/4U", NIT_actual_error, pid);
        }
        else if (ctx.invMin(_current_timestamp, _min_nit_actual_interval)) {
            // [[3.1.a/3]] NIT_actual_error: Any two sections with table_id = 0x40 (NIT_actual) occur on PID 0x0010 within a specified value (25 ms or lower).
            addErrorOnce(u"3.1.a/3", NIT_actual_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_nit_actual_interval)) {
            // [[3.1.a/2]] NIT_actual_error: No section with table_id 0x40 (i.e. an NIT_actual) in PID value 0x0010 for more than 10 s.
            addErrorOnce(u"3.1.a/2", NIT_actual_error, pid);
        }
        if (pid == PID_NIT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_NIT_OTH) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_NIT) {
            // [[3.1.b/2U]] NIT_other_error: A NIT_other section is present on PID other than 0x0010.
            addErrorOnce(u"3.1.b/2U", NIT_other_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_nit_other_interval)) {
            // [[3.1.b/1]] NIT_other_error: Interval between sections with the same section_number and table_id = 0x41 (NIT_other) on PID 0x0010 longer than a specified value (10 s or higher).
            // Note: [[3.1.b/1]] is not exactly implemented. We test the interval between two NIT_other sections, regardless of section number, but with the same network_id (tid-ext).
            addErrorOnce(u"3.1.b/1", NIT_other_error, pid);
        }
        if (pid == PID_NIT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_SDT_ACT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_SDT) {
            // [[3.5.a/4U]] SDT_actual_error: A SDT_actual section is present on PID other than 0x0011.
            addErrorOnce(u"3.5.a/4U", SDT_actual_error, pid);
        }
        else if (ctx.invMin(_current_timestamp, _min_sdt_actual_interval)) {
            // [[3.5.a/3]] SDT_actual_error: Any two sections with table_id = 0x42 (SDT_actual) occur on PID 0x0011 within a specified value (25 ms or lower).
            addErrorOnce(u"3.5.a/3", SDT_actual_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_sdt_actual_interval)) {
            // [[3.5.a/1]] SDT_actual_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
            addErrorOnce(u"3.5.a/1", SDT_actual_error, pid);
            // [[3.5/1]] SDT_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
            addErrorOnce(u"3.5/1", SDT_error, pid);
        }
        if (pid == PID_SDT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_SDT_OTH) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_SDT) {
            // [[3.5.b/2U]] SDT_other_error: A SDT_other section is present on PID other than 0x0011.
            addErrorOnce(u"3.5.b/2U", SDT_other_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_sdt_other_interval)) {
            // [[3.5.b/1]] SDT_other_error: Interval between sections with the same section_number and table_id = 0x46 (SDT, other TS) on PID 0x0011 longer than a specified value (10s or higher).
            // Note: [[3.5.b/1]] is not exactly implemented. We test the interval between two SDT_other sections, regardless of section number, but with the same ts_id (tid-ext).
            addErrorOnce(u"3.5.b/1", SDT_other_error, pid);
        }
        if (pid == PID_SDT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }
    else if (tid == TID_BAT) {
        auto& ctx(_xtids[section.xtid()]);
        ctx.last_pid = pid;
        if (pid != PID_BAT) {
            // [[3.2/2U]] SI_PID_error: A SI sections is present on PID other than its allocated PID.
            addErrorOnce(u"3.2/2U", SI_PID_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_bat_interval)) {
            // [[3.2/1]] SI_repetition_error: Repetition rate of SI tables outside of specified limits.
            addErrorOnce(u"3.2/1", SI_repetition_error, pid, u"BAT");
        }
        if (pid == PID_BAT) {
            ctx.last_timestamp = _current_timestamp;
        }
    }

    // These shall be separately treated.
    if ((tid == TID_NIT_ACT || tid == TID_NIT_OTH) && pid == PID_NIT) {
        if (_last_nit_timestamp >= PCR::zero() && (_current_timestamp - _last_nit_timestamp) > _max_nit_interval) {
            // [[3.1/2]] NIT_error: No section with table_id 0x40 or 0x41 (i.e. an NIT) in PID value 0x0010 for more than 10 s.
            addErrorOnce(u"3.1/2", NIT_error, pid);
        }
        _last_nit_timestamp = _current_timestamp;
    }

    // Various independent EIT checking.
    if (tid == TID_EIT_PF_ACT && pid == PID_EIT) {
        auto& ctx(_xtids[XTID(tid)]); // ignore tid-ext
        ctx.last_pid = pid;
        if (ctx.invMin(_current_timestamp, _min_eitpf_actual_interval)) {
            // [[3.6.a/4]] EIT_actual_error: Any two sections with table_id = 0x4E (EIT-P/F, actual TS) occur on PID 0x0012 within a specified value (25 ms or lower).
            addErrorOnce(u"3.6.a/4", EIT_actual_error, pid);
        }
        else if (ctx.invMax(_current_timestamp, _max_eitpf_actual_interval)) {
            // [[3.6/1]] EIT_error: Sections with table_id = 0x4E (EIT-P/F, actual TS) not present on PID 0x0012 for more than 2 s.
            addErrorOnce(u"3.6/1", EIT_error, pid);
        }
        ctx.last_timestamp = _current_timestamp;
    }
    if (EIT::IsEIT(tid) && pid != PID_EIT) {
        // [[3.6/3U]] EIT_error: An EIT section is present on PID other than 0x0012.
        addErrorOnce(u"3.6/3U", EIT_error, pid);
    }
    if (EIT::IsPresentFollowing(tid) && pid == PID_EIT) {
        auto& ctx(_xtids[section.xtid()]); // include tid-ext
        ctx.last_pid = pid;
        const uint8_t secnum = section.sectionNumber();
        if (secnum > 1) {
            // [[3.6.c/2U]] EIT_PF_error: An EIT P/F section has section number greater that 1.
            addErrorOnce(u"3.6.c/2U", EIT_PF_error, pid);
        }
        else {
            // [[3.6.c/1]] EIT_PF_error: If either section ('0' or '1') of each EIT P/F sub table is present both should exist.
            // Note: "both should exist" is ambiguous. It cannot be "the other one appeared just once long ago".
            // It is also overkill to require section #0 and #1 to be exactly interlaced.
            // ==> we test if the other is within the allowed range.
            const PCR last_other = ctx.last_time_01[~secnum & 0x01];
            if (last_other >= PCR::zero() && (_current_timestamp - last_other) > _max_eitpf_actual_interval) {
                addErrorOnce(u"3.6.c/1", EIT_PF_error, pid);
            }
            if (tid == TID_EIT_PF_ACT && ctx.last_time_01[secnum] >= PCR::zero() && (_current_timestamp - ctx.last_time_01[secnum]) > _max_eitpf_actual_interval) {
                // [[3.6.a/1]] EIT_actual_error: Section '0' with table_id = 0x4E (EIT-P, actual TS) not present on PID 0x0012 for more than 2 s.
                // [[3.6.a/2]] EIT_actual_error: Section '1' with table_id = 0x4E (EIT-F, actual TS) not present on PID 0x0012 for more than 2 s.
                addErrorOnce(secnum == 0 ? u"3.6.a/1]]" : u"3.6.a/2", EIT_actual_error, pid);
            }
            if (tid == TID_EIT_PF_OTH && ctx.last_time_01[secnum] >= PCR::zero() && (_current_timestamp - ctx.last_time_01[secnum]) > _max_eitpf_other_interval) {
                // [[3.6.b/1]] EIT_other_error: Interval between sections '0' with table_id = 0x4F (EIT-P, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
                // [[3.6.b/2]] EIT_other_error: Interval between sections '1' with table_id = 0x4F (EIT-F, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
                addErrorOnce(secnum == 0 ? u"3.6.b/1]]" : u"3.6.b/2", EIT_other_error, pid);
            }
            ctx.last_time_01[secnum] = ctx.last_timestamp = _current_timestamp;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the SectionDemux when an invalid section is detected.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status)
{
    if (status == Section::INV_CRC32) {
        const PID pid = data.sourcePID();
        const TID tid = data.size() < 1 ? TID(TID_NULL) : *data.content();
        const UString context(UString::Format(u"%s in PID %n", TIDName(_duck, tid), pid));

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
            addErrorOnce(u"2.2/1", CRC_error, pid, context);
        }
        else {
            // [[2.2/2U]] CRC_error_2: CRC error occurred in other table id than specified in CRC_error.
            addErrorOnce(u"2.2/2U", CRC_error_2, pid, context);
        }
    }
}


//----------------------------------------------------------------------------
// Get the error counters - public methods.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::getCounters(Counters& counters)
{
    CountersByPID unused;
    getCountersImpl(counters, unused, false);
}

void ts::tr101290::Analyzer::getCountersRestart(Counters& counters)
{
    CountersByPID unused;
    getCountersImpl(counters, unused, false);
    _counters.clear();
    _counters_by_pid.clear();
}

void ts::tr101290::Analyzer::getCounters(Counters& global, CountersByPID& by_pid)
{
    getCountersImpl(global, by_pid, _collect_by_pid);
}

void ts::tr101290::Analyzer::getCountersRestart(Counters& global, CountersByPID& by_pid)
{
    getCountersImpl(global, by_pid, _collect_by_pid);
    _counters.clear();
    _counters_by_pid.clear();
}


//----------------------------------------------------------------------------
// Common code to get global.
//----------------------------------------------------------------------------

void ts::tr101290::Analyzer::getCountersImpl(Counters& global, CountersByPID& by_pid, bool collect_by_pid)
{
    // Current counters.
    global = _counters;
    if (collect_by_pid) {
        by_pid = _counters_by_pid;
    }

    // Detection of errors which are based on time, for which we need the time of last packet.
    if (_last_timestamp >= PCR::zero()) {
        // Unreferenced PID's are detected here. The error can be persistent over cycles
        // of getCountersRestart() and should not be reset.
        global[Unreferenced_PID] = 0;
        for (const auto& it : _pids) {
            // Check if PID is unreferenced and present for more than the max time.
            if (it.first > PID_DVB_LAST &&
                it.first < PID_NULL &&
                it.second.type != PIDClass::EMM &&
                it.second.services.empty() &&
                it.second.first_timestamp >= PCR::zero() &&
                (_last_timestamp - it.second.first_timestamp) > _max_pid_reference_interval)
            {
                // [[3.4]] Unreferenced_PID: PID (other than PAT, CAT, CAT_PIDs, PMT_PIDs, NIT_PID, SDT_PID, TDT_PID, EIT_PID, RST_PID, reserved_for_future_use PIDs, or PIDs user defined as private data streams) not referred to by a PMT within 0,5 s.
                addError(global, by_pid, u"3.4", Unreferenced_PID, it.first);
                // [[3.4.a]] Unreferenced_PID: PID (other than PMT_PIDs, PIDs with numbers between 0x00 and 0x1F or PIDs user defined as private data streams) not referred to by a PMT or a CAT within 0,5 s.
                // Note: 3.4.a is a superset of 3.4 but they both refer to the same error Unreferenced_PID, so don't increment again.
            }
        }

        // Add timeouts on PID's or TID's. We add them only on the returned counters, not in the instance counters
        // because we will continue to get packets and we will count the error when the next PID or TID is found
        // and we do not want to count the error twice.

        // Check PAT PID repetition.
        if (_pids[PID_PAT].last_timestamp >= PCR::zero() && (_last_timestamp - _pids[PID_PAT].last_timestamp) > _max_pat_interval) {
            // [[1.3/1]] PAT_error: PID 0x0000 does not occur at least every 0,5 s
            addError(global, by_pid, u"1.3/1", PAT_error, PID_PAT);
        }

        // Check user PID's (audio, video, etc.) repetition.
        for (const auto& it : _pids) {
            if (it.second.user_pid && it.second.last_timestamp >= PCR::zero() && (_last_timestamp - it.second.last_timestamp) > _max_pid_interval) {
                // [[1.6]] PID_error: Referred PID does not occur for a user specified period.
                addError(global, by_pid, u"1.6", PID_error, it.first);
            }
        }

        // Check sections repetition.
        for (const auto& it : _xtids) {
            const PID pid = it.second.last_pid;
            const TID tid = it.first.tid();
            if (tid == TID_PAT && it.second.invMax(_last_timestamp, _max_pat_interval)) {
                // [[1.3.a/1]] PAT_error_2: Sections with table_id 0x00 do not occur at least every 0,5 s on PID 0x0000.
                addError(global, by_pid, u"1.3.a/1", PAT_error_2, pid, it.first.toString());
            }
            else if (tid == TID_PMT && it.second.invMax(_last_timestamp, _max_pmt_interval)) {
                // [[1.5/1]] PMT_error: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on the PID which is referred to in the PAT.
                addError(global, by_pid, u"1.5/1", PMT_error, pid, it.first.toString());
                // [[1.5.a/1]] PMT_error_2: Sections with table_id 0x02, (i.e. a PMT), do not occur at least every 0,5 s on each program_map_PID which is referred to in the PAT.
                addError(global, by_pid, u"1.5.a/1", PMT_error_2, pid, it.first.toString());
            }
            else if (tid == TID_TDT && it.second.invMax(_last_timestamp, _max_tdt_interval)) {
                // [[3.8/1]] TDT_error: Sections with table_id = 0x70 (TDT) not present on PID 0x0014 for more than 30 s.
                addError(global, by_pid, u"[3.8/1", TDT_error, pid, u"TDT");
            }
            else if (tid == TID_TOT && it.second.invMax(_last_timestamp, _max_tot_interval)) {
                // [[3.2/1]] SI_repetition_error: Repetition rate of SI tables outside of specified limits.
                addError(global, by_pid, u"3.2/1", SI_repetition_error, pid, u"TOT");
            }
            else if (tid == TID_NIT_ACT && it.second.invMax(_last_timestamp, _max_nit_actual_interval)) {
                // [[3.1.a/2]] NIT_actual_error: No section with table_id 0x40 (i.e. an NIT_actual) in PID value 0x0010 for more than 10 s.
                addError(global, by_pid, u"3.1.a/2", NIT_actual_error, pid, it.first.toString());
            }
            else if (tid == TID_NIT_OTH && it.second.invMax(_last_timestamp, _max_nit_other_interval)) {
                // [[3.1.b/1]] NIT_other_error: Interval between sections with the same section_number and table_id = 0x41 (NIT_other) on PID 0x0010 longer than a specified value (10 s or higher).
                // Note: [[3.1.b/1]] is not exactly implemented. We test the interval between two NIT_other sections, regardless of section number, but with the same network_id (tid-ext).
                addError(global, by_pid, u"3.1.b/1", NIT_other_error, pid, it.first.toString());
            }
            else if (tid == TID_SDT_ACT && it.second.invMax(_last_timestamp, _max_sdt_actual_interval)) {
                // [[3.5.a/1]] SDT_actual_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
                addError(global, by_pid, u"3.5.a/1", SDT_actual_error, pid, it.first.toString());
                // [[3.5/1]] SDT_error: Sections with table_id = 0x42 (SDT, actual TS) not present on PID 0x0011 for more than 2 s.
                addError(global, by_pid, u"3.5/1", SDT_error, pid, it.first.toString());
            }
            else if (tid == TID_SDT_OTH && it.second.invMax(_last_timestamp, _max_sdt_other_interval)) {
                // [[3.5.b/1]] SDT_other_error: Interval between sections with the same section_number and table_id = 0x46 (SDT, other TS) on PID 0x0011 longer than a specified value (10s or higher).
                // Note: [[3.5.b/1]] is not exactly implemented. We test the interval between two SDT_other sections, regardless of section number, but with the same ts_id (tid-ext).
                addError(global, by_pid, u"3.5.b/1", SDT_other_error, pid, it.first.toString());
            }
            else if (tid == TID_BAT && it.second.invMax(_last_timestamp, _max_bat_interval)) {
                // [[3.2/1]] SI_repetition_error: Repetition rate of SI tables outside of specified limits.
                addError(global, by_pid, u"3.2/1", SI_repetition_error, pid, it.first.toString());
            }
            else if (tid == TID_EIT_PF_ACT) {
                if (it.second.invMax(_last_timestamp, _max_eitpf_actual_interval)) {
                    // [[3.6/1]] EIT_error: Sections with table_id = 0x4E (EIT-P/F, actual TS) not present on PID 0x0012 for more than 2 s.
                    addError(global, by_pid, u"3.6/1", EIT_error, pid, it.first.toString());
                }
                if (it.second.last_time_01[0] >= PCR::zero() && (_last_timestamp - it.second.last_time_01[0]) > _max_eitpf_actual_interval) {
                    // [[3.6.a/1]] EIT_actual_error: Section '0' with table_id = 0x4E (EIT-P, actual TS) not present on PID 0x0012 for more than 2 s.
                    addError(global, by_pid, u"3.6.a/1]]", EIT_actual_error, pid, it.first.toString());
                }
                if (it.second.last_time_01[1] >= PCR::zero() && (_last_timestamp - it.second.last_time_01[1]) > _max_eitpf_actual_interval) {
                    // [[3.6.a/2]] EIT_actual_error: Section '1' with table_id = 0x4E (EIT-F, actual TS) not present on PID 0x0012 for more than 2 s.
                    addError(global, by_pid, u"3.6.a/2", EIT_actual_error, pid, it.first.toString());
                }
            }
            else if (tid == TID_EIT_PF_OTH) {
                if (it.second.last_time_01[0] >= PCR::zero() && (_last_timestamp - it.second.last_time_01[0]) > _max_eitpf_other_interval) {
                    // [[3.6.b/1]] EIT_other_error: Interval between sections '0' with table_id = 0x4F (EIT-P, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
                    addError(global, by_pid, u"3.6.b/1]]", EIT_other_error, pid, it.first.toString());
                }
                if (it.second.last_time_01[1] >= PCR::zero() && (_last_timestamp - it.second.last_time_01[1]) > _max_eitpf_other_interval) {
                    // [[3.6.b/2]] EIT_other_error: Interval between sections '1' with table_id = 0x4F (EIT-F, other TS) on PID 0x0012 longer than a specified value (10 s or higher).
                    addError(global, by_pid, u"3.6.b/2", EIT_other_error, pid, it.first.toString());
                }
            }
        }

        // These shall be separately treated.
        if (_last_nit_timestamp >= PCR::zero() && (_last_timestamp - _last_nit_timestamp) > _max_nit_interval) {
            // [[3.1/2]] NIT_error: No section with table_id 0x40 or 0x41 (i.e. an NIT) in PID value 0x0010 for more than 10 s.
            addError(global, by_pid, u"3.1/2", NIT_error, PID_NIT, u"NIT");
        }
    }

    // Clear counters by PID, if we detected some in the above loop and they are not claimed.
    if (!collect_by_pid) {
        by_pid.clear();
    }
}
