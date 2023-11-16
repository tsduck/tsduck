//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSIMerger.h"
#include "tsCADescriptor.h"
#include "tsTSPacket.h"
#include "tsAlgorithm.h"
#include "tsEIT.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PSIMerger::PSIMerger(DuckContext& duck, Options options) :
    _duck(duck),
    _options(options)
{
    reset();
}


//----------------------------------------------------------------------------
// Reset the PSI merger.
//----------------------------------------------------------------------------

void ts::PSIMerger::reset()
{
    reset(_options);
}

void ts::PSIMerger::reset(Options options)
{
    // Remember now options.
    _options = options;

    // Configure all the demux.
    // Note that we do not use the same demux for PAT/CAT/SDT/BAT/NIT and for EIT's.
    // In the EIT demux, we do not accumulate incomplete sections, which saves a lot of memory.
    _main_demux.reset();
    _main_demux.setDemuxId(DEMUX_MAIN);

    _main_eit_demux.reset();
    _main_eit_demux.setDemuxId(DEMUX_MAIN_EIT);

    _merge_demux.reset();
    _merge_demux.setDemuxId(DEMUX_MERGE);

    _merge_eit_demux.reset();
    _merge_eit_demux.setDemuxId(DEMUX_MERGE_EIT);

    if ((_options & MERGE_PAT) != 0) {
        _main_demux.addPID(PID_PAT);
        _merge_demux.addPID(PID_PAT);
    }
    else {
        _main_demux.removePID(PID_PAT);
        _merge_demux.removePID(PID_PAT);
    }

    if ((_options & MERGE_CAT) != 0) {
        _main_demux.addPID(PID_CAT);
        _merge_demux.addPID(PID_CAT);
    }
    else {
        _main_demux.removePID(PID_CAT);
        _merge_demux.removePID(PID_CAT);
    }

    if ((_options & MERGE_NIT) != 0) {
        _main_demux.addPID(PID_NIT);
        _merge_demux.addPID(PID_NIT);
    }
    else {
        _main_demux.removePID(PID_NIT);
        _merge_demux.removePID(PID_NIT);
    }

    if ((_options & (MERGE_SDT | MERGE_BAT)) != 0) {
        // SDT and BAT share the same PID.
        _main_demux.addPID(PID_BAT);
        _merge_demux.addPID(PID_BAT);
    }
    else {
        _main_demux.removePID(PID_BAT);
        _merge_demux.removePID(PID_BAT);
    }

    if ((_options & MERGE_EIT) != 0) {
        _main_eit_demux.addPID(PID_EIT);
        _merge_eit_demux.addPID(PID_EIT);
    }
    else {
        _main_eit_demux.removePID(PID_EIT);
        _merge_eit_demux.removePID(PID_EIT);
    }

    // Configure the packetizers.
    _pat_pzer.reset();
    _pat_pzer.setPID(PID_PAT);

    _cat_pzer.reset();
    _cat_pzer.setPID(PID_CAT);

    _nit_pzer.reset();
    _nit_pzer.setPID(PID_NIT);

    _sdt_bat_pzer.reset();
    _sdt_bat_pzer.setPID(PID_SDT);

    _eit_pzer.reset();
    _eit_pzer.setPID(PID_EIT);

    // Make sure that all input tables are invalid.
    _main_tsid.reset();
    _main_pat.invalidate();
    _merge_pat.invalidate();
    _main_cat.invalidate();
    _merge_cat.invalidate();
    _main_sdt.invalidate();
    _merge_sdt.invalidate();
    _main_nit.invalidate();
    _merge_nit.invalidate();
    _main_bats.clear();
    _merge_bats.clear();
    _eits.clear();
}


//----------------------------------------------------------------------------
// Get main and merged complete TS id. Return false if not yet known.
//----------------------------------------------------------------------------

