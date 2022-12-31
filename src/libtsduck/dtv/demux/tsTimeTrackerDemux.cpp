//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsTimeTrackerDemux.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Internal class which tracks time stamps on one PID.
//----------------------------------------------------------------------------

ts::TimeTrackerDemux::TimeTracker::TimeTracker(uint64_t scale) :
    _scale(scale),
    _first(INVALID_PCR),
    _last(INVALID_PCR),
    _offset(0)
{
}

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
    SuperClass(duck, pid_filter),
    _pcrPID(PID_NULL),
    _pcrTime(PCR_SCALE),
    _pids()
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
