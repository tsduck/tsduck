//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCRAnalyzer.h"
#include "tsMemory.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::PCRAnalyzer::PCRAnalyzer(size_t min_pid, size_t min_pcr) :
    _min_pid(std::max<size_t>(1, min_pid)),
    _min_pcr(std::max<size_t>(1, min_pcr))
{
    TS_ZERO(_pid);
}

ts::PCRAnalyzer::~PCRAnalyzer()
{
    reset();
}


//----------------------------------------------------------------------------
// PCRAnalyzez::Status constructors
//----------------------------------------------------------------------------

ts::PCRAnalyzer::Status::Status(const PCRAnalyzer& an) : Status()
{
    an.getStatus(*this);
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface for PCRAnalyzez::Status.
//----------------------------------------------------------------------------

ts::UString ts::PCRAnalyzer::Status::toString() const
{
    return UString::Format(u"valid: %s, bitrate: %'d b/s, packets: %'d, PCRs: %'d, PIDs with PCR: %'d, discont: %'d, instantaneous bitrate: %'d b/s",
                           {bitrate_valid, bitrate_188, packet_count, pcr_count, pcr_pids, discontinuities, instantaneous_bitrate_188});
}


//----------------------------------------------------------------------------
// Reset all collected information
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::reset(size_t min_pid, size_t min_pcr)
{
    _min_pid = std::max<size_t>(1, min_pid);
    _min_pcr = std::max<size_t>(1, min_pcr);
    reset();
}


//----------------------------------------------------------------------------
// Reset all collected information
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::reset()
{
    _bitrate_valid = false;
    _ts_pkt_cnt = 0;
    _ts_bitrate_188 = 0;
    _ts_bitrate_204 = 0;
    _ts_bitrate_cnt = 0;
    _completed_pids = 0;
    _pcr_pids = 0;
    _inst_ts_bitrate_188 = 0;
    _inst_ts_bitrate_204 = 0;

    for (size_t i = 0; i < PID_MAX; ++i) {
        if (_pid[i] != nullptr) {
            delete _pid[i];
            _pid[i] = nullptr;
        }
    }

    _packet_pcr_index_map.clear();
}


//----------------------------------------------------------------------------
// Reset all collected information and use DTS instead of PCR from now on.
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::resetAndUseDTS()
{
    reset();
    _use_dts = true;
}

void ts::PCRAnalyzer::resetAndUseDTS(size_t min_pid, size_t min_dts)
{
    reset(min_pid, min_dts);
    _use_dts = true;
}

void ts::PCRAnalyzer::setIgnoreErrors(bool ignore)
{
    _ignore_errors = ignore;
}


//----------------------------------------------------------------------------
// Process a discontinuity in the transport stream
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::processDiscontinuity()
{
    _discontinuities++;

    // All collected PCR's become invalid since at least one packet is missing.
    for (size_t i = 0; i < PID_MAX; ++i) {
        if (_pid[i] != nullptr) {
            _pid[i]->last_pcr_value = INVALID_PCR;
        }
    }
    _packet_pcr_index_map.clear();
}


//----------------------------------------------------------------------------
// Return the evaluated TS bitrate in bits/second
// (based on 188-byte or 204-byte packets).
//----------------------------------------------------------------------------

ts::BitRate ts::PCRAnalyzer::bitrate188() const
{
    return _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_188 / _ts_bitrate_cnt);
}

ts::BitRate ts::PCRAnalyzer::bitrate204() const
{
    return _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_204 / _ts_bitrate_cnt);
}

ts::BitRate ts::PCRAnalyzer::instantaneousBitrate188() const
{
    return BitRate(_inst_ts_bitrate_188);
}

ts::BitRate ts::PCRAnalyzer::instantaneousBitrate204() const
{
    return BitRate(_inst_ts_bitrate_204);
}


//----------------------------------------------------------------------------
// Return the evaluated PID bitrate in bits/second
// (based on 188-byte or 204-byte packets).
//----------------------------------------------------------------------------

