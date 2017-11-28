//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Transport stream processor: --list-processors option
//
//----------------------------------------------------------------------------

#include "tspListProcessors.h"
#include "tsPluginSharedLibrary.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// List all processors
//----------------------------------------------------------------------------

void ts::tsp::ListProcessors (Report& report)
{
    // Get list of shared library files

    StringVector files;
    ApplicationSharedLibrary::GetPluginList(files, "tsplugin_", TS_PLUGINS_PATH);

    // Build list of names and load shared libraries

    StringVector names(files.size());
    std::vector<PluginSharedLibraryPtr> shlibs(files.size());
    size_t name_width = 0;

    for (size_t i = 0; i < files.size(); ++i) {
        shlibs[i] = new PluginSharedLibrary(files[i], report);
        names[i] = shlibs[i]->moduleName();
        if (shlibs[i]->isLoaded()) {
            name_width = std::max(name_width, names[i].size());
        }
    }

    // List capabilities

    std::cerr << std::endl << "List of tsp input plugins:" << std::endl << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
        if (shlibs[i]->isLoaded() && shlibs[i]->new_input != 0) {
            Plugin* p = shlibs[i]->new_input(0);
            std::cerr << "  " << JustifyLeft(names[i] + ' ', name_width + 1, '.')
                      << " " << p->getDescription() << std::endl;
            delete p;
        }
    }

    std::cerr << std::endl << "List of tsp output plugins:" << std::endl << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
        if (shlibs[i]->isLoaded() && shlibs[i]->new_output != 0) {
            Plugin* p = shlibs[i]->new_output(0);
            std::cerr << "  " << JustifyLeft (names[i] + ' ', name_width + 1, '.')
                      << " " << p->getDescription() << std::endl;
            delete p;
        }
    }

    std::cerr << std::endl << "List of tsp packet processor plugins:" << std::endl << std::endl;
    for (size_t i = 0; i < files.size(); ++i) {
        if (shlibs[i]->isLoaded() && shlibs[i]->new_processor != 0) {
            Plugin* p = shlibs[i]->new_processor(0);
            std::cerr << "  " << JustifyLeft(names[i] + ' ', name_width + 1, '.')
                      << " " << p->getDescription() << std::endl;
            delete p;
        }
    }

    std::cerr << std::endl;
}
