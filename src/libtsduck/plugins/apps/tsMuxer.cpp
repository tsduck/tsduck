//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMuxer.h"
#include "tstsmuxCore.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::Muxer::Muxer(Report& report) :
    _report(report)
{
}

ts::Muxer::~Muxer()
{
    // Wait for processing termination to avoid other threads accessing a destroyed object.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Start the multiplexer session.
//----------------------------------------------------------------------------

bool ts::Muxer::start(const MuxerArgs& args)
{
    // Filter already started.
    if (_core != nullptr) {
        _report.error(u"multiplexer already started");
        return false;
    }

    // Keep command line options for further use.
    _args = args;
    _args.enforceDefaults();

    // Debug message.
    if (_report.debug()) {
        UString cmd(args.appName);
        cmd.append(u" ");
        for (const auto& it : args.inputs) {
            cmd.append(u" ");
            cmd.append(it.toString(PluginType::INPUT));
        }
        cmd.append(u" ");
        cmd.append(args.output.toString(PluginType::OUTPUT));
        _report.debug(u"starting: %s", cmd);
    }

    // Allocate a muxer core object.
    _core = new tsmux::Core(args, *this, _report);
    CheckNonNull(_core);
    return _core->start();
}


//----------------------------------------------------------------------------
// Stop the multiplexer session.
//----------------------------------------------------------------------------

void ts::Muxer::stop()
{
    if (_core != nullptr) {
        _core->stop();
    }
}


//----------------------------------------------------------------------------
// Suspend the calling thread until input switcher is completed.
//----------------------------------------------------------------------------

void ts::Muxer::waitForTermination()
{
    if (_core != nullptr) {
        _core->waitForTermination();
        delete _core;
        _core = nullptr;
    }
}
