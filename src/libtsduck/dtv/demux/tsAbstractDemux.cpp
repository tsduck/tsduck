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

#include "tsAbstractDemux.h"


//----------------------------------------------------------------------------
// Constructor / destructor.
//----------------------------------------------------------------------------

ts::AbstractDemux::AbstractDemux(DuckContext& duck, const PIDSet& pid_filter) :
    _duck(duck),
    _pid_filter(pid_filter),
    _packet_count(0),
    _in_handler(false),
    _pid_in_handler(PID_NULL),
    _reset_pending(false),
    _pid_reset_pending(false),
    _demux_id(0)
{
}

ts::AbstractDemux::~AbstractDemux()
{
}


//----------------------------------------------------------------------------
// PID filter management. The base class only manipulates the set of PID's.
// Concrete classes should override these methods to add specific processing.
//----------------------------------------------------------------------------

void ts::AbstractDemux::setPIDFilter(const PIDSet& pids)
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

void ts::AbstractDemux::addPID(PID pid)
{
    _pid_filter.set(pid);
}

void ts::AbstractDemux::addPIDs(const PIDSet& pids)
{
    _pid_filter |= pids;
}

void ts::AbstractDemux::removePID(PID pid)
{
    if (_pid_filter[pid]) {
        _pid_filter.reset(pid);
        resetPID(pid);
    }
}

size_t ts::AbstractDemux::pidCount() const
{
    return _pid_filter.count();
}

bool ts::AbstractDemux::hasPID(PID pid) const
{
    return pid < _pid_filter.size() && _pid_filter.test(pid);
}


//----------------------------------------------------------------------------
// Reset the demux. The base class does not do anything.
//----------------------------------------------------------------------------

void ts::AbstractDemux::reset()
{
    // In the context of a handler, delay the reset
    if (_in_handler) {
        _reset_pending = true;
    }
    else {
        immediateReset();
    }
}

void ts::AbstractDemux::resetPID(PID pid)
{
    // In the context of a handler, delay the reset
    if (_in_handler && pid == _pid_in_handler) {
        _pid_reset_pending = true;
    }
    else {
        immediateResetPID(pid);
    }
}

void ts::AbstractDemux::immediateReset()
{
}

void ts::AbstractDemux::immediateResetPID(PID pid)
{
}


//----------------------------------------------------------------------------
// Feed the demux with a TS packet.
//----------------------------------------------------------------------------

void ts::AbstractDemux::feedPacket(const TSPacket& pkt)
{
    // At this stage, we only count packets.
    // More interesting stuff in subclasses.
    _packet_count++;
}


//----------------------------------------------------------------------------
// Helpers for subclasses, protecting the invocation to handlers.
//----------------------------------------------------------------------------

void ts::AbstractDemux::beforeCallingHandler(PID pid)
{
    // Mark that we are in the context of handlers.
    _in_handler = true;
    _pid_in_handler = pid;
    _reset_pending = false;
    _pid_reset_pending = false;
}

bool ts::AbstractDemux::afterCallingHandler(bool executeDelayedOperations)
{
    bool result = false;

    // End of handler-calling sequence.
    _in_handler = false;

    // Now process the delayed destructions.
    if (executeDelayedOperations) {
        if (_pid_reset_pending) {
            // Reset of this PID was requested by a handler.
            immediateResetPID(_pid_in_handler);
            result = true;
        }
        if (_reset_pending) {
            // Full reset was requested by a handler.
            immediateReset();
            result = true;
        }
    }

    _pid_in_handler = PID_NULL;
    _pid_reset_pending = false;
    _reset_pending = false;

    return result;
}
