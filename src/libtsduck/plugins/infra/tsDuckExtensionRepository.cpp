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

#include "tsDuckExtensionRepository.h"
#include "tsApplicationSharedLibrary.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsCerrReport.h"

// Define the singleton.
TS_DEFINE_SINGLETON(ts::DuckExtensionRepository);

// The internal singleton which loads all extensions.
const ts::DuckExtensionRepository::Loader ts::DuckExtensionRepository::LoaderInstance;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DuckExtensionRepository::DuckExtensionRepository() :
    _extensions()
{
}

ts::DuckExtensionRepository::Loader::Loader()
{
    // Give up now when TSLIBEXT_NONE is defined.
    if (!GetEnvironment(u"TSLIBEXT_NONE").empty()) {
        CERR.debug(u"TSLIBEXT_NONE defined, no extension loaded");
        return;
    }

    // Get the list of extensions to ignore.
    UStringVector ignore;
    GetEnvironment(u"TSLIBEXT_IGNORE").split(ignore, u',', true, true);
    CERR.debug(u"%d extensions ignored", {ignore.size()});

    // Get list of shared library files
    UStringVector files;
    ApplicationSharedLibrary::GetPluginList(files, u"tslibext_", TS_PLUGINS_PATH);
    CERR.debug(u"found %d possible extensions", {files.size()});

    // Load all plugins shared library.
    for (size_t i = 0; i < files.size(); ++i) {

        // Constant reference to the file name.
        const UString& filename(files[i]);

        // Get extension name from file name (without tslibext_).
        const UString name(BaseName(filename, TS_SHARED_LIB_SUFFIX).toRemovedPrefix(u"tslibext_", FileSystemCaseSensitivity));
        if (name.isContainedSimilarIn(ignore)) {
            // This extension is listed in TSLIBEXT_IGNORE.
            CERR.debug(u"ignoring extension \"%s\"", {filename});
        }
        else {
            // This extension shall be loaded.
            // Use the "permanent" load flag to make sure the shared library remains active.
            CERR.debug(u"loading extension \"%s\"", {filename});
            ApplicationSharedLibrary shlib(filename, UString(), UString(), SharedLibraryFlags::PERMANENT);
            if (!shlib.isLoaded()) {
                CERR.debug(u"failed to load extension \"%s\": %s", {filename, shlib.errorMessage()});
            }
        }
    }

    CERR.debug(u"loaded %d extensions", {DuckExtensionRepository::Instance()->_extensions.size()});
}

//----------------------------------------------------------------------------
// This constructor registers an extension.
//----------------------------------------------------------------------------

ts::DuckExtensionRepository::Register::Register(const UString& name,
                                                const UString& file_name,
                                                const UString& description,
                                                const UStringVector& plugins,
                                                const UStringVector& tools)
{
    CERR.debug(u"registering extension \"%s\"", {name});
    DuckExtensionRepository::Instance()->_extensions.push_back({ name, file_name, description, plugins, tools });
}


//----------------------------------------------------------------------------
// Search a file in a list of directories.
//----------------------------------------------------------------------------

namespace {
    ts::UString SearchFile(const ts::UStringList& dirs, const ts::UString& prefix, const ts::UString& name, const ts::UString& suffix)
    {
        for (const auto& it : dirs) {
            const ts::UString filename(it + ts::PathSeparator + prefix + name + suffix);
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
    for (const auto& ext : _extensions) {
        width = std::max(width, ext.name.width());
    }
    width++; // spacing after name

    // Search path for plugins.
    UStringList plugins_dirs;
    ApplicationSharedLibrary::GetSearchPath(plugins_dirs, TS_PLUGINS_PATH);

    // Search path for executables.
    UStringList tools_dirs;
    GetEnvironmentPath(tools_dirs, TS_COMMAND_PATH);

    // Build the output text as a string.
    UString out;
    for (const auto& ext : _extensions) {

        // First line: name and description.
        out.format(u"%s %s\n", {ext.name.toJustifiedLeft(width, u'.', false, 1), ext.description});

        if (report.verbose()) {
            // Display full file names.
            out.format(u"%*s Library: %s\n", {width, u"", ext.file_name});
            for (size_t i = 0; i < ext.plugins.size(); ++i) {
                out.format(u"%*s Plugin %s: %s\n", {width, u"", ext.plugins[i], SearchFile(plugins_dirs, u"tsplugin_", ext.plugins[i], TS_SHARED_LIB_SUFFIX)});
            }
            for (size_t i = 0; i < ext.tools.size(); ++i) {
                out.format(u"%*s Command %s: %s\n", {width, u"", ext.tools[i], SearchFile(tools_dirs, u"", ext.tools[i], TS_EXECUTABLE_SUFFIX)});
            }
        }
        else {
            // Only display plugins and tools names.
            if (!ext.plugins.empty()) {
                out.format(u"%*s Plugins: %s\n", {width, u"", UString::Join(ext.plugins)});
            }
            if (!ext.tools.empty()) {
                out.format(u"%*s Commands: %s\n", {width, u"", UString::Join(ext.tools)});
            }
        }
    }

    return out;
}
