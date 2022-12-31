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

#include "tsPluginThread.h"
#include "tsPluginRepository.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PluginThread::PluginThread(Report* report, const UString& appName, PluginType type, const PluginOptions& options, const ThreadAttributes& attributes) :
    Thread(),
    TSP(report->maxSeverity()),
    _report(report),
    _name(options.name),
    _logname(),
    _shlib(nullptr)
{
    const UChar* shellOpt = nullptr;

    // Create the plugin instance object
    switch (type) {
        case PluginType::INPUT: {
            PluginRepository::InputPluginFactory allocator = PluginRepository::Instance()->getInput(_name, *report);
            if (allocator != nullptr) {
                _shlib = allocator(this);
                shellOpt = u" -I";
            }
            break;
        }
        case PluginType::OUTPUT: {
            PluginRepository::OutputPluginFactory allocator = PluginRepository::Instance()->getOutput(_name, *report);
            if (allocator != nullptr) {
                _shlib = allocator(this);
                shellOpt = u" -O";
            }
            break;
        }
        case PluginType::PROCESSOR: {
            PluginRepository::ProcessorPluginFactory allocator = PluginRepository::Instance()->getProcessor(_name, *report);
            if (allocator != nullptr) {
                _shlib = allocator(this);
               shellOpt = u" -P";
            }
            break;
        }
        default:
            assert(false);
    }

    if (_shlib == nullptr) {
        // Error message already displayed.
        return;
    }

    // Configure plugin object.
    _shlib->setShell(appName + shellOpt);
    _shlib->setMaxSeverity(report->maxSeverity());

    // Submit the plugin arguments for analysis.
    // Do not process argument redirection, already done at tsp command level.
    _shlib->analyze(options.name, options.args, false);

    // The process should have terminated on argument error.
    assert(_shlib->valid());

    // Get non-default thread stack size.
    size_t stackSize = 0;
    if (!GetEnvironment(u"TSPLUGINS_STACK_SIZE").toInteger(stackSize, UString::DEFAULT_THOUSANDS_SEPARATOR) || stackSize == 0) {
        // Use default value.
        stackSize = STACK_SIZE_OVERHEAD + _shlib->stackUsage();
    }

    // Define thread name and stack size.
    ThreadAttributes attr(attributes);
    attr.setName(_name);
    attr.setStackSize(stackSize);
    Thread::setAttributes(attr);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::PluginThread::~PluginThread()
{
    // Deallocate plugin instance, if allocated.
    if (_shlib != nullptr) {
        delete _shlib;
        _shlib = nullptr;
    }
}


//----------------------------------------------------------------------------
// Implementation of TSP interface.
//----------------------------------------------------------------------------

ts::UString ts::PluginThread::pluginName() const
{
    return _name;
}

ts::Plugin* ts::PluginThread::plugin() const
{
    return _shlib;
}


//----------------------------------------------------------------------------
// Invoked by the plugin shared library to log messages.
// Inherited from Report via TSP.
//----------------------------------------------------------------------------

void ts::PluginThread::writeLog(int severity, const UString& msg)
{
    _report->log(severity, u"%s: %s", {_logname.empty() ? _name : _logname, msg});
}
