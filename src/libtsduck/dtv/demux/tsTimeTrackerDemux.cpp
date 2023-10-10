//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTimeTrackerDemux.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Internal class which tracks time stamps on one PID.
//----------------------------------------------------------------------------

// Reset all value, forget collected time stamps.
void ts::TimeTrackerDemux::TimeTracker::reset()
{
    _first = _last = INVALID_PCR;
    _offset = 0;
}

// Set a new collected time stamp value.
void ts::TimeTrackerDemux::TimeTracker::set(uint64_t value)
{
    if (value >= _scale) {
        // Invalid value, ignore.
    }
    else if (_first >= _scale) {
        // This is the first collected value.
        _first = _last = value;
    }
    else if (value >= _last) {
        // Greater than last value, sequence ok.
        _last = value;
    }
    else if (_last - value > _scale / 2) {
        // New value is much lower than last value, indicating a probable wrap-up.
        // The idea is to ignore slightly lower values such as out-of-order PTS.
        _last = value;
        _offset += _scale; // one more wrap-up.
    }
}

// Get the total duration, in time stamp units, between the first and last value.
uint64_t ts::TimeTrackerDemux::TimeTracker::duration() const
{
    assert(_first >= _scale || _last + _offset >= _first);
    return _first >= _scale ? 0 : _last + _offset - _first;
}


//----------------------------------------------------------------------------
// Construction and reset.
//----------------------------------------------------------------------------

ts::TimeTrackerDemux::TimeTrackerDemux(DuckContext& duck, const PIDSet& pid_filter) :
    SuperClass(duck, pid_filter)
{
}

void ts::TimeTrackerDemux::immediateReset()
{
    SuperClass::immediateReset();
    _pcrPID = PID_NULL;
    _pcrTime.reset();
    _pids.clear();
}

void ts::TimeTrackerDemux::immediateResetPID(PID pid)
{
    SuperClass::immediateResetPID(pid);
    _pids.erase(pid);
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::TimeTrackerDemux::feedPacket(const TSPacket& pkt)
{
    const PID pid = pkt.getPID();

    SuperClass::feedPacket(pkt);

    // Track PCR's on the first PID with PCR.
    if (pkt.hasPCR()) {
        if (_pcrPID == PID_NULL) {
            // No PCR PID was found so far, use this one.
            _pcrPID = pid;
        }
        if (pid == _pcrPID) {
            _pcrTime.set(pkt.getPCR());
        }
    }

    // Track PTS on the demuxed PID's.
    if (_pid_filter[pid] && pkt.hasPTS()) {
        _pids[pid].set(pkt.getPTS());
    }
}


//----------------------------------------------------------------------------
// Get the number of milliseconds measured on a PID.
//----------------------------------------------------------------------------

ts::MilliSecond ts::TimeTrackerDemux::pidDuration(PID pid) const
{
    auto it = _pids.find(pid);
    if (it != _pids.end() && it->second.isValid()) {
        // We have PTS references from the specified PID.
        return (it->second.duration() * 1000) / SYSTEM_CLOCK_SUBFREQ;
    }
    else if (_pcrTime.isValid()) {
        // Use PCR references from some other PID.
        return (_pcrTime.duration() * 1000) / SYSTEM_CLOCK_FREQ;
    }
    else {
        // No reference available, no timing information.
        return 0;
    }
}