bool ts::PSIMerger::getTransportStreamIds(TransportStreamId& main, TransportStreamId& merge) const
{
    // We can get the TS id from the PAT or SDT-Actual and the original network id from the SDT-Actual.
    // Since we need the SDT-Actual, no need to use the PAT.
    if (_main_sdt.isValid() && _merge_sdt.isValid()) {
        main.transport_stream_id = _main_sdt.ts_id;
        main.original_network_id = _main_sdt.onetw_id;
        merge.transport_stream_id = _merge_sdt.ts_id;
        merge.original_network_id = _merge_sdt.onetw_id;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Feed a packet from the main stream.
//----------------------------------------------------------------------------

bool ts::PSIMerger::feedMainPacket(TSPacket& pkt)
{
    const PID pid = pkt.getPID();
    bool ok = true;

    // Filter sections to process / merge.
    _main_demux.feedPacket(pkt);
    _main_eit_demux.feedPacket(pkt);

    // Process packets depending on PID.
    switch (pid) {
        case PID_PAT: {
            // Replace PAT packets using packetizer if a new PAT was generated.
            // Let original packets pass as long as the two PAT's are not merged.
            if (_main_pat.isValid() && _merge_pat.isValid()) {
                _pat_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_CAT: {
            // Replace CAT packets using packetizer if a new CAT was generated.
            // Let original packets pass as long as the two CAT's are not merged.
            if (_main_cat.isValid() && _merge_cat.isValid()) {
                _cat_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_NIT: {
            // Replace NIT packets using packetizer when NIT's are merged.
            // Let original packets pass as long as the two NIT-Actual are not merged.
            // In the meantime, we may miss NIT-Other from the merged stream but we do not care.
            if (_main_nit.isValid() && _merge_nit.isValid()) {
                _nit_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_SDT: {
            // Replace SDT/BAT packets using packetizer when SDT's or BAT's are merged.
            // There is a mixture of merged SDT Actual, mixed SDT Other, merged BAT's.
            // Let original packets pass as long as the two SDT-Actual are not merged.
            // In the meantime, we may miss BAT and SDT-Other from the merged stream but we do not care.
            if (_main_sdt.isValid() && _merge_sdt.isValid()) {
                _sdt_bat_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_EIT: {
            if ((_options & MERGE_EIT) != 0) {
                // Remplace EIT packets in both streams, main and merge.
                _eit_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_TDT: {
            if ((_options & KEEP_MAIN_TDT) == 0) {
                // Do not keep TDT/TOT from main stream.
                pkt = NullPacket;
            }
            break;
        }
        default: {
            // Other PID's are left unmodified.
            break;
        }
    }

    // Check EIT overflow before returning.
    return checkEITs() && ok;
}


//----------------------------------------------------------------------------
// Feed a packet from the merged stream.
//----------------------------------------------------------------------------

bool ts::PSIMerger::feedMergedPacket(TSPacket& pkt)
{
    const PID pid = pkt.getPID();
    bool ok = true;

    // Filter sections to process / merge.
    _merge_demux.feedPacket(pkt);
    _merge_eit_demux.feedPacket(pkt);

    // Process packets depending on PID.
    const bool null_merged = (_options & NULL_MERGED) != 0;
    const bool null_unmerged = (_options & NULL_UNMERGED) != 0;
    switch (pid) {
        case PID_PAT: {
            const bool merge = (_options & MERGE_PAT) != 0;
            if ((merge && null_merged) || (!merge && null_unmerged)) {
                pkt = NullPacket;
            }
            break;
        }
        case PID_CAT: {
            const bool merge = (_options & MERGE_CAT) != 0;
            if ((merge && null_merged) || (!merge && null_unmerged)) {
                pkt = NullPacket;
            }
            break;
        }
        case PID_NIT: {
            const bool merge = (_options & MERGE_NIT) != 0;
            if ((merge && null_merged) || (!merge && null_unmerged)) {
                pkt = NullPacket;
            }
            break;
        }
        case PID_SDT: {
            // Same PID for BAT and SDT.
            const bool merge = (_options & (MERGE_SDT | MERGE_BAT)) != 0;
            if ((merge && null_merged) || (!merge && null_unmerged)) {
                pkt = NullPacket;
            }
            break;
        }
        case PID_EIT: {
            if ((_options & MERGE_EIT) != 0) {
                // Remplace EIT packets in both streams, main and merge.
                // We never nullify the merged EIT stream, otherwise there will not be enough packets for all EIT's.
                _eit_pzer.getNextPacket(pkt);
            }
            else if (null_unmerged) {
                pkt = NullPacket;
            }
            break;
        }
        case PID_TDT: {
            if ((_options & KEEP_MERGE_TDT) == 0) {
                // Do not keep TDT/TOT from merge stream.
                pkt = NullPacket;
            }
            break;
        }
        default: {
            // Other PID's are left unmodified.
            break;
        }
    }

    // Check EIT overflow before returning.
    return checkEITs() && ok;
}


//----------------------------------------------------------------------------
// Check that the queue of EIT's does not overflow.
//----------------------------------------------------------------------------

bool ts::PSIMerger::checkEITs()
{
    // Fool-proof check.
    if (_eits.size() > _max_eits) {
        _duck.report().error(u"too many accumulated EIT sections, not enough space in output EIT PID");
        // Drop oldest EIT's.
        while (_eits.size() > _max_eits) {
            _eits.pop_front();
        }
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface (for EIT's only).
//----------------------------------------------------------------------------

bool ts::PSIMerger::doStuffing()
{
    // Never do stuffing, always pack EIT's to make sure we have enough packets to reserialize EIT's.
    return false;
}

void ts::PSIMerger::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_eits.empty()) {
        // No EIT section to provide.
        section.clear();
    }
    else {
        // Remove one EIT section from the queue for insertion.
        section = _eits.front();
        _eits.pop_front();
    }
}


//----------------------------------------------------------------------------
// Handle an individual section from either the main or merged stream.
//----------------------------------------------------------------------------

void ts::PSIMerger::handleSection(SectionDemux& demux, const Section& section)
{
    const TID tid = section.tableId();
    const bool is_eit = EIT::IsEIT(tid) && section.sourcePID() == PID_EIT;
    const bool is_actual = EIT::IsActual(tid);

    // Enqueue EIT's from main and merged stream.
    if (is_eit && (_options & MERGE_EIT) != 0) {

        // Create a copy of the section object (shared section data).
        const SectionPtr sp(new Section(section, ShareMode::SHARE));
        CheckNonNull(sp.pointer());

        if (demux.demuxId() != DEMUX_MERGE_EIT || !is_actual) {
            // Not an EIT-Actual from the merge stream, pass section without modification.
            _eits.push_back(sp);
        }
        else if (sp->payloadSize() >= 2 && _main_tsid.has_value()) {
            // This is an EIT-Actual from merged stream and we know the main TS id.
            // Patch the EIT with new TS id before enqueueing.
            // The TSid is in the first two bytes of the EIT payload.
            sp->setUInt16(0, _main_tsid.value(), true);
            _eits.push_back(sp);
        }
    }
}


//----------------------------------------------------------------------------
// Handle a complete table from either the main or merged stream.
//----------------------------------------------------------------------------

void ts::PSIMerger::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (demux.demuxId()) {
        case DEMUX_MAIN:
            handleMainTable(table);
            break;
        case DEMUX_MERGE:
            handleMergeTable(table);
            break;
        default:
            assert(false); // Unknown demux. Should not get there.
            break;
    }
}


//----------------------------------------------------------------------------
// Handle a table from the main transport stream.
//----------------------------------------------------------------------------

void ts::PSIMerger::handleMainTable(const BinaryTable& table)
{
    // The processing is the same for PAT, CAT, BAT, NIT-Actual and SDT-Actual:
    // update last input table and merge with table from the other stream.
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(_duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                _main_tsid = pat.ts_id;
                copyTableKeepVersion(_main_pat, pat);
                mergePAT();
            }
            break;
        }
        case TID_CAT: {
            const CAT cat(_duck, table);
            if (cat.isValid() && table.sourcePID() == PID_CAT) {
                copyTableKeepVersion(_main_cat, cat);
                mergeCAT();
            }
            break;
        }
        case TID_NIT_ACT: {
            const NIT nit(_duck, table);
            if (nit.isValid() && table.sourcePID() == PID_NIT) {
                copyTableKeepVersion(_main_nit, nit);
                mergeNIT();
            }
            break;
        }
        case TID_NIT_OTH: {
            if (table.sourcePID() == PID_NIT) {
                // This is a NIT-Other. It must be reinserted without modification in the NIT PID.
                _nit_pzer.removeSections(table.tableId(), table.tableIdExtension());
                _nit_pzer.addTable(table);
            }
            break;
        }
        case TID_SDT_ACT: {
            const SDT sdt(_duck, table);
            if (sdt.isValid() && table.sourcePID() == PID_SDT) {
                _main_tsid = sdt.ts_id;
                copyTableKeepVersion(_main_sdt, sdt);
                mergeSDT();
            }
            break;
        }
        case TID_SDT_OTH: {
            if (table.sourcePID() == PID_SDT) {
                // This is an SDT-Other. It must be reinserted without modification in the SDT/BAT PID.
                _sdt_bat_pzer.removeSections(table.tableId(), table.tableIdExtension());
                _sdt_bat_pzer.addTable(table);
            }
            break;
        }
        case TID_BAT: {
            const BAT bat(_duck, table);
            if (bat.isValid() && table.sourcePID() == PID_BAT) {
                if (!Contains(_main_bats, bat.bouquet_id)) {
                    // No previous BAT for this bouquet.
                    _main_bats[bat.bouquet_id] = bat;
                }
                else {
                    copyTableKeepVersion(_main_bats[bat.bouquet_id], bat);
                }
                mergeBAT(bat.bouquet_id);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Handle a table from the merged transport stream.
//----------------------------------------------------------------------------

void ts::PSIMerger::handleMergeTable(const BinaryTable& table)
{
    // The processing the same for PAT, CAT and SDT-Actual:
    // update last input table and merge with table from the other stream.
    switch (table.tableId()) {
        case TID_PAT: {
            const PAT pat(_duck, table);
            if (pat.isValid() && table.sourcePID() == PID_PAT) {
                _merge_pat = pat;
                mergePAT();
            }
            break;
        }
        case TID_CAT: {
            const CAT cat(_duck, table);
            if (cat.isValid() && table.sourcePID() == PID_CAT) {
                _merge_cat = cat;
                mergeCAT();
            }
            break;
        }
        case TID_NIT_ACT: {
            const NIT nit(_duck, table);
            if (nit.isValid() && table.sourcePID() == PID_NIT) {
                _merge_nit = nit;
                mergeNIT();
            }
            break;
        }
        case TID_SDT_ACT: {
            const SDT sdt(_duck, table);
            if (sdt.isValid() && table.sourcePID() == PID_SDT) {
                _merge_sdt = sdt;
                mergeSDT();
            }
            break;
        }
        case TID_BAT: {
            const BAT bat(_duck, table);
            if (bat.isValid() && table.sourcePID() == PID_BAT) {
                _merge_bats[bat.bouquet_id] = bat;
                mergeBAT(bat.bouquet_id);
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Merge the PAT's and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::PSIMerger::mergePAT()
{
    // Check that we have valid tables to merge.
    if (!_main_pat.isValid() || !_merge_pat.isValid()) {
        return;
    }

    _duck.report().debug(u"merging PAT");

    // Build a new PAT based on last main PAT with incremented version number.
    PAT pat(_main_pat);
    pat.version = (pat.version + 1) & SVERSION_MASK;

    // Add all services from merged stream into main PAT.
    for (const auto& merge : _merge_pat.pmts) {
        // Check if the service already exists in the main PAT.
        if (Contains(pat.pmts, merge.first)) {
            _duck.report().error(u"service conflict, service 0x%X (%d) exists in the two streams, dropping from merged stream", {merge.first, merge.first});
        }
        else {
            pat.pmts[merge.first] = merge.second;
            _duck.report().verbose(u"adding service 0x%X (%d) in PAT from merged stream", {merge.first, merge.first});
        }
    }

    // Replace the PAT in the packetizer.
    _pat_pzer.removeSections(TID_PAT);
    _pat_pzer.addTable(_duck, pat);

    // Save PAT version number for later increment.
    _main_pat.version = pat.version;
}


//----------------------------------------------------------------------------
// Merge the CAT's and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::PSIMerger::mergeCAT()
{
    // Check that we have valid tables to merge.
    if (!_main_cat.isValid() || !_merge_cat.isValid()) {
        return;
    }

    _duck.report().debug(u"merging CAT");

    // Build a new CAT based on last main CAT with incremented version number.
    CAT cat(_main_cat);
    cat.version = (cat.version + 1) & SVERSION_MASK;

    // Add all CA descriptors from merged stream into main CAT.
    for (size_t index = _merge_cat.descs.search(DID_CA); index < _merge_cat.descs.count(); index = _merge_cat.descs.search(DID_CA, index + 1)) {
        const CADescriptor ca(_duck, *_merge_cat.descs[index]);
        // Check if the same EMM PID already exists in the main CAT.
        if (CADescriptor::SearchByPID(_main_cat.descs, ca.ca_pid) < _main_cat.descs.count()) {
            _duck.report().error(u"EMM PID conflict, PID 0x%X (%d) referenced in the two streams, dropping from merged stream", {ca.ca_pid, ca.ca_pid});
        }
        else {
            cat.descs.add(_merge_cat.descs[index]);
            _duck.report().verbose(u"adding EMM PID 0x%X (%d) in CAT from merged stream", {ca.ca_pid, ca.ca_pid});
        }
    }

    // Replace the CAT in the packetizer.
    _cat_pzer.removeSections(TID_CAT);
    _cat_pzer.addTable(_duck, cat);

    // Save CAT version number for later increment.
    _main_cat.version = cat.version;
}


//----------------------------------------------------------------------------
// Merge the two SDT-Actual and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::PSIMerger::mergeSDT()
{
    // Check that we have valid tables to merge.
    if (!_main_sdt.isValid() || !_merge_sdt.isValid()) {
        return;
    }

    _duck.report().debug(u"merging SDT");

    // Build a new SDT based on last main SDT with incremented version number.
    SDT sdt(_main_sdt);
    sdt.version = (sdt.version + 1) & SVERSION_MASK;

    // Add all services from merged stream into main SDT.
    for (const auto& merge : _merge_sdt.services) {
        // Check if the service already exists in the main SDT.
        if (Contains(sdt.services, merge.first)) {
            _duck.report().error(u"service conflict, service 0x%X (%d) exists in the two streams, dropping from merged stream", {merge.first, merge.first});
        }
        else {
            sdt.services[merge.first] = merge.second;
            _duck.report().verbose(u"adding service \"%s\", id 0x%X (%d) in SDT from merged stream", {merge.second.serviceName(_duck), merge.first, merge.first});
        }
    }

    // Replace the SDT in the packetizer.
    _sdt_bat_pzer.removeSections(TID_SDT_ACT, sdt.ts_id);
    _sdt_bat_pzer.addTable(_duck, sdt);

    // Save SDT version number for later increment.
    _main_sdt.version = sdt.version;
}


//----------------------------------------------------------------------------
// Merge the two NIT-Actual and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::PSIMerger::mergeNIT()
{
    TransportStreamId main_tsid;
    TransportStreamId merge_tsid;

    // Check that we have valid tables to merge. We also need the extended transport stream ids.
    if (!_main_nit.isValid() || !_merge_nit.isValid() || !getTransportStreamIds(main_tsid, merge_tsid)) {
        return;
    }

    _duck.report().debug(u"merging NIT");

    // Build a new NIT based on last main NIT with incremented version number.
    NIT nit(_main_nit);
    nit.version = (nit.version + 1) & SVERSION_MASK;

    // If the two TS are from the same network and have distinct TS ids, remove the
    // description of the merged TS since it is now merged.
    if (_main_nit.network_id == _merge_nit.network_id && main_tsid != merge_tsid) {
        nit.transports.erase(merge_tsid);
    }

    // Description of the merged TS from its description in its own NIT.
    auto merge_ts = _merge_nit.transports.find(merge_tsid);

    // If the merged stream has its own description, add the descriptors into
    // the description of the merged TS in the main NIT, if there is one.
    // This is not perfect since some descriptors can be duplicated.
    // In some cases such as service_list_descriptor, the two descriptors
    // Merge transport streams description into main NIT.
    if (merge_ts != _merge_nit.transports.end()) {
        nit.transports[main_tsid].descs.add(merge_ts->second.descs);
    }

    // Replace the NIT in the packetizer.
    _nit_pzer.removeSections(TID_NIT_ACT, nit.network_id);
    _nit_pzer.addTable(_duck, nit);

    // Save NIT version number for later increment.
    _main_nit.version = nit.version;
}


//----------------------------------------------------------------------------
// Merge two BAT for the same bouquet and build a new one into the packetizer.
//----------------------------------------------------------------------------

void ts::PSIMerger::mergeBAT(uint16_t bouquet_id)
{
    TransportStreamId main_tsid;
    TransportStreamId merge_tsid;

    // Existing main and merge BAT for this bouquet.
    auto main(_main_bats.find(bouquet_id));
    auto merge(_merge_bats.find(bouquet_id));

    // Check that we have valid tables to merge. We also need the extended transport stream ids.
    if (main == _main_bats.end() || merge == _merge_bats.end() || !main->second.isValid() || !merge->second.isValid() || !getTransportStreamIds(main_tsid, merge_tsid)) {
        return;
    }

    _duck.report().debug(u"merging BAT for bouquet id 0x%X (%d)", {bouquet_id, bouquet_id});

    // Build a new BAT based on last main BAT with incremented version number.
    BAT bat(main->second);
    bat.version = (bat.version + 1) & SVERSION_MASK;

    // If the two TS have distinct TS ids, remove the description of the merged TS since it is now merged.
    if (main_tsid != merge_tsid) {
        bat.transports.erase(merge_tsid);
    }

    // Description of the merged TS from its description in its own BAT.
    auto merge_ts = merge->second.transports.find(merge_tsid);

    // If the merged stream has its own description, add the descriptors into
    // the description of the merged TS in the main NIT, if there is one.
    // This is not perfect since some descriptors can be duplicated.
    // In some cases such as service_list_descriptor, the two descriptors
    // Merge transport streams description into main NIT.
    if (merge_ts != merge->second.transports.end()) {
        bat.transports[main_tsid].descs.add(merge_ts->second.descs);
    }

    // Replace the BAT in the packetizer.
    _sdt_bat_pzer.removeSections(TID_BAT, bouquet_id);
    _sdt_bat_pzer.addTable(_duck, bat);

    // Save NIT version number for later increment.
    main->second.version = bat.version;
}
