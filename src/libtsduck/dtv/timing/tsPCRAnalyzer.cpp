//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCRAnalyzer.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::PCRAnalyzer::PCRAnalyzer(size_t min_pid, size_t min_pcr) :
    _min_pid(std::max<size_t>(1, min_pid)),
    _min_values(std::max<size_t>(1, min_pcr))
{
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
                           bitrate_valid, bitrate_188, packet_count, clock_count, clock_pids, discontinuities, instantaneous_bitrate_188);
}


//----------------------------------------------------------------------------
// Reset all collected information
//----------------------------------------------------------------------------

void ts::PCRAnalyzer::reset(size_t min_pid, size_t min_pcr)
{
    _min_pid = std::max<size_t>(1, min_pid);
    _min_values = std::max<size_t>(1, min_pcr);
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
    _clock_pids_count = 0;
    _inst_ts_bitrate_188 = 0;
    _inst_ts_bitrate_204 = 0;
    _duration = PCR::zero();
    _pids.clear();
    _packet_clock_index_map.clear();
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
    // All collected PCR's become invalid since at least one packet is missing.
    for (auto& it : _pids) {
        it.second.last_is_valid = false;
    }
    _packet_clock_index_map.clear();
    _discontinuities++;
}


//----------------------------------------------------------------------------
// Return the evaluated TS bitrate in bits/second
//----------------------------------------------------------------------------

ts::BitRate ts::PCRAnalyzer::bitrate188() const
{
    return _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_188 / _ts_bitrate_cnt);
}

ts::BitRate ts::PCRAnalyzer::bitrate204() const
{
    return _ts_bitrate_cnt == 0 ? 0 : BitRate(_ts_bitrate_204 / _ts_bitrate_cnt);
}


//----------------------------------------------------------------------------
// Return the evaluated PID bitrate in bits/second
//----------------------------------------------------------------------------

ts::BitRate ts::PCRAnalyzer::bitrate188(PID pid) const
{
    const auto it = _pids.find(pid);
    return (_ts_bitrate_cnt == 0 || _ts_pkt_cnt == 0 || it == _pids.end()) ? 0 :
        BitRate((_ts_bitrate_188 * it->second.ts_pkt_cnt) / (_ts_bitrate_cnt * _ts_pkt_cnt));
}

ts::BitRate ts::PCRAnalyzer::bitrate204(PID pid) const
{
    const auto it = _pids.find(pid);
    return (_ts_bitrate_cnt == 0 || _ts_pkt_cnt == 0 || it == _pids.end()) ? 0 :
        BitRate((_ts_bitrate_204 * it->second.ts_pkt_cnt) / (_ts_bitrate_cnt * _ts_pkt_cnt));
}


//----------------------------------------------------------------------------
// Get the estimated playout duration in PCR units, based on a given PID.
//----------------------------------------------------------------------------

ts::PCR ts::PCRAnalyzer::duration(PID pid) const
{
    const auto it = _pids.find(pid);
    return it == _pids.end() ? PCR::zero() : it->second.duration;
}


//----------------------------------------------------------------------------
// Return the number of TS packets on a PID
//----------------------------------------------------------------------------

