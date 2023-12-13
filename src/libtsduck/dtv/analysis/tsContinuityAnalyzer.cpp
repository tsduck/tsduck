//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsContinuityAnalyzer.h"
#include "tsjsonObject.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::ContinuityAnalyzer::ContinuityAnalyzer(const PIDSet& pid_filter, Report* report) :
    _report(report != nullptr ? report : &NULLREP),
    _pid_filter(pid_filter)
{
}


//----------------------------------------------------------------------------
// Change the output device to report errors.
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::setReport(Report* report)
{
    _report = report != nullptr ? report : &NULLREP;
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
// PIDState access
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

size_t ts::ContinuityAnalyzer::dupCount(PID pid) const
{
    auto it = _pid_states.find(pid);
    return it == _pid_states.end() ? NPOS : it->second.dup_count;
}

void ts::ContinuityAnalyzer::getLastPacket(PID pid, TSPacket& packet) const
{
    auto it = _pid_states.find(pid);
    packet = it == _pid_states.end() ? NullPacket : it->second.last_pkt_in;
}

ts::TSPacket ts::ContinuityAnalyzer::lastPacket(PID pid) const
{
    TSPacket pkt;
    getLastPacket(pid, pkt);
    return pkt;
}


//----------------------------------------------------------------------------
//  Return the number of missing packets between two continuity counters
//----------------------------------------------------------------------------

size_t ts::ContinuityAnalyzer::MissingPackets(int cc1, int cc2)
{
    cc1 &= CC_MASK;
    cc2 &= CC_MASK;
    return (cc2 <= cc1 ? 16 : 0) + cc2 - cc1 - 1;
}


//----------------------------------------------------------------------------
// Build the first part of an error message.
//----------------------------------------------------------------------------

ts::UString ts::ContinuityAnalyzer::linePrefix(PID pid) const
{
    return UString::Format(u"%spacket index: %'d, PID: 0x%04X", {_prefix, _total_packets, pid});
}


//----------------------------------------------------------------------------
// Log a JSON message.
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::logJSON(PID pid, const UChar* type, size_t packet_count)
{
    json::Object root;
    root.add(u"index", _total_packets);
    root.add(u"pid", pid);
    root.add(u"type", type);
    if (packet_count != NPOS) {
        root.add(u"packets", packet_count);
    }
    _report->log(_severity, _prefix + root.oneLiner(*_report));
}


//----------------------------------------------------------------------------
// Detect / fix error on packet.
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
        const bool new_pid = state.first_cc == INVALID_CC;

        // Remember initial characteristics of the input packet.
        const uint8_t last_cc_in = new_pid ? INVALID_CC : state.last_pkt_in.getCC();
        const uint8_t cc = pkt->getCC();
        const bool has_payload = pkt->hasPayload();
        const bool has_discontinuity = pkt->getDiscontinuityIndicator();
        const bool duplicated = !new_pid && !has_discontinuity && pkt->isDuplicate(state.last_pkt_in);

        // Save input packet as originally received.
        state.last_pkt_in = *pkt;

        if (new_pid) {
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
        else if (has_discontinuity) {
            // Discontinuity indicator is set, ignore any discontinuity.
            state.dup_count = 0;
        }
        else if (duplicated) {
            // Duplicate packet.
            if (++state.dup_count >= 2) {
                // The standard allows at most 2 duplicate packets.
                if (_display_errors) {
                    if (_json) {
                        logJSON(pid, u"duplicate", state.dup_count + 1);
                    }
                    else {
                        _report->log(_severity, u"%s, %d duplicate packets", {linePrefix(pid), state.dup_count + 1});
                    }
                }
                // There is nothing we can do to fix this.
                _error_count++;
                result = false;
            }
            if (update &&_fix_errors) {
                // Check if we need to replicate a duplicate packet (same CC) or increment the CC.
                const uint8_t cc_out = _replicate_dup || !has_payload ? state.last_cc_out : ((state.last_cc_out + 1) & CC_MASK);
                if (cc != cc_out) {
                    pkt->setCC(cc_out);
                    result = false;
                    _fix_count++;
                }
            }
        }
        else {
            // Compute expected CC for this packet.
            const uint8_t good_cc_in = has_payload ? ((last_cc_in + 1) & CC_MASK) : last_cc_in;
            const uint8_t good_cc_out = has_payload ? ((state.last_cc_out + 1) & CC_MASK) : state.last_cc_out;

            if (cc != good_cc_in) {
                if (_display_errors) {
                    // Display a specific message depending on the error.
                    if (!has_payload && cc == ((last_cc_in + 1) & CC_MASK)) {
                        if (_json) {
                            logJSON(pid, u"increment-without-payload");
                        }
                        else {
                            _report->log(_severity, u"%s, incorrect CC increment without payload", {linePrefix(pid)});
                        }
                    }
                    else {
                        if (_json) {
                            logJSON(pid, u"missing", MissingPackets(last_cc_in, cc));
                        }
                        else {
                            _report->log(_severity, u"%s, missing %d packets", {linePrefix(pid), MissingPackets(last_cc_in, cc)});
                        }
                    }
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
        state.last_cc_out = pkt->getCC();
        _processed_packets++;
    }

    // Count total packets.
    _total_packets++;
    return result;
}
