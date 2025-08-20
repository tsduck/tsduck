//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInputSwitcher.h"
#include "tstsswitchCore.h"
#include "tstsswitchCommandListener.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::InputSwitcher::InputSwitcher(Report& report) :
    _report(report)
{
}

ts::InputSwitcher::InputSwitcher(const InputSwitcherArgs& args, Report& report) :
    _report(report)
{
    _success = start(args);
    waitForTermination();
}

ts::InputSwitcher::~InputSwitcher()
{
    // Wait for processing termination to avoid other threads accessing a destroyed object.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Start the input switcher session.
//----------------------------------------------------------------------------

bool ts::InputSwitcher::start(const InputSwitcherArgs& args)
{
    // Filter already started.
    if (_core != nullptr) {
        _report.error(u"input switcher already started");
        return false;
    }

    // Keep command line options for further use.
    _args = args;
    _args.enforceDefaults();

    // Debug message.
    if (_report.debug()) {
        UString cmd(args.app_name);
        cmd.append(u" ");
        for (const auto& it : args.inputs) {
            cmd.append(u" ");
            cmd.append(it.toString(PluginType::INPUT));
        }
        cmd.append(u" ");
        cmd.append(args.output.toString(PluginType::OUTPUT));
        _report.debug(u"starting: %s", cmd);
    }

    // Clear errors on the report, used to check further initialisation errors.
    _report.resetErrors();

    // Create the tsswitch core instance.
    _core = new tsswitch::Core(_args, *this, _report);
    CheckNonNull(_core);
    _success = !_report.gotErrors();

    // If a remote control is specified, start a UDP listener thread.
    if (_success && _args.remote_control.server_addr.hasPort()) {
        _remote = new tsswitch::CommandListener(*_core, _args, _report);
        CheckNonNull(_remote);
        _success = _remote->open();
    }

    // Start the processing.
    _success = _success && _core->start();

    if (!_success) {
        internalCleanup();
    }
    return _success;
}


//----------------------------------------------------------------------------
// Delegations to core object.
//----------------------------------------------------------------------------

void ts::InputSwitcher::setInput(size_t pluginIndex)
{
    if (_core != nullptr) {
        _core->setInput(pluginIndex);
    }
}

void ts::InputSwitcher::nextInput()
{
    if (_core != nullptr) {
        _core->nextInput();
    }
}

void ts::InputSwitcher::previousInput()
{
    if (_core != nullptr) {
        _core->previousInput();
    }
}

size_t ts::InputSwitcher::currentInput()
{
    return _core == nullptr ? 0 : _core->currentInput();
}

void ts::InputSwitcher::stop()
{
    if (_core != nullptr) {
        _core->stop(true);
    }
}


//----------------------------------------------------------------------------
// Internal and unconditional cleanupp of resources.
//----------------------------------------------------------------------------

void ts::InputSwitcher::internalCleanup()
{
    // Deleting each object waits for all its internal threads terminations.
    // Terminate the remote control first since it references the core.
    if (_remote != nullptr) {
        delete _remote;
        _remote = nullptr;
    }

    // Then, terminate the core.
    if (_core != nullptr) {
        delete _core;
        _core = nullptr;
    }
}


//----------------------------------------------------------------------------
// Suspend the calling thread until input switcher is completed.
//----------------------------------------------------------------------------

void ts::InputSwitcher::waitForTermination()
{
    if (_core != nullptr) {
        _core->waitForTermination();
    }
    internalCleanup();
}
