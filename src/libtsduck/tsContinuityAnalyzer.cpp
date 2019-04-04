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
#include "tsUString.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::ContinuityAnalyzer::ContinuityAnalyzer(bool display, bool fix, const PIDSet& pid_filter, Report* report) :
    _report(report != nullptr ? report : NullReport::Instance()),
    _display_errors(display),
    _fix_errors(fix),
    _packet_count(0),
    _fix_count(0),
    _error_count(0),
    _pid_filter(pid_filter)
{
}

ts::ContinuityAnalyzer::~ContinuityAnalyzer()
{
    reset();
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
    _packet_count = 0;
    _fix_count = 0;
    _error_count = 0;

    //@@@@@
}


//----------------------------------------------------------------------------
// Reset the context for one single PID.
//----------------------------------------------------------------------------

void ts::ContinuityAnalyzer::resetPID(PID pid)
{
    //@@@@
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
                resetPID(pid);
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
        resetPID(pid);
    }
}

size_t ts::ContinuityAnalyzer::pidCount() const
{
    return _pid_filter.count();
}

bool ts::ContinuityAnalyzer::hasPID(PID pid) const
{
    return pid < _pid_filter.size() && _pid_filter.test(pid);
}


//----------------------------------------------------------------------------
// Process a constant TS packet.
//----------------------------------------------------------------------------

bool ts::ContinuityAnalyzer::feedPacket(const TSPacket& pkt)
{
    // Count packets.
    _packet_count++;

    // Process error reporting.
    bool success = true;
    if (_display_errors) {
        //@@@@
    }

    return success;
}


//----------------------------------------------------------------------------
// Process or modify a TS packet.
//----------------------------------------------------------------------------

bool ts::ContinuityAnalyzer::feedPacket(TSPacket& pkt)
{
    // Process error reporting first.
    bool success = feedPacket(*static_cast<const TSPacket*>(&pkt));

    // Process packet fixing.
    if (_fix_errors) {
        //@@@@
    }

    return success;
}
