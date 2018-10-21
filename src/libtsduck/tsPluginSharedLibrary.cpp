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
//
//  TSP plugin shared libraries
//
//----------------------------------------------------------------------------

#include "tsPluginSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor. The load order is the following:
// - Default system lookup using filename string.
// - If filename is a base name (no directory), search it into the
//   same directory as the executable.
// - Same as previous with "tsplugin_" prefix in base name.
// In all cases, if the filename does not contain a suffix, the standard
// system suffix (.so or .dll) is added.
// When the load is successful, locate tsp plugin API and check the API version.
//----------------------------------------------------------------------------

ts::PluginSharedLibrary::PluginSharedLibrary(const UString& filename, Report& report) :
    ApplicationSharedLibrary(filename, u"tsplugin_", TS_PLUGINS_PATH, true, report),
    new_input(nullptr),
    new_output(nullptr),
    new_processor(nullptr)
{
    // If still not loaded, error
    if (!isLoaded()) {
        report.error(errorMessage());
        return;
    }

    // Locate and check the API version
    const UString path(fileName());
    const int* version = reinterpret_cast<const int*>(getSymbol("tspInterfaceVersion"));

    if (version == nullptr) {
        report.error(u"no symbol tspInterfaceVersion in " + path);
        unload();
        return;
    }

    if (*version != TSP::API_VERSION) {
        report.error(u"incompatible API version %d in %s, expected %d", {*version, path, TSP::API_VERSION});
        unload();
        return;
    }

    // Load the plugin entry points
    new_input = reinterpret_cast<NewInputProfile>(getSymbol("tspNewInput"));
    new_output = reinterpret_cast<NewOutputProfile>(getSymbol("tspNewOutput"));
    new_processor = reinterpret_cast<NewProcessorProfile>(getSymbol("tspNewProcessor"));
}
