//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSectionDemux.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsReportFile.h"
#include "tsEIT.h"


//----------------------------------------------------------------------------
// Demux status information.
//----------------------------------------------------------------------------

ts::SectionDemux::Status::Status() :
    invalid_ts(0),
    discontinuities(0),
    scrambled(0),
    inv_sect_length(0),
    inv_sect_index(0),
    inv_sect_version(0),
    wrong_crc(0),
    is_next(0),
    truncated_sect(0)
{
}

ts::SectionDemux::Status::Status(const SectionDemux& demux) : Status()
{
    demux.getStatus(*this);
}

// Reset the content of the demux status.
void ts::SectionDemux::Status::reset()
{
    invalid_ts = 0;
    discontinuities = 0;
    scrambled = 0;
    inv_sect_length = 0;
    inv_sect_index = 0;
    inv_sect_version = 0;
    wrong_crc = 0;
    is_next = 0;
    truncated_sect = 0;
}

// Check if any counter is non zero.
bool ts::SectionDemux::Status::hasErrors() const
{
    return
        invalid_ts != 0 ||
        discontinuities != 0 ||
        scrambled != 0 ||
        inv_sect_length != 0 ||
        inv_sect_index != 0 ||
        inv_sect_version != 0 ||
        wrong_crc != 0 ||
        is_next != 0 ||
        truncated_sect != 0;
}


//----------------------------------------------------------------------------
// Display content of a status block.
//----------------------------------------------------------------------------

std::ostream& ts::SectionDemux::Status::display(std::ostream& strm, int indent, bool errors_only) const
{
    ReportFile<> rep(strm);
    const UString margin(indent, ' ');
    display(rep, Severity::Info, margin, errors_only);
    return strm;
}

void ts::SectionDemux::Status::display(Report& report, int level, const UString& prefix, bool errors_only) const
{
    if (!errors_only || invalid_ts != 0) {
        report.log(level, u"%sInvalid TS packets: %'d", {prefix, invalid_ts});
    }
    if (!errors_only || discontinuities != 0) {
        report.log(level, u"%sTS packets discontinuities: %'d", {prefix, discontinuities});
    }
    if (!errors_only || scrambled != 0) {
        report.log(level, u"%sScrambled TS packets: %'d", {prefix, scrambled});
    }
    if (!errors_only || inv_sect_length != 0) {
        report.log(level, u"%sInvalid section lengths: %'d", {prefix, inv_sect_length});
    }
    if (!errors_only || truncated_sect != 0) {
        report.log(level, u"%sTruncated sections: %'d", {prefix, truncated_sect});
    }
    if (!errors_only || inv_sect_index != 0) {
        report.log(level, u"%sInvalid section index: %'d", {prefix, inv_sect_index});
    }
    if (!errors_only || inv_sect_version != 0) {
        report.log(level, u"%sInvalid unchanged section version: %'d", {prefix, inv_sect_version});
    }
    if (!errors_only || wrong_crc != 0) {
        report.log(level, u"%sCorrupted sections (bad CRC): %'d", {prefix, wrong_crc});
    }
    if (!errors_only || is_next != 0) {
        report.log(level, u"%sNext sections (not yet applicable): %'d", {prefix, is_next});
    }
}


//----------------------------------------------------------------------------
// Analysis context for one TID/TIDext into one PID.
//----------------------------------------------------------------------------

// Init for a new table.
void ts::SectionDemux::ETIDContext::init(uint8_t new_version, uint8_t last_section)
{
    notified = false;
    version = new_version;
    sect_expected = size_t(last_section) + 1;
    sect_received = 0;
    sects.resize(sect_expected);

    // Mark all section entries as unused
    for (size_t i = 0; i < sect_expected; i++) {
        sects[i].clear();
    }
}

