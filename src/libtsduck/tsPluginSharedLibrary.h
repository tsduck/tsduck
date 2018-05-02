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
//!
//!  @file
//!  TSP plugin shared libraries
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"
#include "tsApplicationSharedLibrary.h"
#include "tsCerrReport.h"
#include "tsNullMutex.h"
#include "tsSafePtr.h"

namespace ts {
    //!
    //! Representation of a TSP plugin shared library.
    //! @ingroup plugin
    //!
    class TSDUCKDLL PluginSharedLibrary: public ApplicationSharedLibrary
    {
    public:
        //!
        //! Constructor.
        //!
        //! When the load is successful, the API version has been successfully checked
        //! and the tsp plugin API has been located.
        //!
        //! @param [in] filename Share library file name. Directory, "tsplugin_" prefix and suffix are optional.
        //! @param [in,out] report Where to report errors.
        //!
        explicit PluginSharedLibrary(const UString& filename, Report& report = CERR);

        //!
        //! Input plugin allocation function.
        //! If null, the plugin either does not provide input capability or is not a valid TSP plugin.
        //!
        NewInputProfile new_input;

        //!
        //! Output plugin allocation function.
        //! If null, the plugin either does not provide output capability or is not a valid TSP plugin.
        //!
        NewOutputProfile new_output;

        //!
        //! Packet processing plugin allocation function.
        //! If null, the plugin either does not provide packet processing capability or is not a valid TSP plugin.
        //!
        NewProcessorProfile new_processor;

    private:
        // Unreachable operations.
        PluginSharedLibrary() = delete;
        PluginSharedLibrary(const PluginSharedLibrary&) = delete;
        PluginSharedLibrary& operator=(const PluginSharedLibrary&) = delete;
    };

    //!
    //! Safe pointer for PluginSharedLibrary (not thread-safe).
    //!
    typedef SafePtr <PluginSharedLibrary, NullMutex> PluginSharedLibraryPtr;
}
