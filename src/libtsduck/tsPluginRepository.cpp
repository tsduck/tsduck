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

#include "tsPluginRepository.h"
#include "tsApplicationSharedLibrary.h"
#include "tsPluginSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

TS_DEFINE_SINGLETON(ts::PluginRepository);


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

void ts::PluginRepository::registerInput(const UString& name, NewInputProfile allocator)
{
    if (allocator != nullptr) {
        _inputPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerProcessor(const UString& name, NewProcessorProfile allocator)
{
    if (allocator != nullptr) {
        _processorPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerOutput(const UString& name, NewOutputProfile allocator)
{
    if (allocator != nullptr) {
        _outputPlugins[name] = allocator;
    }
}

ts::PluginRepository::Register::Register(const char* name, NewInputProfile allocator)
{
    PluginRepository::Instance()->registerInput(UString::FromUTF8(name), allocator);
}

ts::PluginRepository::Register::Register(const char* name, NewProcessorProfile allocator)
{
    PluginRepository::Instance()->registerProcessor(UString::FromUTF8(name), allocator);
}

ts::PluginRepository::Register::Register(const char* name, NewOutputProfile allocator)
{
    PluginRepository::Instance()->registerOutput(UString::FromUTF8(name), allocator);
}


//----------------------------------------------------------------------------
// Get plugins by name.
//----------------------------------------------------------------------------

ts::NewInputProfile ts::PluginRepository::getInput(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const InputMap::const_iterator it = _inputPlugins.find(name);
    if (it != _inputPlugins.end()) {
        assert(it->second != nullptr);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"input plugin %s not found", {name});
        return nullptr;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return nullptr;
    }
    else if (shlib.new_input != nullptr) {
        registerInput(shlib.moduleName(), shlib.new_input);
        return shlib.new_input;
    }
    else {
        report.error(u"plugin %s has no input capability", {shlib.moduleName()});
        return nullptr;
    }
}

ts::NewProcessorProfile ts::PluginRepository::getProcessor(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const ProcessorMap::const_iterator it = _processorPlugins.find(name);
    if (it != _processorPlugins.end()) {
        assert(it->second != nullptr);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"processor plugin %s not found", {name});
        return nullptr;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return nullptr;
    }
    else if (shlib.new_processor != nullptr) {
        registerProcessor(shlib.moduleName(), shlib.new_processor);
        return shlib.new_processor;
    }
    else {
        report.error(u"plugin %s has no processor capability", {shlib.moduleName()});
        return nullptr;
    }
}

ts::NewOutputProfile ts::PluginRepository::getOutput(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const OutputMap::const_iterator it = _outputPlugins.find(name);
    if (it != _outputPlugins.end()) {
        assert(it->second != nullptr);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"output plugin %s not found", {name});
        return nullptr;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return nullptr;
    }
    else if (shlib.new_output != nullptr) {
        registerOutput(shlib.moduleName(), shlib.new_output);
        return shlib.new_output;
    }
    else {
        report.error(u"plugin %s has no output capability", {shlib.moduleName()});
        return nullptr;
    }
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

    // Load all plugins and register allocator functions (when not zero).
    for (size_t i = 0; i < files.size(); ++i) {
        PluginSharedLibrary shlib(files[i], report);
        if (shlib.isLoaded()) {
            const UString name(shlib.moduleName());
            registerInput(name, shlib.new_input);
            registerOutput(name, shlib.new_output);
            registerProcessor(name, shlib.new_processor);
        }
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
