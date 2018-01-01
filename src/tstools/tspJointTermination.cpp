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
//  Transport stream processor: Implementation of "Joint Termination"
//
//----------------------------------------------------------------------------

#include "tspJointTermination.h"
#include "tsGuard.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Static data, access under protection of the global mutex only.
//----------------------------------------------------------------------------

int ts::tsp::JointTermination::_jt_users = 0;
int ts::tsp::JointTermination::_jt_remaining = 0;
ts::PacketCounter ts::tsp::JointTermination::_jt_hightest_pkt = 0;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::JointTermination::JointTermination(const Options* options, Mutex& global_mutex) :
    TSP(options->maxSeverity()),
    _global_mutex(global_mutex),
    _total_packets(0),
    _ignore_jt(options->ignore_jt),
    _use_jt(false),
    _jt_completed(false)
{
}



//----------------------------------------------------------------------------
// This method activates "joint termination" for the calling plugin.
// It should be invoked during the plugin's start().
//----------------------------------------------------------------------------

void ts::tsp::JointTermination::useJointTermination(bool on)
{
    if (on && !_use_jt) {
        _use_jt = true;
        Guard lock (_global_mutex);
        _jt_users++;
        _jt_remaining++;
        debug(u"using \"joint termination\", now %d plugins use it", {_jt_users});
    }
    else if (!on && _use_jt) {
        _use_jt = false;
        Guard lock (_global_mutex);
        _jt_users--;
        _jt_remaining--;
        assert (_jt_users >= 0);
        assert (_jt_remaining >= 0);
        debug(u"no longer using \"joint termination\", now %d plugins use it", {_jt_users});
    }
}


//----------------------------------------------------------------------------
// This method is used by the plugin to declare that the plugin's
// execution is potentially terminated in the context of "joint
// termination" between several plugins.
//----------------------------------------------------------------------------

void ts::tsp::JointTermination::jointTerminate()
{
    if (_use_jt && !_jt_completed) {
        _jt_completed = true;
        Guard lock(_global_mutex);
        _jt_remaining--;
        assert(_jt_remaining >= 0);
        if (_total_packets > _jt_hightest_pkt) {
            _jt_hightest_pkt = _total_packets;
        }
        debug(u"completed for \"joint termination\", %d plugins remaining, current pkt limit: %'d", {_jt_remaining, _jt_hightest_pkt});
    }
}


//----------------------------------------------------------------------------
// Return packet number after which the "joint termination" must be applied.
// If no "joint termination" applies, return the maximum int value.
//----------------------------------------------------------------------------

ts::PacketCounter ts::tsp::JointTermination::totalPacketsBeforeJointTermination() const
{
    Guard lock (_global_mutex);
    return !_ignore_jt && _jt_users > 0 && _jt_remaining <= 0 ? _jt_hightest_pkt : std::numeric_limits<PacketCounter>::max();
}
