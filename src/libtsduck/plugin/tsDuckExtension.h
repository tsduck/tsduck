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
//!  Definition of a TSDuck Extension.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Definition of a TSDuck extension.
    //! @ingroup plugin
    //!
    //! A TSDuck Extension is a dynamically loaded shared library. It is typically
    //! provided by some third party. Unlike tsp plugins, a TSDuck extension
    //! shared library has no callable interface. Instead, it statically registers
    //! hooks inside the TSDuck library, when the TSDuck extension shared library
    //! is loaded.
    //!
    //! A TSDuck extension library is identified by the exported symbol
    //! "TSDuckExtensionId" which contains a pointed to a static instance of
    //! the class DuckExtension.
    //!
    //! The constructor is typically used inside a TSDuck library extension.
    //! The "getters" are typically used from TSDuck itself to identify the extensions.
    //!
    class TSDUCKDLL DuckExtension
    {
        TS_NOBUILD_NOCOPY(DuckExtension);
    public:
        //!
        //! Constructor.
        //! @param [in] name Extension name.
        //! @param [in] description One-line description of the extension.
        //! @param [in] plugins List of tsp plugin names which are provided by this extension.
        //! @param [in] tools List of tools (executables) which are provided by this extension.
        //!
        DuckExtension(const UString& name, const UString& description, const UStringVector& plugins = UStringVector(), const UStringVector& tools = UStringVector());

        //!
        //! A TSDuck extension shared library exports a symbol which contains a pointer to a constant DuckExtension instance.
        //!
        typedef const ts::DuckExtension* ConstPointer;

        //!
        //! Get the extension name.
        //! @return A constant reference to the extension name.
        //!
        const UString& name() const {return _name;}

        //!
        //! Get the one-line description of the extension.
        //! @return A constant reference to the one-line description of the extension.
        //!
        const UString& description() const {return _description;}

        //!
        //! Get the list of tsp plugins from this extension.
        //! @return A constant reference to the list of tsp plugins from this extension.
        //!
        const UStringVector& plugins() const {return _plugins;}

        //!
        //! Get the list of tools from this extension.
        //! @return A constant reference to the list of tools from this extension.
        //!
        const UStringVector& tools() const {return _tools;}

    private:
        const UString       _name;
        const UString       _description;
        const UStringVector _plugins;
        const UStringVector _tools;
    };
}


//!
//! @hideinitializer
//! Export the TSDuck extension out of the shared library.
//! All TSDuck extension shared libraries must invoke this macro once.
//! The parameters are the same as the ts::DuckExtension constructor.
//!
//! Sample usage, from one source file inside the extension library:
//! @code
//! TS_REGISTER_EXTENSION(u"foo", u"Manipulate FOO tables", {u"fooinject", u"fooextract"}, {u"foogen"});
//! @endcode
//!
#define TS_REGISTER_EXTENSION(...)                                               \
    namespace {                                                                  \
        const ts::DuckExtension _TSDuckExtensionId(__VA_ARGS__);                 \
    }                                                                            \
    extern "C" {                                                                 \
        TS_PUSH_WARNING()                                                        \
        TS_LLVM_NOWARNING(missing-variable-declarations)                         \
        TS_DLL_EXPORT                                                            \
        ts::DuckExtension::ConstPointer TSDuckExtensionId = &_TSDuckExtensionId; \
        TS_POP_WARNING()                                                         \
    }                                                                            \
    typedef int TS_UNIQUE_NAME(unused_to_allow_semicolon)
