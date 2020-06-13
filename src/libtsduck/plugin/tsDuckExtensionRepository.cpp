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

#include "tsDuckExtensionRepository.h"
#include "tsApplicationSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

// Define the singleton.
TS_DEFINE_SINGLETON(ts::DuckExtensionRepository);

// Force the creation of the singleton when the TSDuck library is loaded.
// This is the point where the extensions are loaded.
// Make it a exportable symbol to make sure that no compiler will optimize it away.
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(missing-variable-declarations)
const ts::DuckExtensionRepository* TSDuckExtensionRepository = ts::DuckExtensionRepository::Instance();
TS_POP_WARNING()



//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DuckExtensionRepository::DuckExtensionRepository() :
    _extensions()
{
    // Get all environment variables.
    const bool debug = !GetEnvironment(u"TSLIBEXT_DEBUG").empty();
    const bool none = !GetEnvironment(u"TSLIBEXT_NONE").empty();

#define EXTDEBUG(expr) do {if(debug) {std::cerr << "* debug: " << expr << std::endl << std::flush;}} while (0)

    // Give up now when TSLIBEXT_NONE is defined.
    if (none) {
        EXTDEBUG("TSLIBEXT_NONE defined, no extension loaded");
        return;
    }

    // Get the list of extensions to ignore.
    UStringVector ignore;
    GetEnvironment(u"TSLIBEXT_IGNORE").split(ignore, u',', true, true);
    EXTDEBUG(ignore.size() << " extension ignored");

    // Get list of shared library files
    UStringVector files;
    ApplicationSharedLibrary::GetPluginList(files, u"tslibext_", TS_PLUGINS_PATH);
    EXTDEBUG("found " << files.size() << " possible extensions");

    // Load all plugins and register allocator functions (when not zero).
    for (size_t i = 0; i < files.size(); ++i) {

        // Constant reference to the file name.
        const UString& filename(files[i]);

        // Get extension name from file name (without tslibext_).
        const UString name(BaseName(filename, TS_SHARED_LIB_SUFFIX).toRemovedPrefix(u"tslibext_", FileSystemCaseSensitivity));
        if (name.containSimilar(ignore)) {
            // This extension is listed in TSLIBEXT_IGNORE.
            EXTDEBUG("ignoring extension" << filename);
        }
        else {
            // This extension shall be loaded.
            // Use the "permanent" load flag to make sure the shared library remains active.
            EXTDEBUG("loading extension " << filename);
            ApplicationSharedLibrary shlib(filename, UString(), UString(), true);
            if (!shlib.isLoaded()) {
                EXTDEBUG("failed to load extension " << filename << " : " << shlib.errorMessage());
            }
            else {
                // Finding TSDuckExtensionId symbol in the shared library.
                void* sym = shlib.getSymbol("TSDuckExtensionId");
                if (sym == nullptr) {
                    EXTDEBUG("no symbol TSDuckExtensionId found in " << filename);
                }
                else {
                    // The returned address is the address of a pointer to ts::DuckExtension.
                    // See macro TS_REGISTER_EXTENSION.
                    ts::DuckExtension::ConstPointer ext = *reinterpret_cast<ts::DuckExtension::ConstPointer*>(sym);
                    if (ext != nullptr) {
                        // Now the extension is fully identified.
                        _extensions.push_back(std::make_pair(ext, filename));
                        EXTDEBUG("extension \"" << ext->name() << "\" loaded from " << filename);
                    }
                }
            }
        }
    }

    EXTDEBUG("loaded " << _extensions.size() << " extensions");

#undef EXTDEBUG
}


//----------------------------------------------------------------------------
// Search a file in a list of directories.
//----------------------------------------------------------------------------

namespace {
    ts::UString SearchFile(const ts::UStringList& dirs, const ts::UString& prefix, const ts::UString& name, const ts::UString& suffix)
    {
        for (auto it = dirs.begin(); it != dirs.end(); ++it) {
            const ts::UString filename(*it + ts::PathSeparator + prefix + name + suffix);
            if (ts::FileExists(filename)) {
                return filename;
            }
        }
        return u"not found";
    }
}


//----------------------------------------------------------------------------
// List all extensions.
//----------------------------------------------------------------------------

ts::UString ts::DuckExtensionRepository::listExtensions(ts::Report& report)
{
    // Compute max name width of all extensions.
    size_t width = 0;
    for (size_t i = 0; i < _extensions.size(); ++i) {
        width = std::max(width, _extensions[i].first->name().width());
    }
    width++; // spacing after name

    // Search path for plugins.
    UStringList plugins_dirs;
    GetEnvironmentPath(plugins_dirs, TS_PLUGINS_PATH);
    plugins_dirs.push_back(DirectoryName(ExecutableFile()));

    // Search path for executables.
    UStringList tools_dirs;
    GetEnvironmentPath(tools_dirs, TS_COMMAND_PATH);

    // Build the output text as a string.
    UString out;
    for (size_t iext = 0; iext < _extensions.size(); ++iext) {

        // Description of the plugin.
        const DuckExtension::ConstPointer ext = _extensions[iext].first;
        const UString& filename(_extensions[iext].second);
        const UStringVector& plugins(ext->plugins());
        const UStringVector& tools(ext->tools());

        // First line: name and description.
        out += UString::Format(u"%s %s\n", {ext->name().toJustifiedLeft(width, u'.', false, 1), ext->description()});

        if (report.verbose()) {
            // Display full file names.
            out += UString::Format(u"%*s Library: %s\n", {width, u"", filename});
            for (size_t i = 0; i < plugins.size(); ++i) {
                out += UString::Format(u"%*s Plugin %s: %s\n", {width, u"", plugins[i], SearchFile(plugins_dirs, u"tsplugin_", plugins[i], TS_SHARED_LIB_SUFFIX)});
            }
            for (size_t i = 0; i < tools.size(); ++i) {
                out += UString::Format(u"%*s Command %s: %s\n", {width, u"", tools[i], SearchFile(tools_dirs, u"", tools[i], TS_EXECUTABLE_SUFFIX)});
            }
        }
        else {
            // Only display plugins and tools names.
            if (!plugins.empty()) {
                out += UString::Format(u"%*s Plugins: %s\n", {width, u"", UString::Join(plugins)});
            }
            if (!tools.empty()) {
                out += UString::Format(u"%*s Commands: %s\n", {width, u"", UString::Join(tools)});
            }
        }
    }

    return out;
}
