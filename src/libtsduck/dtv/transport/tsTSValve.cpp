//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSValve.h"


//----------------------------------------------------------------------------
// Filter meaningful status.
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::TSValve::NormalizeStatus(PacketProcessStatus s)
{
    return s == TSP_OK || s == TSP_NULL ? s : TSP_DROP;
}


//----------------------------------------------------------------------------
// Reset the transmission.
//----------------------------------------------------------------------------

void ts::TSValve::reset(const TSValveArgs& args, PacketProcessStatus initial_status, PacketProcessStatus first_status)
{
    _args = args;
    _pid_contexts.clear();
    _total_packets = 0;
    _current_status = _previous_status = NormalizeStatus(initial_status);
    change(first_status);
}


//----------------------------------------------------------------------------
// Change the transmission state.
//----------------------------------------------------------------------------

void ts::TSValve::change(PacketProcessStatus new_status)
{
    new_status = NormalizeStatus(new_status);

    // Check if we should start a transition period.
    if (new_status != _current_status) {

        // Distinct previous and current status means "in a transition period".
        _previous_status = _current_status;
        _current_status = new_status;

        // Prepare the transition period.
        _transitioning_pids = 0;
        _next_silent_check = _total_packets + _args.pid_max_silent;
        for (auto& it : _pid_contexts) {
            // A PID is starting a transition if it is not silent (some packets were recently found).
            // Stuffing packets are never transitioned.
            const PacketCounter silent_after = it.second.last_packet + _args.pid_max_silent;
            it.second.transitioning = it.first != PID_NULL && _total_packets < silent_after;
            if (it.second.transitioning) {
                _transitioning_pids++;
                _next_silent_check = std::min(_next_silent_check, silent_after);
            }
        }

        _report.debug(u"switching from %s to %s after %'d packets, %d PIDs, %d transitioning",
                      _names.name(_previous_status), _names.name(_current_status),
                      _total_packets, _pid_contexts.size(), _transitioning_pids);
    }
}


//----------------------------------------------------------------------------
// Process one TS packet.
//----------------------------------------------------------------------------

ts::PacketProcessStatus ts::TSValve::processPacket(const TSPacket& pkt)
{
    // By default, apply the current status (in a transition period, this is the new status).
    PacketProcessStatus status = _current_status;

    // Count packets on each PID.
    const PID pid = pkt.getPID();
    auto& pctx(_pid_contexts[pid]);
    pctx.last_packet = _total_packets;

    // New PID's are always considered as transitioning (except stuffing of course).
    if (pctx.new_pid) {
        pctx.transitioning = pid != PID_NULL;
        pctx.new_pid = false;
        _transitioning_pids += pctx.transitioning;
    }

    // Do something only inside a transition period (preserve unit boundaries and not in stable state).
    // PID's which are no longer transitioning are subject to the new status.
    if (_args.preserve_units && _current_status != _previous_status && pctx.transitioning) {

        if (pkt.getPUSI()) {
            // This is a unit boundary, we can now apply the new status to this PID.
            assert(_transitioning_pids > 0);
            _transitioning_pids--;
            pctx.transitioning = false;
            // If we transition to drop and need to apply stuffing during the transition period.
            if (_current_status == TSP_DROP && _args.stuffing) {
                status = TSP_NULL;
            }
            _report.debug(u"switching PID %n to %s after %'d packets, remaining transitioning PIDs: %d", pid, _names.name(status), _total_packets, _transitioning_pids);
        }
        else if (_current_status == TSP_OK) {
            // Transition period from drop/null to pass: don't pass this PID yet.
            assert(_previous_status == TSP_DROP || _previous_status == TSP_NULL);
            status = _args.stuffing ? TSP_NULL : _previous_status;
        }
        else {
            // Transition period from pass to drop/null: continue to pass this PID.
            assert(_previous_status == TSP_OK);
            assert(_current_status == TSP_DROP || _current_status == TSP_NULL);
            status = TSP_OK;
        }

        // Do we need to check silent PID's to exclude them from the transition period?
        if (_transitioning_pids > 0 && _total_packets >= _next_silent_check) {
            _next_silent_check = _total_packets + _args.pid_max_silent;
            for (auto& it : _pid_contexts) {
                if (it.second.transitioning) {
                    assert(_transitioning_pids > 0);
                    const PacketCounter silent_after = it.second.last_packet + _args.pid_max_silent;
                    if (_total_packets >= silent_after) {
                        // This PID is now silent, ignore it in this transition period.
                        it.second.transitioning = false;
                        _transitioning_pids--;
                        _report.debug(u"PID %n is declared silent at packet %'d", it.first, _total_packets);
                    }
                    else {
                        _next_silent_check = std::min(_next_silent_check, it.second.last_packet + _args.pid_max_silent);
                    }
                }
            }
        }

        // Detect end of transition period.
        // If we are in the initial transition period, new PIDs may appear later. So make sure we have exceeded the silent period.
        if (_transitioning_pids == 0 && _total_packets >= _args.pid_max_silent) {
            // We have reached a stable state.
            _previous_status = _current_status;
            _report.debug(u"end of transition from %s to %s after %'d packets", _names.name(_previous_status), _names.name(_current_status), _total_packets);
        }
    }

    // Count packets in the TS.
    _total_packets++;
    return status;
}
