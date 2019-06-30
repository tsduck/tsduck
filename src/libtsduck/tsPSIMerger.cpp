//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPSIMerger.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PSIMerger::PSIMerger(DuckContext& duck, Options options, Report& report) :
    _duck(duck),
    _report(report),
    _options(options),
    _main_demux(_duck, this),                // Complete table handler only
    _main_eit_demux(_duck, nullptr, this),   // Section handler only, do not accumulate incomplete sections.
    _merge_demux(_duck, this),               // Complete table handler only
    _merge_eit_demux(_duck, nullptr, this),  // Section handler only, do not accumulate incomplete sections.
    _pat_pzer(),
    _cat_pzer(),
    _nit_pzer(),
    _sdt_bat_pzer(),
    _eit_pzer(PID_EIT, this),
    _main_pat(),
    _merge_pat(),
    _main_cat(),
    _merge_cat(),
    _main_sdt(),
    _merge_sdt(),
    _main_nit(),
    _merge_nit(),
    _main_bats(),
    _merge_bats(),
    _eits(),
    _max_eits(128) // hard-coded for now
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
            // Do not wait for the two NIT Actual to be merged since some NIT Other can be mixed.
            if ((_options & MERGE_NIT) != 0) {
                _nit_pzer.getNextPacket(pkt);
            }
            break;
        }
        case PID_SDT: {
            // Replace SDT/BAT packets using packetizer when SDT's or BAT's are merged.
            // There is a mixture of merged SDT Actual, mixed SDT Other, merged BAT's.
            if ((_options & (MERGE_SDT | MERGE_BAT)) != 0) {
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
        _report.error(u"too many accumulated EIT sections, not enough space in output EIT PID");
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
    // Do stuffing when there is no more EIT section to send.
    return _eits.empty();
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
    // Enqueue EIT's from main and merged stream.
    if ((demux.demuxId() == DEMUX_MAIN_EIT || demux.demuxId() == DEMUX_MERGE_EIT) &&
        (section.tableId() >= TID_EIT_MIN && section.tableId() <= TID_EIT_MAX) &&
        (_options & MERGE_EIT) != 0)
    {
        const SectionPtr sp(new Section(section, SHARE));
        CheckNonNull(sp.pointer());
        _eits.push_back(sp);
    }
}


//----------------------------------------------------------------------------
// Handle a complete table from either the main or merged stream.
//----------------------------------------------------------------------------

void ts::PSIMerger::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    //@@@@@
}
