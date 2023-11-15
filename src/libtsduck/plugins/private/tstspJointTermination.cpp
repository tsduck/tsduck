//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstspJointTermination.h"


//----------------------------------------------------------------------------
// Static data, access under protection of the global mutex only.
//----------------------------------------------------------------------------

int ts::tsp::JointTermination::_jt_users = 0;
int ts::tsp::JointTermination::_jt_remaining = 0;
ts::PacketCounter ts::tsp::JointTermination::_jt_hightest_pkt = 0;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::JointTermination::JointTermination(const TSProcessorArgs& options,
                                            PluginType type,
                                            const PluginOptions& pl_options,
                                            const ThreadAttributes& attributes,
                                            std::recursive_mutex& global_mutex,
                                            Report* report) :

    PluginThread(report, options.app_name, type, pl_options, attributes),
    _global_mutex(global_mutex),
    _options(options)
{
}


//----------------------------------------------------------------------------
// Implementation of "joint termination", inherited from TSP.
//----------------------------------------------------------------------------

bool ts::tsp::JointTermination::useJointTermination() const
{
    return _use_jt;
}

bool ts::tsp::JointTermination::thisJointTerminated() const
{
    return _jt_completed;
}


//----------------------------------------------------------------------------
// This method activates "joint termination" for the calling plugin.
// It should be invoked during the plugin's start().
//----------------------------------------------------------------------------

void ts::tsp::JointTermination::useJointTermination(bool on)
{
    if (on && !_use_jt) {
        _use_jt = true;
        {
            std::lock_guard<std::recursive_mutex> lock(_global_mutex);
            _jt_users++;
            _jt_remaining++;
        }
        debug(u"using \"joint termination\", now %d plugins use it", {_jt_users});
    }
    else if (!on && _use_jt) {
        _use_jt = false;
        {
            std::lock_guard<std::recursive_mutex> lock(_global_mutex);
            _jt_users--;
            _jt_remaining--;
            assert (_jt_users >= 0);
            assert (_jt_remaining >= 0);
        }
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
        {
            std::lock_guard<std::recursive_mutex> lock(_global_mutex);
            _jt_remaining--;
            assert(_jt_remaining >= 0);
        }
        if (totalPacketsInThread() > _jt_hightest_pkt) {
            _jt_hightest_pkt = totalPacketsInThread();
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
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    return !_options.ignore_jt && _jt_users > 0 && _jt_remaining <= 0 ? _jt_hightest_pkt : std::numeric_limits<PacketCounter>::max();
}
