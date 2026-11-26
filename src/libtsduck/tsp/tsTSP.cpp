//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSP.h"
#include "tstspCore.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSP::TSP(const std::shared_ptr<tsp::Core>& core, const UString& prefix, PluginType type) :
    Report(core->report().maxSeverity(), prefix, &core->report()),
    _core(core),
    _plugin_type(type)
{
}

ts::TSP::~TSP()
{
}


//----------------------------------------------------------------------------
// Implementation of AbortInterface.
//----------------------------------------------------------------------------

bool ts::TSP::aborting() const
{
    return _tsp_aborting;
}


//----------------------------------------------------------------------------
// Activates or deactivates "joint termination".
//----------------------------------------------------------------------------

bool ts::TSP::useJointTermination(bool on)
{
    if (on && !_use_jt) {
        _use_jt = true;
        const int users = _core->declareJointTerminationUser(true);
        debug(u"using joint termination, now %d plugins use it", users);
    }
    else if (!on && _use_jt) {
        _use_jt = false;
        const int users = _core->declareJointTerminationUser(false);
        debug(u"no longer using joint termination, now %d plugins use it", users);
    }
    return true;
}


//----------------------------------------------------------------------------
// Signaling "joint termination".
//----------------------------------------------------------------------------

void ts::TSP::jointTerminate()
{
    if (_use_jt && !_jt_completed) {
        _jt_completed = true;
        const int remaining = _core->declareJointTermination(totalPacketsInThread());
        debug(u"completed for joint termination, %d plugins remaining", remaining);
    }
}
