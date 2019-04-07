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

#include "tsContinuityAnalyzer.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::ContinuityAnalyzer::ContinuityAnalyzer(const PIDSet& pid_filter, Report* report) :
    _report(report != nullptr ? report : NullReport::Instance()),
    _severity(Severity::Info),
    _display_errors(false),
    _fix_errors(false),
    _generator(false),
    _prefix(),
    _total_packets(0),
    _processed_packets(0),
    _fix_count(0),
    _error_count(0),
    _pid_filter(pid_filter),
    _pid_states()
{
}

ts::ContinuityAnalyzer::PIDState::PIDState() :
    first_cc(INVALID_CC),
    last_cc_in(INVALID_CC),
    last_cc_out(INVALID_CC),
    dup_count(0)
{
}


//----------------------------------------------------------------------------
// Change the output device to report errors.
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::setReport(Report* report)
{
    _report = report != nullptr ? report : NullReport::Instance();
}


//----------------------------------------------------------------------------
// Reset all collected information
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::reset()
{
    _total_packets = 0;
    _processed_packets = 0;
    _fix_count = 0;
    _error_count = 0;
    _pid_states.clear();
}


//----------------------------------------------------------------------------
// PID filter management.
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::setPIDFilter(const PIDSet& pids)
{
    // Get list of removed PID's
    const PIDSet removed_pids(_pid_filter & ~pids);

    // Set the new filter
    _pid_filter = pids;

    // Reset context of all removed PID's
    if (removed_pids.any()) {
        for (PID pid = 0; pid < PID_MAX; ++pid) {
            if (removed_pids[pid]) {
                _pid_states.erase(pid);
            }
        }
    }
}

void ts::ContinuityAnalyzer::addPID(PID pid)
{
    if (pid < _pid_filter.size()) {
        _pid_filter.set(pid);
    }
}

void ts::ContinuityAnalyzer::addPIDs(const PIDSet& pids)
{
    _pid_filter |= pids;
}

void ts::ContinuityAnalyzer::removePID(PID pid)
{
    if (pid < _pid_filter.size() && _pid_filter[pid]) {
        _pid_filter.reset(pid);
        _pid_states.erase(pid);
    }
}

bool ts::ContinuityAnalyzer::hasPID(PID pid) const
{
    return pid < _pid_filter.size() && _pid_filter.test(pid);
}


//----------------------------------------------------------------------------
// Get the first and last CC in a PID.
//----------------------------------------------------------------------------

uint8_t ts::ContinuityAnalyzer::firstCC(PID pid) const
{
    auto it = _pid_states.find(pid);
    return it == _pid_states.end() ? INVALID_CC : it->second.first_cc;
}

uint8_t ts::ContinuityAnalyzer::lastCC(PID pid) const
{
    auto it = _pid_states.find(pid);
    return it == _pid_states.end() ? INVALID_CC : it->second.last_cc_out;
}


//----------------------------------------------------------------------------
//  Return the number of missing packets between two continuity counters
//----------------------------------------------------------------------------

int ts::ContinuityAnalyzer::MissingPackets(int cc1, int cc2)
{
    return (cc2 <= cc1 ? 16 : 0) + cc2 - cc1 - 1;
}


//----------------------------------------------------------------------------
// Detect error on packet.
//----------------------------------------------------------------------------

bool ts::ContinuityAnalyzer::feedPacketInternal(TSPacket* pkt, bool update)
{
    assert(pkt != nullptr);
    const PID pid = pkt->getPID();
    bool result = true;

    // The null PID is never eligible for CC processing.
    if (pid != PID_NULL && _pid_filter.test(pid)) {

        // Get or create PID context.
        PIDState& state(_pid_states[pid]);

        const uint8_t cc = pkt->getCC();
        const bool has_payload = pkt->hasPayload();

        if (state.first_cc == INVALID_CC) {
            // First packet on this PID
            state.first_cc = cc;
        }
        else if (_generator) {
            // Generator mode, ignore input CC, generate a smooth stream.
            if (update) {
                pkt->clearDiscontinuityIndicator();
                pkt->setCC(has_payload ? ((state.last_cc_out + 1) & CC_MASK) : state.last_cc_out);
                _fix_count++;
                result = false;
            }
        }
        else if (pkt->getDiscontinuityIndicator()) {
            // Discontinuity indicator is set, ignore any discontinuity.
            state.dup_count = 0;
        }
        else if (cc == state.last_cc_in && pkt->hasPayload()) {
            // Duplicate packet.
            if (++state.dup_count >= 2) {
                // The standard allows at most 2 duplicate packets.
                if (_display_errors) {
                    _report->log(_severity, u"%spacket: %'d, PID: 0x%04X, %d duplicate packets", {_prefix, _total_packets, pid, state.dup_count + 1});
                }
                // There is nothing we can do to fix this.
                _error_count++;
                result = false;
            }
            if (update && cc != state.last_cc_out && _fix_errors) {
                // Replicate a duplicate.
                pkt->setCC(state.last_cc_out);
                result = false;
                _fix_count++;
            }
        }
        else {
            // Compute expected CC for this packet.
            const uint8_t good_cc_in = has_payload ? ((state.last_cc_in + 1) & CC_MASK) : state.last_cc_in;
            const uint8_t good_cc_out = has_payload ? ((state.last_cc_out + 1) & CC_MASK) : state.last_cc_out;

            if (cc != good_cc_in) {
                if (_display_errors) {
                    _report->log(_severity, u"%spacket: %'d, PID: 0x%04X, missing: %2d packets", {_prefix, _total_packets, pid, MissingPackets(state.last_cc_in, cc)});
                }
                _error_count++;
                result = false;
            }
            if (update && cc != good_cc_out && _fix_errors) {
                pkt->setCC(good_cc_out);
                result = false;
                _fix_count++;
            }
            state.dup_count = 0;
        }

        // Save actual CC for next time.
        state.last_cc_in = cc;
        state.last_cc_out = pkt->getCC();
        _processed_packets++;
    }

    // Count total packets.
    _total_packets++;
    return result;
}