ts::PacketCounter ts::PCRAnalyzer::packetCount(PID pid) const
{
    const auto it = _pids.find(pid);
    return it == _pids.end() ? 0 : it->second.ts_pkt_cnt;
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
    stat.clock_count = _ts_bitrate_cnt;
    stat.clock_pids = _clock_pids_count;
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
    PIDAnalysis& ps(_pids[pid]);

    // Count one more packet in the PID
    ps.ts_pkt_cnt++;

    // Null packets are ignored in PCR calculation (except for increment of _ts_pkt_cnt/ts_pkt_cnt).
    if (pid == PID_NULL) {
        return _bitrate_valid;
    }

    // Process discontinuities. If a discontinuity is discovered, the PCR calculation across this packet is not valid.
    if (!_ignore_errors) {
        bool broken_rate = false;
        uint8_t continuity_cnt = pkt.getCC();

        if (ps.ts_pkt_cnt == 1) {
            // First packet on this PID, initialize continuity
            ps.cur_continuity = continuity_cnt;
        }
        else if (pkt.getDiscontinuityIndicator()) {
            // Expected discontinuity
            broken_rate = true;
        }
        else if (pkt.hasPayload()) {
            // Packet has payload. Compute next continuity counter.
            uint8_t next_cont((ps.cur_continuity + 1) & 0x0F);
            // The countinuity counter must be either identical to previous one (duplicated packet) or adjacent.
            broken_rate = continuity_cnt != ps.cur_continuity && continuity_cnt != next_cont;
        }
        else if (continuity_cnt != ps.cur_continuity) {
            // Packet has no payload -> should have same counter
            broken_rate = continuity_cnt != ps.cur_continuity;
        }
        ps.cur_continuity = continuity_cnt;

        // In case of suspected packet loss, reset calculations
        if (broken_rate) {
            processDiscontinuity();
        }
    }

    // Process PCR (or DTS)
    if ((_use_dts && pkt.hasDTS()) || (!_use_dts && pkt.hasPCR())) {

        // Get PCR value (or DTS)
        const uint64_t pcr_dts = _use_dts ? pkt.getDTS() : pkt.getPCR();

        // Get increment since last value, in PCR units.
        const uint64_t pcr_increment = ps.last_pcr_dts_value == INVALID_PCR ? 0 : (_use_dts ?
            DiffPTS(ps.last_pcr_dts_value, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
            DiffPCR(ps.last_pcr_dts_value, pcr_dts));

        // Adjust the clock of the current PID (accumulated PCR ticks).
        // Here, we do not care about discontinuities, we just use clock values.
        if (pcr_increment > 0) {
            if (pcr_increment < SYSTEM_CLOCK_FREQ) {
                // Less than one second  since last PCR, seems reasonable.
                ps.duration += PCR(pcr_increment);
            }
            else {
                // More than one second since last PCR. Looks suspicious. Maybe a clock reference change.
                // Evaluate what the next PCR should be, using instantaneous bitrate.
                const BitRate bitrate = instantaneousBitrate188();
                if (bitrate > 0) {
                    ps.duration += PCR((BitRate((_ts_pkt_cnt - ps.last_pcr_dts_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt());
                }
            }
            // Adjust the global clock. If is adjusted on each PCR/DTS, always incrementing.
            _duration = std::max(_duration + PCR(1), ps.duration);
        }

        // If last PCR/DTS valid, compute transport rate between the two
        if (ps.last_is_valid && ps.last_pcr_dts_value != INVALID_PCR && ps.last_pcr_dts_value != pcr_dts) {

            // Compute transport rate in b/s since last PCR/DTS
            uint64_t diff_values = pcr_increment;
            BitRate ts_bitrate_188 = diff_values == 0 ? 0 :
                BitRate((_ts_pkt_cnt - ps.last_pcr_dts_packet) * SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS) / diff_values;
            BitRate ts_bitrate_204 = diff_values == 0 ? 0 :
                BitRate((_ts_pkt_cnt - ps.last_pcr_dts_packet) * SYSTEM_CLOCK_FREQ * PKT_RS_SIZE_BITS) / diff_values;

            // Clear out values older than 1 second from _packet_pcr_index_map.
            // Note that this is a map that covers PCR/DTS packets across all PIDs
            // as long as the clocks used to generate the PCR/DTS values for different
            // programs is the same clock, there should be no issue, but if the PCR/DTS values
            // across the two programs are wildly different, then the following approach won't work.
            while (!_packet_clock_index_map.empty()) {
                const uint64_t earliestPCR_DTS = _packet_clock_index_map.begin()->first;
                diff_values = _use_dts ?
                    DiffPTS(earliestPCR_DTS, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
                    DiffPCR(earliestPCR_DTS, pcr_dts);
                if (diff_values > SYSTEM_CLOCK_FREQ) {
                    _packet_clock_index_map.erase(_packet_clock_index_map.begin());
                }
                else {
                    break;
                }
            }

            // Per-PID statistics:
            ps.ts_bitrate_188 += ts_bitrate_188;
            ps.ts_bitrate_204 += ts_bitrate_204;
            ps.ts_bitrate_cnt++;
            if (ps.ts_bitrate_cnt == 1) {
                // First PCR result on this PID
                _clock_pids_count++;
            }

            // Transport stream statistics:
            _ts_bitrate_188 += ts_bitrate_188;
            _ts_bitrate_204 += ts_bitrate_204;
            _ts_bitrate_cnt++;

            // Transport stream instantaneous statistics.
            // For instantaneous bitrates, these are the actual bitrates, and it doesn't use the "count" approach.
            if (!_packet_clock_index_map.empty()) {
                diff_values = _use_dts ?
                    DiffPTS(_packet_clock_index_map.begin()->first, pcr_dts) * SYSTEM_CLOCK_SUBFACTOR :
                    DiffPCR(_packet_clock_index_map.begin()->first, pcr_dts);
                _inst_ts_bitrate_188 = diff_values == 0 ? 0 :
                    BitRate((_ts_pkt_cnt - _packet_clock_index_map.begin()->second) * SYSTEM_CLOCK_FREQ * PKT_SIZE_BITS) / diff_values;
                _inst_ts_bitrate_204 = diff_values == 0 ? 0 :
                    BitRate((_ts_pkt_cnt - _packet_clock_index_map.begin()->second) * SYSTEM_CLOCK_FREQ * PKT_RS_SIZE_BITS) / diff_values;
            }

            // Check if we got enough values for this PID
            if (ps.ts_bitrate_cnt == _min_values) {
                _completed_pids++;
                _bitrate_valid = _completed_pids >= _min_pid;
            }
        }

        // Save PCR/DTS for next calculation, ignore duplicated values.
        if (ps.last_pcr_dts_value != pcr_dts) {
            ps.last_pcr_dts_value = pcr_dts;
            ps.last_pcr_dts_packet = _ts_pkt_cnt;
            ps.last_is_valid = true;

            // Also add PCR (or DTS)/packet index combo to map for use in instantaneous bitrate calculations.
            _packet_clock_index_map[pcr_dts] = _ts_pkt_cnt;

            // Make sure that some crazy TS does not accumulate thousands of PCR values in the same second range.
            while (_packet_clock_index_map.size() > FOOLPROOF_MAP_LIMIT) {
                // Erase older entries.
                _packet_clock_index_map.erase(_packet_clock_index_map.begin());
            }
        }
    }

    return _bitrate_valid;
}
