//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  PCR statistics analysis
//
//----------------------------------------------------------------------------

#include "tsPCRAnalyzer.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
// Specify the criteria for valid bitrate analysis:
// Minimum number of PID's, each with a minimum number of PCR"s.
//----------------------------------------------------------------------------

ts::PCRAnalyzer::PCRAnalyzer(size_t min_pid, size_t min_pcr) :
    _use_dts(false),
    _ignore_errors(false),
    _min_pid(std::max<size_t>(1, min_pid)),
    _min_pcr(std::max<size_t>(1, min_pcr)),
    _bitrate_valid(false),
    _ts_pkt_cnt(0),
    _ts_bitrate_188(0),
    _ts_bitrate_204(0),
    _ts_bitrate_cnt(0),
    _completed_pids(0),
    _pcr_pids(0)
{
    TS_ZERO(_pid);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::PCRAnalyzer::~PCRAnalyzer()
{
    reset();
}


//----------------------------------------------------------------------------
// PIDAnalysis constructor
//----------------------------------------------------------------------------

ts::PCRAnalyzer::PIDAnalysis::PIDAnalysis() :
    ts_pkt_cnt(0),
    cur_continuity(0),
    last_pcr_value(0),
    last_pcr_packet(0),
    ts_bitrate_188(0),
    ts_bitrate_204(0),
    ts_bitrate_cnt(0)
{
}


//----------------------------------------------------------------------------
// PCRAnalyzez::Status constructors
//----------------------------------------------------------------------------

ts::PCRAnalyzer::Status::Status() :
    bitrate_valid(false),
    bitrate_188(0),
    bitrate_204(0),
    packet_count(0),
    pcr_count(0),
    pcr_pids(0)
{
}

ts::PCRAnalyzer::Status::Status(const PCRAnalyzer& an) :
    bitrate_valid(false),
    bitrate_188(0),
    bitrate_204(0),
    packet_count(0),
    pcr_count(0),
    pcr_pids(0)
{
    an.getStatus(*this);
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

    for (size_t i = 0; i < PID_MAX; ++i) {
        if (_pid[i] != nullptr) {
            delete _pid[i];
            _pid[i] = nullptr;
        }
    }
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

void ts::PCRAnalyzer::processDiscountinuity()
{
    // All collected PCR becomes invalid since at least one packet is missing.
    for (size_t i = 0; i < PID_MAX; ++i) {
        if (_pid[i] != nullptr) {
            _pid[i]->last_pcr_value = 0;
        }
    }
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
    stat.bitrate_188   = bitrate188();
    stat.bitrate_204   = bitrate204();
    stat.packet_count  = _ts_pkt_cnt;
    stat.pcr_count     = _ts_bitrate_cnt;
    stat.pcr_pids      = _pcr_pids;
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
        processDiscountinuity();
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
            processDiscountinuity();
        }
    }

    // Process PCR (or DTS)
    if ((_use_dts && pkt.hasDTS()) || (!_use_dts && pkt.hasPCR())) {

        // Get PCR value (or converted DTS)
        const uint64_t pcr = _use_dts ? pkt.getDTS() * SYSTEM_CLOCK_SUBFACTOR : pkt.getPCR();

        // If last PCR valid, compute transport rate between the two
        if (ps->last_pcr_value != 0 && ps->last_pcr_value < pcr) {

            // Compute transport rate in b/s since last PCR
            uint64_t ts_bitrate_188 =
                ((_ts_pkt_cnt - ps->last_pcr_packet) * SYSTEM_CLOCK_FREQ * PKT_SIZE * 8) /
                (pcr - ps->last_pcr_value);
            uint64_t ts_bitrate_204 =
                ((_ts_pkt_cnt - ps->last_pcr_packet) * SYSTEM_CLOCK_FREQ * PKT_RS_SIZE * 8) /
                (pcr - ps->last_pcr_value);

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

            // Check if we got enough values for this PID
            if (ps->ts_bitrate_cnt == _min_pcr) {
                _completed_pids++;
                _bitrate_valid = _completed_pids >= _min_pid;
            }
        }

        // Save PCR for next calculation, ignore duplicated PCR values.
        if (ps->last_pcr_value != pcr) {
            ps->last_pcr_value = pcr;
            ps->last_pcr_packet = _ts_pkt_cnt;
        }
    }

    return _bitrate_valid;
}
