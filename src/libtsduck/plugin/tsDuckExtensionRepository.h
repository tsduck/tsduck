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
//!
//!  @file
//!  TSDusk extensions repository
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDuckExtension.h"
#include "tsSingletonManager.h"
#include "tsReport.h"

namespace ts {
    //!
    //! A repository of TSDuck extensions.
    //! @ingroup plugin
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    //! The extensions repository is responsible for statically loading all available extensions.
    //! The extension shared libraries are all shared libraries named "tslibext_*", using the
    //! same search rules as tsp plugins.
    //!
    //! Since this operation occurs before entering the main code of any executable using the
    //! TSDuck library, the application has no control over the loading of extensions.
    //! The following environment variables can be defined to alter the loading of extensions:
    //!
    //! - TSLIBEXT_DEBUG : If defined and not empty, display debug messages on the standard error.
    //! - TSLIBEXT_NONE : If defined and not empty, do not load any extension.
    //! - TSLIBEXT_IGNORE : A comma-separated list of extensions to ignore (useful when one
    //!   extension creates problems when loaded).
    //!
    class TSDUCKDLL DuckExtensionRepository
    {
        TS_DECLARE_SINGLETON(DuckExtensionRepository);
    public:
        //!
        //! Get the number of loaded extensions.
        //! @return The number of loaded extensions.
        //!
        size_t extensionCount() const { return _extensions.size(); }

        //!
        //! List all loaded extensions.
        //! This function is typically used to implement the <code>tsversion -\-list-extensions</code> option.
        //! @param [in,out] report Where to report errors. Used to get verbosity level.
        //! @return The text to display.
        //!
        UString listExtensions(Report& report);

    private:
        std::vector<std::pair<DuckExtension::ConstPointer, UString>> _extensions;
    };
}
