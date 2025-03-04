//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPluginThread.h"
#include "tsPluginRepository.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PluginThread::PluginThread(Report* report, const UString& appName, PluginType type, const PluginOptions& options, const ThreadAttributes& attributes) :
    Thread(),
    TSP(report->maxSeverity(), options.name + u": ", report),
    _name(options.name),
    _shlib(nullptr)
{
    const UChar* shellOpt = nullptr;

    // Create the plugin instance object
    switch (type) {
        case PluginType::INPUT: {
            PluginRepository::InputPluginFactory allocator = PluginRepository::Instance().getInput(_name, *report);
            if (allocator != nullptr) {
                _shlib = allocator(this);
                shellOpt = u" -I";
            }
            break;
        }
        case PluginType::OUTPUT: {
            PluginRepository::OutputPluginFactory allocator = PluginRepository::Instance().getOutput(_name, *report);
            if (allocator != nullptr) {
                _shlib = allocator(this);
                shellOpt = u" -O";
            }
            break;
        }
        case PluginType::PROCESSOR: {
            PluginRepository::ProcessorPluginFactory allocator = PluginRepository::Instance().getProcessor(_name, *report);
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
    // Exit application when a thread terminates on an exception. This is required because
    // a dead plugin thread will block the processing chain and the application will hang.
    ThreadAttributes attr(attributes);
    attr.setName(_name);
    attr.setStackSize(stackSize);
    attr.setExitOnException(true);
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
// Set the plugin name as displayed in log messages.
//----------------------------------------------------------------------------

void ts::PluginThread::setLogName(const UString& name)
{
    setReportPrefix((name.empty() ? _name : name) + u": ");
}