ts::BitRate ts::PCRAnalyzer::bitrate188(PID pid) const
{
    return (pid >= PID_MAX || _ts_bitrate_cnt == 0 || _ts_pkt_cnt == 0 || _pid[pid] == nullptr) ? 0 :
        BitRate((_ts_bitrate_188 * _pid[pid]->ts_pkt_cnt) / (_ts_bitrate_cnt * _ts_pkt_cnt));
}

ts::BitRate ts::PCRAnalyzer::bitrate204(PID pid) const
{
    return (pid >= PID_MAX || _ts_bitrate_cnt == 0 || _ts_pkt_cnt == 0 || _pid[pid] == nullptr) ? 0 :
        BitRate((_ts_bitrate_204 * _pid[pid]->ts_pkt_cnt) / (_ts_bitrate_cnt * _ts_pkt_cnt));
}


//----------------------------------------------------------------------------
// Return the number of TS packets on a PID
//----------------------------------------------------------------------------

ts::PacketCounter ts::PCRAnalyzer::packetCount(PID pid) const
{
    return (pid >= PID_MAX || _pid[pid] == nullptr) ? 0 : _pid[pid]->ts_pkt_cnt;
}


//----------------------------------------------------------------------------
// Return all global results at once.
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::getStatus(Status& stat) const
{
    stat.bitrate_valid = _bitrate_valid;
    stat.bitrate_188 = bitrate188();
    stat.bitrate_204 = bitrate204();
    stat.packet_count = _ts_pkt_cnt;
    stat.pcr_count = _ts_bitrate_cnt;
    stat.pcr_pids = _pcr_pids;
    stat.discontinuities = _discontinuities;
    stat.instantaneous_bitrate_188 = instantaneousBitrate188();
    stat.instantaneous_bitrate_204 = instantaneousBitrate204();
}


//----------------------------------------------------------------------------
// Feed the PCR analyzer with a new transport packet.
// Return true if we have collected enough packet to evaluate TS bitrate.
//----------------------------------------------------------------------------

