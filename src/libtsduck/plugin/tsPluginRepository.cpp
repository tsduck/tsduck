//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsPluginRepository.h"
#include "tsApplicationSharedLibrary.h"
#include "tsAlgorithm.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_DEFINE_SINGLETON(ts::PluginRepository);

// Options for --list-processor.
const ts::Enumeration ts::PluginRepository::ListProcessorEnum({
    {u"all",    ts::PluginRepository::LIST_ALL},
    {u"input",  ts::PluginRepository::LIST_INPUT  | ts::PluginRepository::LIST_COMPACT},
    {u"output", ts::PluginRepository::LIST_OUTPUT | ts::PluginRepository::LIST_COMPACT},
    {u"packet", ts::PluginRepository::LIST_PACKET | ts::PluginRepository::LIST_COMPACT},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PluginRepository::PluginRepository() :
    _sharedLibraryAllowed(true),
    _inputPlugins(),
    _processorPlugins(),
    _outputPlugins()
{
}


//----------------------------------------------------------------------------
// Plugin registration.
//----------------------------------------------------------------------------

void ts::PluginRepository::registerInput(const UString& name, InputPluginFactory allocator)
{
    if (allocator != nullptr) {
        _inputPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerProcessor(const UString& name, ProcessorPluginFactory allocator)
{
    if (allocator != nullptr) {
        _processorPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerOutput(const UString& name, OutputPluginFactory allocator)
{
    if (allocator != nullptr) {
        _outputPlugins[name] = allocator;
    }
}

ts::PluginRepository::Register::Register(const UString& name, InputPluginFactory allocator)
{
    PluginRepository::Instance()->registerInput(name, allocator);
}

ts::PluginRepository::Register::Register(const UString& name, ProcessorPluginFactory allocator)
{
    PluginRepository::Instance()->registerProcessor(name, allocator);
}

ts::PluginRepository::Register::Register(const UString& name, OutputPluginFactory allocator)
{
    PluginRepository::Instance()->registerOutput(name, allocator);
}


//----------------------------------------------------------------------------
// Get plugins by name.
//----------------------------------------------------------------------------

template<typename FACTORY>
FACTORY ts::PluginRepository::getFactory(const UString& plugin_name, const UString& plugin_type, const std::map<UString,FACTORY>& plugin_map, Report& report)
{
    // Search plugin in current cache.
    auto it = plugin_map.find(plugin_name);

    // Load a shared library if not found and allowed.
    if (it == plugin_map.end() && _sharedLibraryAllowed) {
        // Load shareable library. Use name resolution. Use permanent mapping to keep
        // the shareable image in memory after returning from this function.
        ApplicationSharedLibrary shlib(plugin_name, u"tsplugin_", TS_PLUGINS_PATH, true, report);
        if (shlib.isLoaded()) {
            // Search again if the shareable library was loaded.
            // The shareable library is supposed to register its plugins on initialization.
            it = plugin_map.find(plugin_name);
        }
        else {
            report.error(shlib.errorMessage());
        }
    }

    // Return the factory function if one was found.
    if (it != plugin_map.end()) {
        assert(it->second != nullptr);
        return it->second;
    }
    else {
        report.error(u"%s plugin %s not found", {plugin_type, plugin_name});
        return nullptr;
    }
}

ts::PluginRepository::InputPluginFactory ts::PluginRepository::getInput(const UString& name, Report& report)
{
    return getFactory(name, u"input", _inputPlugins, report);
}

ts::PluginRepository::ProcessorPluginFactory ts::PluginRepository::getProcessor(const UString& name, Report& report)
{
    return getFactory(name, u"processor", _processorPlugins, report);
}

ts::PluginRepository::OutputPluginFactory ts::PluginRepository::getOutput(const UString& name, Report& report)
{
    return getFactory(name, u"output", _outputPlugins, report);
}


//----------------------------------------------------------------------------
// Get the names of all registered plugins.
//----------------------------------------------------------------------------

ts::UStringList ts::PluginRepository::inputNames() const
{
    return MapKeys(_inputPlugins);
}

ts::UStringList ts::PluginRepository::processorNames() const
{
    return MapKeys(_processorPlugins);
}

ts::UStringList ts::PluginRepository::outputNames() const
{
    return MapKeys(_outputPlugins);
}


//----------------------------------------------------------------------------
// Load all available tsp processors.
//----------------------------------------------------------------------------

void ts::PluginRepository::loadAllPlugins(Report& report)
{
    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        return;
    }

    // Get list of shared library files
    UStringVector files;
    ApplicationSharedLibrary::GetPluginList(files, u"tsplugin_", TS_PLUGINS_PATH);

    // Load all plugins, let them register their plugins.
    for (size_t i = 0; i < files.size(); ++i) {
        // Permanent load.
        SharedLibrary shlib(files[i], true, report);
    }
}


//----------------------------------------------------------------------------
// List all tsp processors.
//----------------------------------------------------------------------------

ts::UString ts::PluginRepository::listPlugins(bool loadAll, Report& report, int flags)
{
    // Output text, use some preservation.
    UString out;
    out.reserve(5000);

    // Load all shareable plugins first.
    if (loadAll) {
        loadAllPlugins(report);
    }

    // Compute max name width of all plugins.
    size_t name_width = 0;
    if ((flags & LIST_COMPACT) == 0) {
        if ((flags & LIST_INPUT) != 0) {
            for (InputMap::const_iterator it = _inputPlugins.begin(); it != _inputPlugins.end(); ++it) {
                name_width = std::max(name_width, it->first.width());
            }
        }
        if ((flags & LIST_PACKET) != 0) {
            for (ProcessorMap::const_iterator it = _processorPlugins.begin(); it != _processorPlugins.end(); ++it) {
                name_width = std::max(name_width, it->first.width());
            }
        }
        if ((flags & LIST_OUTPUT) != 0) {
            for (OutputMap::const_iterator it = _outputPlugins.begin(); it != _outputPlugins.end(); ++it) {
                name_width = std::max(name_width, it->first.width());
            }
        }
    }

    // List capabilities.
    if ((flags & LIST_INPUT) != 0) {
        if ((flags & LIST_COMPACT) == 0) {
            out += u"\nList of tsp input plugins:\n\n";
        }
        for (InputMap::const_iterator it = _inputPlugins.begin(); it != _inputPlugins.end(); ++it) {
            Plugin* p = it->second(nullptr);
            ListOnePlugin(out, it->first, p, name_width, flags);
            delete p;
        }
    }

    if ((flags & LIST_OUTPUT) != 0) {
        if ((flags & LIST_COMPACT) == 0) {
            out += u"\nList of tsp output plugins:\n\n";
        }
        for (OutputMap::const_iterator it = _outputPlugins.begin(); it != _outputPlugins.end(); ++it) {
            Plugin* p = it->second(nullptr);
            ListOnePlugin(out, it->first, p, name_width, flags);
            delete p;
        }
    }

    if ((flags & LIST_PACKET) != 0) {
        if ((flags & LIST_COMPACT) == 0) {
            out += u"\nList of tsp packet processor plugins:\n\n";
        }
        for (ProcessorMap::const_iterator it = _processorPlugins.begin(); it != _processorPlugins.end(); ++it) {
            Plugin* p = it->second(nullptr);
            ListOnePlugin(out, it->first, p, name_width, flags);
            delete p;
        }
    }

    return out;
}


//----------------------------------------------------------------------------
// List one plugin.
//----------------------------------------------------------------------------

void ts::PluginRepository::ListOnePlugin(UString& out, const UString& name, Plugin* plugin, size_t name_width, int flags)
{
    if ((flags & LIST_COMPACT) != 0) {
        out += name;
        out += u":";
        out += plugin->getDescription();
        out += u"\n";
    }
    else {
        out += u"  ";
        out += name.toJustifiedLeft(name_width + 1, u'.', false, 1);
        out += u" ";
        out += plugin->getDescription();
        out += u"\n";
    }
}