// Notify the application if the table is complete.
void ts::SectionDemux::ETIDContext::notify(SectionDemux& demux, bool pack, bool fill_eit)
{
    if (!notified && (sect_received == sect_expected || pack || fill_eit) && demux._table_handler != nullptr) {

        // Build the table
        BinaryTable table;
        for (size_t i = 0; i < sects.size(); ++i) {
            table.addSection(sects[i]);
        }

        // Pack incomplete table with force.
        if (pack) {
            table.packSections();
        }

        // Add missing sections in EIT (if the table is an EIT).
        if (fill_eit) {
            EIT::Fix(table, EIT::ADD_MISSING);
        }

        // Invoke the table handler.
        if (table.isValid()) {
            notified = true;
            demux._table_handler->handleTable(demux, table);
        }
    }
}


//----------------------------------------------------------------------------
// Analysis context for one PID.
// Called when packet synchronization is lost on the pid.
//----------------------------------------------------------------------------

void ts::SectionDemux::PIDContext::syncLost()
{
    sync = false;
    ts.clear();
}


//----------------------------------------------------------------------------
// SectionDemux constructor and destructor.
//----------------------------------------------------------------------------

ts::SectionDemux::SectionDemux(DuckContext& duck, TableHandlerInterface* table_handler, SectionHandlerInterface* section_handler, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter),
    _table_handler(table_handler),
    _section_handler(section_handler)
{
}


//----------------------------------------------------------------------------
// Reset the analysis context (partially built sections and tables).
//----------------------------------------------------------------------------

void ts::SectionDemux::immediateReset()
{
    SuperClass::immediateReset();
    _pids.clear();
}

void ts::SectionDemux::immediateResetPID(PID pid)
{
    SuperClass::immediateResetPID(pid);
    _pids.erase(pid);
}


//----------------------------------------------------------------------------
// Feed the depacketizer with a TS packet.
//----------------------------------------------------------------------------

void ts::SectionDemux::feedPacket(const TSPacket& pkt)
{
    if (_pid_filter[pkt.getPID()]) {
        processPacket(pkt);
    }
    SuperClass::feedPacket(pkt);
}

