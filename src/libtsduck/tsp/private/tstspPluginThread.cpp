//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstspPluginThread.h"
#include "tstspCore.h"
#include "tsPluginRepository.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::PluginThread::PluginThread(const std::shared_ptr<tsp::Core>& core, PluginType type, const PluginOptions& plugin_options, const ThreadAttributes& attributes) :
    Thread(),
    TSP(core, plugin_options.name + u": ", type),
    _plugin_name(plugin_options.name),
    _plugin(nullptr)
{
    const UChar* shell_opt = nullptr;

    // Create the plugin instance object
    switch (type) {
        case PluginType::INPUT: {
                PluginRepository::InputPluginFactory allocator = PluginRepository::Instance().getInput(_plugin_name, core->report());
            if (allocator != nullptr) {
                _plugin = allocator(this);
                shell_opt = u" -I";
            }
            break;
        }
        case PluginType::OUTPUT: {
            PluginRepository::OutputPluginFactory allocator = PluginRepository::Instance().getOutput(_plugin_name, core->report());
            if (allocator != nullptr) {
                _plugin = allocator(this);
                shell_opt = u" -O";
            }
            break;
        }
        case PluginType::PROCESSOR: {
            PluginRepository::ProcessorPluginFactory allocator = PluginRepository::Instance().getProcessor(_plugin_name, core->report());
            if (allocator != nullptr) {
                _plugin = allocator(this);
                shell_opt = u" -P";
            }
            break;
        }
        default:
            assert(false);
    }

    if (_plugin == nullptr) {
        // Error message already displayed.
        return;
    }

    // Configure plugin object.
    _plugin->setShell(core->commonArgs().app_name + shell_opt);
    _plugin->setMaxSeverity(core->report().maxSeverity());

    // Submit the plugin arguments for analysis.
    // Do not process argument redirection, already done at tsp command level.
    _plugin->analyze(_plugin_name, plugin_options.args, false);

    // The process should have terminated on argument error.
    assert(_plugin->valid());

    // Get non-default thread stack size.
    size_t stackSize = 0;
    if (!GetEnvironment(u"TSPLUGINS_STACK_SIZE").toInteger(stackSize, UString::DEFAULT_THOUSANDS_SEPARATOR) || stackSize == 0) {
        // Use default value.
        stackSize = STACK_SIZE_OVERHEAD + _plugin->stackUsage();
    }

    // Define thread name and stack size.
    // Exit application when a thread terminates on an exception. This is required because
    // a dead plugin thread will block the processing chain and the application will hang.
    ThreadAttributes attr(attributes);
    attr.setName(_plugin_name);
    attr.setStackSize(stackSize);
    attr.setExitOnException(true);
    Thread::setAttributes(attr);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::tsp::PluginThread::~PluginThread()
{
    // Deallocate plugin instance, if allocated.
    if (_plugin != nullptr) {
        delete _plugin;
        _plugin = nullptr;
    }
}


//----------------------------------------------------------------------------
// Implementation of TSP interface.
//----------------------------------------------------------------------------

ts::UString ts::tsp::PluginThread::pluginName() const
{
    return _plugin_name;
}

ts::Plugin* ts::tsp::PluginThread::plugin() const
{
    return _plugin;
}

//----------------------------------------------------------------------------
// Set the plugin name as displayed in log messages.
//----------------------------------------------------------------------------

void ts::tsp::PluginThread::setLogName(const UString& name)
{
    setReportPrefix((name.empty() ? _plugin_name : name) + u": ");
}