bool ts::PCRAnalyzer::feedPacket(const TSPacket& pkt)
{
    // Count one more packet in the TS
    _ts_pkt_cnt++;

    // Reject invalid packets, suspected TS corruption
    if (!_ignore_errors && !pkt.hasValidSync()) {
        processDiscontinuity();
        return _bitrate_valid;
    }

    // Find PID context
    const PID pid = pkt.getPID();
    assert(pid < PID_MAX);

    PIDAnalysis* ps = _pid[pid];
    if (ps == nullptr) {
        ps = _pid[pid] = new PIDAnalysis;
    }

    // Count one more packet in the PID
    ps->ts_pkt_cnt++;

    // Null packets are ignored in PCR calculation (except for increment of _ts_pkt_cnt/ts_pkt_cnt).
    if (pid == PID_NULL) {
        return _bitrate_valid;
    }

    // Process discontinuities. If a discontinuity is discovered,
    // the PCR calculation across this packet is not valid.
    if (!_ignore_errors) {
        bool broken_rate = false;
        uint8_t continuity_cnt = pkt.getCC();

        if (ps->ts_pkt_cnt == 1) {
            // First packet on this PID, initialize continuity
            ps->cur_continuity = continuity_cnt;
        }
        else if (pkt.getDiscontinuityIndicator()) {
            // Expected discontinuity
            broken_rate = true;
        }
        else if (pkt.hasPayload()) {
            // Packet has payload. Compute next continuity counter.
            uint8_t next_cont((ps->cur_continuity + 1) & 0x0F);
            // The countinuity counter must be either identical to previous one
            // (duplicated packet) or adjacent.
            broken_rate = continuity_cnt != ps->cur_continuity && continuity_cnt != next_cont;
        }
        else if (continuity_cnt != ps->cur_continuity) {
            // Packet has no payload -> should have same counter
            broken_rate = continuity_cnt != ps->cur_continuity;
        }
        ps->cur_continuity = continuity_cnt;

        // In case of suspected packet loss, reset calculations
        if (broken_rate) {
            processDiscontinuity();
        }
    }

    // Process PCR (or DTS)
    if ((_use_dts && pkt.hasDTS()) || (!_use_dts && pkt.hasPCR())) {

        // Get PCR value (or DTS)
        const uint64_t pcr_dts = _use_dts ? pkt.getDTS() : pkt.getPCR();

        // If last PCR/DTS valid, compute transport rate between the two
        if (ps->last_pcr_value != INVALID_PCR && ps->last_pcr_value != pcr_dts) {

            // Compute transport rate in b/s since last PCR/DTS
            uint64_t diff_values = _use_dts ?
                DiffPTS(ps->last_pcr_value, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
                DiffPCR(ps->last_pcr_value, pcr_dts);

            BitRate ts_bitrate_188 = diff_values == 0 ? 0 :
                BitRate((_ts_pkt_cnt - ps->last_pcr_packet) * SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS) / diff_values;
            BitRate ts_bitrate_204 = diff_values == 0 ? 0 :
                BitRate((_ts_pkt_cnt - ps->last_pcr_packet) * SYSTEM_CLOCK_FREQ * PKT_RS_SIZE_BITS) / diff_values;

            // Clear out values older than 1 second from _packet_pcr_index_map.
            // Note that this is a map that covers PCR/DTS packets across all PIDs
            // as long as the clocks used to generate the PCR/DTS values for different
            // programs is the same clock, there should be no issue, but if the PCR/DTS values
            // across the two programs are wildly different, then the following approach won't work.
            while (!_packet_pcr_index_map.empty()) {
                const uint64_t earliestPCR_DTS = _packet_pcr_index_map.begin()->first;
                diff_values = _use_dts ?
                    DiffPTS(earliestPCR_DTS, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
                    DiffPCR(earliestPCR_DTS, pcr_dts);
                if (diff_values > SYSTEM_CLOCK_FREQ) {
                    _packet_pcr_index_map.erase(_packet_pcr_index_map.begin());
                }
                else {
                    break;
                }
            }

            // Per-PID statistics:
            ps->ts_bitrate_188 += ts_bitrate_188;
            ps->ts_bitrate_204 += ts_bitrate_204;
            ps->ts_bitrate_cnt++;
            if (ps->ts_bitrate_cnt == 1) {
                // First PCR result on this PID
                _pcr_pids++;
            }

            // Transport stream statistics:
            _ts_bitrate_188 += ts_bitrate_188;
            _ts_bitrate_204 += ts_bitrate_204;
            _ts_bitrate_cnt++;

            // Transport stream instantaneous statistics.
            // For instantaneous bit rates, these are the actual bit rates, and it doesn't use the "count" approach.
            if (!_packet_pcr_index_map.empty()) {
                diff_values = _use_dts ?
                    DiffPTS(_packet_pcr_index_map.begin()->first, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
                    DiffPCR(_packet_pcr_index_map.begin()->first, pcr_dts);
                _inst_ts_bitrate_188 = diff_values == 0 ? 0 :
                    BitRate((_ts_pkt_cnt - _packet_pcr_index_map.begin()->second) * SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS) / diff_values;
                _inst_ts_bitrate_204 = diff_values == 0 ? 0 :
                    BitRate((_ts_pkt_cnt - _packet_pcr_index_map.begin()->second) * SYSTEM_CLOCK_FREQ * PKT_RS_SIZE_BITS) / diff_values;
            }

            // Check if we got enough values for this PID
            if (ps->ts_bitrate_cnt == _min_pcr) {
                _completed_pids++;
                _bitrate_valid = _completed_pids >= _min_pid;
            }
        }

        // Save PCR/DTS for next calculation, ignore duplicated values.
        if (ps->last_pcr_value != pcr_dts) {
            ps->last_pcr_value = pcr_dts;
            ps->last_pcr_packet = _ts_pkt_cnt;

            // Also add PCR (or DTS)/packet index combo to map for use in instantaneous bit rate calculations.
            _packet_pcr_index_map[pcr_dts] = _ts_pkt_cnt;

            // Make sure that some crazy TS does not accumulate thousands of PCR values in the same second range.
            while (_packet_pcr_index_map.size() > FOOLPROOF_MAP_LIMIT) {
                // Erase older entries.
                _packet_pcr_index_map.erase(_packet_pcr_index_map.begin());
            }
        }
    }

    return _bitrate_valid;
}