void ts::SectionDemux::processPacket(const TSPacket& pkt)
{
    // Reject invalid packets
    if (!pkt.hasValidSync()) {
        _status.invalid_ts++;
        return;
    }

    // Get PID and reference to the PID context.
    // The PID context is created if did not exist.
    const PID pid = pkt.getPID();
    PIDContext& pc(_pids[pid]);

    // If TS packet is scrambled, we cannot decode it and we loose synchronization
    // on this PID (usually, PID's carrying sections are not scrambled).
    if (pkt.getScrambling()) {
        _status.scrambled++;
        pc.syncLost();
        return;
    }

    // Check continuity counter on this PID (only if we have not lost
    // the synchronization on this PID).
    if (pc.sync) {
        // Ignore duplicate packets (same CC)
        if (pkt.getCC() == pc.continuity) {
            return;
        }
        // Check if we are still synchronized
        if (pkt.getCC() != ((pc.continuity + 1) & CC_MASK)) {
            _duck.report().log(_ts_error_level, u"demux sync lost on discontinuity, PID 0x%X (%<d), packet index %'d", {pid, _packet_count});
            _status.discontinuities++;
            pc.syncLost();
        }
    }

    pc.continuity = pkt.getCC();

    // Locate TS packet payload
    size_t header_size = pkt.getHeaderSize();
    if (!pkt.hasPayload() || header_size >= PKT_SIZE) {
        return;
    }
    uint8_t pointer_field = 0xFF;
    const uint8_t* payload = nullptr;
    size_t payload_size = 0;

    // Packet index of start of next section to analyze.
    PacketCounter pusi_pkt_index = pc.pusi_pkt_index;

    if (pkt.getPUSI()) {
        // Keep track of last packet containing a PUSI in this PID
        pc.pusi_pkt_index = _packet_count;
        // Payload Unit Start Indicator (PUSI) is set.
        // Filter out PES packets. A PES packet starts with the "start code prefix"
        // 00 00 01. This sequence cannot be found in a TS packet with sections
        // (would be 00 = pointer field, 00 = PAT, 01 = not possible for a PAT).
        if (header_size + 3 <= PKT_SIZE &&
            pkt.b[header_size] == 0x00 &&
            pkt.b[header_size + 1] == 0x00 &&
            pkt.b[header_size + 2] == 0x01)
        {
            // Losing sync, will skip all TS packet until next PUSI
            pc.syncLost();
            return;
        }
        // First byte of payload is a pointer field
        pointer_field = pkt.b[header_size];
        payload = pkt.b + header_size + 1;
        payload_size = PKT_SIZE - header_size - 1;
        // Ignore packet and loose sync if inconsistent pointer field
        if (pointer_field >= payload_size) {
            pc.syncLost();
            return;
        }
        // Adjust packet index of start of next section if there is nothing before it.
        if (pointer_field == 0 && pc.ts.empty()) {
            pusi_pkt_index = _packet_count;
        }
    }
    else {
        // PUSI not set, first byte of payload is section data
        pointer_field = 0xFF;
        payload = pkt.b + header_size;
        payload_size = PKT_SIZE - header_size;
    }

    if (payload_size <= 0) {
        return;
    }

    // If no previous synchronization, skip incomplete sections
    if (!pc.sync) {
        // If no new section in this packet, ignore it
        if (!pkt.getPUSI()) {
            return;
        }
        // Skip end of previous section
        payload += pointer_field;
        payload_size -= pointer_field;
        pointer_field = 0;
        // We have found the beginning of a section, we are now synchronized
        pc.sync = true;
    }

    // Copy TS packet payload in PID context
    pc.ts.append(payload, payload_size);

    // Locate TS buffer by address and size.
    const uint8_t* ts_start = pc.ts.data();
    size_t ts_size = pc.ts.size();

    // If current packet has a PUSI, locate start of this new section inside the TS buffer.
    // This is not useful to locate the section but it is used to check that the previous section was not truncated.
    const uint8_t* pusi_section = nullptr;
    if (pkt.getPUSI()) {
        pusi_section = ts_start + ts_size - payload_size + pointer_field;
    }

    // Loop on all complete sections in the buffer.
    // If there is less than 3 bytes in the buffer, we cannot even determine the section length.
    while (ts_size >= 3) {

        // If start of next area is 0xFF (invalid TID value), the rest of
        // the packet is stuffing. Skip it, unless there is a PUSI later.
        if (ts_start[0] == 0xFF) {
            if (pusi_section != nullptr && ts_start < pusi_section) {
                // We can resync at a PUSI later in the TS buffer.
                ts_size -= (pusi_section - ts_start);
                ts_start = pusi_section;
                continue;
            }
            else {
                // There is no PUSI later, skip the rest of the TS packet.
                ts_size = 0;
                break;
            }
        }

        // Get section header.
        bool section_ok = true;
        const TID tid = ts_start[0];
        ETID etid(tid);
        const bool long_header = Section::StartLongSection(ts_start, ts_size);
        uint16_t section_length = (GetUInt16(ts_start + 1) & 0x0FFF) + SHORT_SECTION_HEADER_SIZE;

        // Lose synchronization when invalid section length.
        if (section_length > MAX_PRIVATE_SECTION_SIZE ||
            section_length < MIN_SHORT_SECTION_SIZE ||
            (long_header && section_length < MIN_LONG_SECTION_SIZE))
        {
            _duck.report().log(_ts_error_level, u"invalid section length: %'d bytes, PID 0x%X (%<d), TID 0x%X (%<d), packet index %'d", {section_length, pid, tid, _packet_count});
            _status.inv_sect_length++;
            if (pusi_section != nullptr && ts_start < pusi_section) {
                // We can resync at a PUSI later in the TS buffer.
                ts_size -= (pusi_section - ts_start);
                ts_start = pusi_section;
                continue;
            }
            else {
                // No way to resync now, wait for next packet with PUSI.
                pc.syncLost();
                return;
            }
        }

        // If we detect that the section is incorrectly truncated, skip it.
        if (pusi_section != nullptr && ts_start < pusi_section && ts_start + section_length > pusi_section) {
            const uint16_t actual_length = uint16_t(pusi_section - ts_start);
            _duck.report().log(_ts_error_level, u"truncated section: %'d bytes instead of %'d, PID 0x%X (%<d), TID 0x%X (%<d), packet index %'d", {actual_length, section_length, pid, tid, _packet_count});
            section_ok = false;
            _status.truncated_sect++;
            // Resynchronize to actual section start
            section_length = actual_length;
        }

        // Exit when end of section is missing. Wait for next TS packets.
        if (ts_size < section_length) {
            break;
        }

        // We have a complete section in the pc.ts buffer. Analyze it.
        uint8_t version = 0;
        bool is_next = false;
        uint8_t section_number = 0;
        uint8_t last_section_number = 0;

        if (section_ok && long_header) {
            etid = ETID (etid.tid(), GetUInt16 (ts_start + 3));
            version = (ts_start[5] >> 1) & 0x1F;
            is_next = (ts_start[5] & 0x01) == 0;
            section_number = ts_start[6];
            last_section_number = ts_start[7];
            // Check that the section number fits in the range
            if (section_number > last_section_number) {
                _duck.report().log(_ts_error_level, u"invalid section index: %d/%d, PID 0x%X (%<d), TID 0x%X (%<d), packet index %'d", {section_number, last_section_number, pid, tid, _packet_count});
                _status.inv_sect_index++;
                section_ok = false;
            }
        }

        // Sections with the 'next' indicator are filtered by options.

        if (is_next && !_get_next) {
            _status.is_next++;
            section_ok = false;
        }
        if (!is_next && !_get_current) {
            section_ok = false;
        }

        if (section_ok) {

            // Get the list of standards which define this table id and add them in context.
            _duck.addStandards(PSIRepository::Instance().getTableStandards(etid.tid(), pid));

            // Get reference to the ETID context for this PID.
            // The ETID context is created if did not exist.
            // Avoid accumulating partial sections when there is no table handler.
            ETIDContext* tc = _table_handler == nullptr ? nullptr : &pc.tids[etid];

            // If this is a new version of the table, reset the TID context.
            // Note that short sections do not have versions, so the version
            // field is implicitely zero. However, every short section must
            // be considered as a new version since there is no way to track versions.

            if (tc != nullptr) {
                if (!long_header ||              // short section
                    tc->sect_expected == 0 ||    // new TID on this PID
                    tc->version != version)      // new version
                {
                    tc->init(version, last_section_number);
                }

                // Check that the total number of sections in the table
                // has not changed since last section.
                if (last_section_number != tc->sect_expected - 1) {
                    _duck.report().log(_ts_error_level, u"inconsistent last section index: %d, was %d, PID 0x%X (%<d), TID 0x%X (%<d), packet index %'d", {last_section_number, tc->sect_expected - 1, pid, tid, _packet_count});
                    _status.inv_sect_index++;
                    section_ok = false;
                }
            }

            // Track invalid section version numbers.
            if (section_ok && _track_invalid_version && long_header && tc != nullptr && !tc->sects[section_number].isNull()) {
                const Section& old(*tc->sects[section_number]);
                // At this point, the version is necessarily identical. If this was another version,
                // ts->init() was called and tc->sects[section_number] is null.
                assert(old.version() == version);
                if (section_length != old.size() || std::memcmp(ts_start, old.content(), section_length) != 0) {
                    _duck.report().log(_ts_error_level, u"section updated without version update, PID 0x%X (%<d), TID 0x%X (%<d), section %d, version %d, packet index %'d", {pid, tid, section_number, version, _packet_count});
                    // Reset the previous content of the section and make sure the table will be notified again.
                    tc->sects[section_number].clear();
                    assert(tc->sect_received > 0);
                    tc->sect_received--;
                    tc->notified = false;
                    _status.inv_sect_version++;
                }
            }

            // Create a new Section object if necessary (ie. if a section
            // hendler is registered or if this is a new section).
            SectionPtr sect_ptr;

            if (section_ok && (_section_handler != nullptr || (tc != nullptr && tc->sects[section_number].isNull()))) {
                sect_ptr = new Section(ts_start, section_length, pid, CRC32::CHECK);
                sect_ptr->setFirstTSPacketIndex(pusi_pkt_index);
                sect_ptr->setLastTSPacketIndex(_packet_count);
                if (!sect_ptr->isValid()) {
                    _duck.report().log(_ts_error_level, u"invalid section CRC, PID 0x%X (%<d), TID 0x%X (%<d), section %d, version %d, packet index %'d", {pid, tid, section_number, version, _packet_count});
                    _status.wrong_crc++;  // only possible error (hum?)
                    section_ok = false;
                }
            }

            // Mark that we are in the context of a table or section handler.
            // This is used to prevent the destruction of PID contexts during
            // the execution of a handler.
            beforeCallingHandler(pid);
            try {
                // If a handler is defined for sections, invoke it.
                if (section_ok && _section_handler != nullptr) {
                    _section_handler->handleSection(*this, *sect_ptr);
                }

                // Save the section in the TID context if this is a new one.
                if (section_ok && tc != nullptr && tc->sects[section_number].isNull()) {

                    // Save the section
                    tc->sects[section_number] = sect_ptr;
                    tc->sect_received++;

                    // If the table is completed and a handler is present, build the table.
                    tc->notify(*this, false, false);
                }
            }
            catch (...) {
                afterCallingHandler(false);
                throw;
            }
            if (afterCallingHandler(true)) {
                return;  // the PID of this packet or the complete demux was reset.
            }
        }

        // If a handler is defined for invalid sections, call it.
        if (!section_ok && _invalid_handler != nullptr) {
            beforeCallingHandler(pid);
            try {
                DemuxedData data(ts_start, section_length, pid);
                data.setFirstTSPacketIndex(pusi_pkt_index);
                data.setLastTSPacketIndex(_packet_count);
                _invalid_handler->handleInvalidSection(*this, data);
            }
            catch (...) {
                afterCallingHandler(false);
                throw;
            }
            if (afterCallingHandler(true)) {
                return;  // the PID of this packet or the complete demux was reset.
            }
        }

        // Move to next section in the buffer
        ts_start += section_length;
        ts_size -= section_length;

        // The next section necessarily starts in current packet
        pusi_pkt_index = _packet_count;
    }

    // If an incomplete section remains in the buffer, move it back to the start of the buffer.
    if (ts_size <= 0) {
        // TS buffer becomes empty
        pc.ts.clear();
    }
    else if (ts_start > pc.ts.data()) {
        // Remove start of TS buffer
        pc.ts.erase(0, ts_start - pc.ts.data());
    }
}


//----------------------------------------------------------------------------
// Fix incomplete tables and notify these rebuilt tables.
//----------------------------------------------------------------------------

void ts::SectionDemux::fixAndFlush(bool pack, bool fill_eit)
{
    // Loop on all PID's.
    for (auto& it1 : _pids) {
        const PID pid = it1.first;
        PIDContext& pc(it1.second);

        // Mark that we are in the context of a table or section handler.
        // This is used to prevent the destruction of PID contexts during
        // the execution of a handler.
        beforeCallingHandler(pid);
        try {
            // Loop on all TID's currently found in the PID.
            for (auto& it2 : pc.tids) {
                // Force a notification of the partial table, if any.
                it2.second.notify(*this, pack, fill_eit);
            }
        }
        catch (...) {
            afterCallingHandler(false);
            throw;
        }
        afterCallingHandler(true);
    }
}
