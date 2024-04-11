//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

ts::DuckExtensionRepository::DuckExtensionRepository()
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
    CERR.debug(u"%d extensions ignored", ignore.size());

    // Get list of shared library files
    UStringVector files;
    ApplicationSharedLibrary::GetPluginList(files, u"tslibext_", PLUGINS_PATH_ENVIRONMENT_VARIABLE);
    CERR.debug(u"found %d possible extensions", files.size());

    // Load all plugins shared library.
    for (size_t i = 0; i < files.size(); ++i) {

        // Constant reference to the file name.
        const UString& filename(files[i]);

        // Get extension name from file name (without tslibext_).
        const UString name(BaseName(filename, SHARED_LIBRARY_SUFFIX).toRemovedPrefix(u"tslibext_", FILE_SYSTEM_CASE_SENSITVITY));
        if (name.isContainedSimilarIn(ignore)) {
            // This extension is listed in TSLIBEXT_IGNORE.
            CERR.debug(u"ignoring extension \"%s\"", filename);
        }
        else {
            // This extension shall be loaded.
            // Use the "permanent" load flag to make sure the shared library remains active.
            CERR.debug(u"loading extension \"%s\"", filename);
            ApplicationSharedLibrary shlib(filename, UString(), UString(), SharedLibraryFlags::PERMANENT);
            if (!shlib.isLoaded()) {
                CERR.debug(u"failed to load extension \"%s\": %s", filename, shlib.errorMessage());
            }
        }
    }

    CERR.debug(u"loaded %d extensions", DuckExtensionRepository::Instance()._extensions.size());
}

//----------------------------------------------------------------------------
// This constructor registers an extension.
//----------------------------------------------------------------------------

ts::DuckExtensionRepository::Register::Register(const UString& name,
                                                const fs::path& file_name,
                                                const UString& description,
                                                const UStringVector& plugins,
                                                const UStringVector& tools)
{
    CERR.debug(u"registering extension \"%s\"", name);
    DuckExtensionRepository::Instance()._extensions.push_back({name, file_name, description, plugins, tools});
}


//----------------------------------------------------------------------------
// Search a file in a list of directories.
//----------------------------------------------------------------------------

namespace {
    ts::UString SearchFile(const ts::UStringList& dirs, const ts::UString& prefix, const ts::UString& name, const ts::UString& suffix)
    {
        for (const auto& it : dirs) {
            const ts::UString filename(it + fs::path::preferred_separator + prefix + name + suffix);
            if (fs::exists(filename)) {
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
    ApplicationSharedLibrary::GetSearchPath(plugins_dirs, PLUGINS_PATH_ENVIRONMENT_VARIABLE);

    // Search path for executables.
    UStringList tools_dirs;
    GetEnvironmentPath(tools_dirs, PATH_ENVIRONMENT_VARIABLE);

    // Build the output text as a string.
    UString out;
    for (const auto& ext : _extensions) {

        // First line: name and description.
        out.format(u"%s %s\n", ext.name.toJustifiedLeft(width, u'.', false, 1), ext.description);

        if (report.verbose()) {
            // Display full file names.
            out.format(u"%*s Library: %s\n", width, u"", ext.file_name);
            for (size_t i = 0; i < ext.plugins.size(); ++i) {
                out.format(u"%*s Plugin %s: %s\n", width, u"", ext.plugins[i], SearchFile(plugins_dirs, u"tsplugin_", ext.plugins[i], SHARED_LIBRARY_SUFFIX));
            }
            for (size_t i = 0; i < ext.tools.size(); ++i) {
                out.format(u"%*s Command %s: %s\n", width, u"", ext.tools[i], SearchFile(tools_dirs, u"", ext.tools[i], EXECUTABLE_FILE_SUFFIX));
            }
        }
        else {
            // Only display plugins and tools names.
            if (!ext.plugins.empty()) {
                out.format(u"%*s Plugins: %s\n", width, u"", UString::Join(ext.plugins));
            }
            if (!ext.tools.empty()) {
                out.format(u"%*s Commands: %s\n", width, u"", UString::Join(ext.tools));
            }
        }
    }

    return out;
}
