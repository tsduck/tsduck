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
    if (allocator != 0) {
        _inputPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerProcessor(const UString& name, NewProcessorProfile allocator)
{
    if (allocator != 0) {
        _processorPlugins[name] = allocator;
    }
}

void ts::PluginRepository::registerOutput(const UString& name, NewOutputProfile allocator)
{
    if (allocator != 0) {
        _outputPlugins[name] = allocator;
    }
}

ts::PluginRepository::Register::Register(const UString& name, NewInputProfile allocator)
{
    PluginRepository::Instance()->registerInput(name, allocator);
}

ts::PluginRepository::Register::Register(const UString& name, NewProcessorProfile allocator)
{
    PluginRepository::Instance()->registerProcessor(name, allocator);
}

ts::PluginRepository::Register::Register(const UString& name, NewOutputProfile allocator)
{
    PluginRepository::Instance()->registerOutput(name, allocator);
}


//----------------------------------------------------------------------------
// Get plugins by name.
//----------------------------------------------------------------------------

ts::NewInputProfile ts::PluginRepository::getInput(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const InputMap::const_iterator it = _inputPlugins.find(name);
    if (it != _inputPlugins.end()) {
        assert(it->second != 0);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"input plugin %s not found", {name});
        return 0;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return 0;
    }
    else if (shlib.new_input != 0) {
        registerInput(shlib.moduleName(), shlib.new_input);
        return shlib.new_input;
    }
    else {
        report.error(u"plugin %s has no input capability", {shlib.moduleName()});
        return 0;
    }
}

ts::NewProcessorProfile ts::PluginRepository::getProcessor(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const ProcessorMap::const_iterator it = _processorPlugins.find(name);
    if (it != _processorPlugins.end()) {
        assert(it->second != 0);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"processor plugin %s not found", {name});
        return 0;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return 0;
    }
    else if (shlib.new_processor != 0) {
        registerProcessor(shlib.moduleName(), shlib.new_processor);
        return shlib.new_processor;
    }
    else {
        report.error(u"plugin %s has no processor capability", {shlib.moduleName()});
        return 0;
    }
}

ts::NewOutputProfile ts::PluginRepository::getOutput(const UString& name, Report& report)
{
    // Search plugin in current cache.
    const OutputMap::const_iterator it = _outputPlugins.find(name);
    if (it != _outputPlugins.end()) {
        assert(it->second != 0);
        return it->second;
    }

    // Do nothing if loading dynamic libraries is disallowed.
    if (!_sharedLibraryAllowed) {
        report.error(u"output plugin %s not found", {name});
        return 0;
    }

    // Try to load a shareable library.
    PluginSharedLibrary shlib(name, report);
    if (!shlib.isLoaded()) {
        // Error message already displayed.
        return 0;
    }
    else if (shlib.new_output != 0) {
        registerOutput(shlib.moduleName(), shlib.new_output);
        return shlib.new_output;
    }
    else {
        report.error(u"plugin %s has no output capability", {shlib.moduleName()});
        return 0;
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

void ts::PluginRepository::listPlugins(bool loadAll, std::ostream& strm, Report& report)
{
    // Load all shareable plugins first.
    if (loadAll) {
        loadAllPlugins(report);
    }

    // Compute max name width of all plugins.
    size_t name_width = 0;
    for (InputMap::const_iterator it = _inputPlugins.begin(); it != _inputPlugins.end(); ++it) {
        name_width = std::max(name_width, it->first.width());
    }
    for (ProcessorMap::const_iterator it = _processorPlugins.begin(); it != _processorPlugins.end(); ++it) {
        name_width = std::max(name_width, it->first.width());
    }
    for (OutputMap::const_iterator it = _outputPlugins.begin(); it != _outputPlugins.end(); ++it) {
        name_width = std::max(name_width, it->first.width());
    }

    // List capabilities
    strm << std::endl << "List of tsp input plugins:" << std::endl << std::endl;
    for (InputMap::const_iterator it = _inputPlugins.begin(); it != _inputPlugins.end(); ++it) {
        Plugin* p = it->second(0);
        strm << "  " << it->first.toJustifiedLeft(name_width + 1, u'.', false, 1) << " " << p->getDescription() << std::endl;
        delete p;
    }

    strm << std::endl << "List of tsp output plugins:" << std::endl << std::endl;
    for (OutputMap::const_iterator it = _outputPlugins.begin(); it != _outputPlugins.end(); ++it) {
        Plugin* p = it->second(0);
        strm << "  " << it->first.toJustifiedLeft(name_width + 1, u'.', false, 1) << " " << p->getDescription() << std::endl;
        delete p;
    }

    strm << std::endl << "List of tsp packet processor plugins:" << std::endl << std::endl;
    for (ProcessorMap::const_iterator it = _processorPlugins.begin(); it != _processorPlugins.end(); ++it) {
        Plugin* p = it->second(0);
        strm << "  " << it->first.toJustifiedLeft(name_width + 1, u'.', false, 1) << " " << p->getDescription() << std::endl;
        delete p;
    }

    strm << std::endl;
}
