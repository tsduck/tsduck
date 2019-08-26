//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

        // Get extension name from file name (without tslibext_).
        const UString name(BaseName(files[i], TS_SHARED_LIB_SUFFIX).toRemovedPrefix(u"tslibext_", FileSystemCaseSensitivity));
        if (name.containSimilar(ignore)) {
            // This extension is listed in TSLIBEXT_IGNORE.
            EXTDEBUG("ignoring extension" << files[i]);
        }
        else {
            // This extension shall be loaded.
            EXTDEBUG("loading extension " << files[i]);
            ApplicationSharedLibrary shlib(files[i]);
            if (!shlib.isLoaded()) {
                EXTDEBUG("failed to load extension " << files[i] << " : " << shlib.errorMessage());
            }
            else {
                // Finding TSDuckExtensionId symbol in the shared library.
                void* sym = shlib.getSymbol("TSDuckExtensionId");
                if (sym == nullptr) {
                    EXTDEBUG("no symbol TSDuckExtensionId found in " << files[i]);
                }
                else {
                    // The returned address is the address of a pointer to ts::DuckExtension.
                    // See macro TS_DECLARE_EXTENSION.
                    ts::DuckExtension::ConstPointer ext = *reinterpret_cast<ts::DuckExtension::ConstPointer*>(sym);
                    if (ext != nullptr) {
                        // Now the extension is fully identified.
                        _extensions.push_back(ext);
                        EXTDEBUG("extension \"" << ext->name() << "\" loaded from " << files[i]);
                    }
                }
            }
        }
    }

    EXTDEBUG("loaded " << _extensions.size() << " extensions");

#undef EXTDEBUG
}


//----------------------------------------------------------------------------
// List all extensions.
//----------------------------------------------------------------------------

ts::UString ts::DuckExtensionRepository::listExtensions(ts::Report& report)
{
    return UString(); //@@@@@@@@@@@@@@
}
