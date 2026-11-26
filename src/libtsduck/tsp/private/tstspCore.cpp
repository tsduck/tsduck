//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstspCore.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::Core::Core(const TSProcessorCommonArgs& options, Report& report) :
    _args(options),
    _report(report)
{
}


//----------------------------------------------------------------------------
// Reset the "Joint Termination" manager.
//----------------------------------------------------------------------------

void ts::tsp::Core::reset()
{
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    _jt_users = 0;
    _jt_remaining = 0;
    _jt_highest_pkt = 0;
}


//----------------------------------------------------------------------------
// Declare that a plugin uses or no longer uses joint termination.
//----------------------------------------------------------------------------

int ts::tsp::Core::declareJointTerminationUser(bool on)
{
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    if (on) {
        _jt_users++;
        _jt_remaining++;
    }
    else {
        _jt_users--;
        _jt_remaining--;
        assert (_jt_users >= 0);
        assert (_jt_remaining >= 0);
    }
    return _jt_users;
}


//----------------------------------------------------------------------------
// Declare that a plugin performs joint termination.
//----------------------------------------------------------------------------

int ts::tsp::Core::declareJointTermination(PacketCounter total_packets_in_thread)
{
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    _jt_remaining--;
    assert(_jt_remaining >= 0);
    if (total_packets_in_thread > _jt_highest_pkt) {
        _jt_highest_pkt = total_packets_in_thread;
    }
    return _jt_remaining;
}


//----------------------------------------------------------------------------
// Return packet number after which the "joint termination" must be applied.
// If no "joint termination" applies, return the maximum int value.
//----------------------------------------------------------------------------

ts::PacketCounter ts::tsp::Core::totalPacketsBeforeJointTermination()
{
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    return !_args.ignore_jt && _jt_users > 0 && _jt_remaining <= 0 ? _jt_highest_pkt : std::numeric_limits<PacketCounter>::max();
}
