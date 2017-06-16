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
//!
//!  @file
//!  Shared library handling (.so on UNIX, DLL on Windows)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Shared library handling (.so on UNIX, DLL on Windows).
    //!
    class TSDUCKDLL SharedLibrary
    {
    public:
        //!
        //! File name extension of shared library file names (".so" on UNIX, ".dll" on Windows).
        //!
        static const char* const Extension;

        //!
        //! Constructor: Load a shared library
        //! @param [in] filename Shared library file name.
        //! @param [in] permanent If false (the default), the shared library is unloaded from the current process
        //! when this object is destroyed. If true, the shared library remains active.
        //! @param [in,out] report Where to report errors.
        //!
        SharedLibrary(const std::string& filename, bool permanent = false, ReportInterface& report = NULLREP);

        //!
        //! Destructor.
        //! Unload the shared library if @a permanent was false.
        //!
        virtual ~SharedLibrary();

        //!
        //! Check if the library was successfully loaded.
        //! @return True if the library was successfully loaded.
        //!
        bool isLoaded() const {return _is_loaded;}

        //!
        //! Return a message describing the constructor error.
        //! Useful when isLoaded() == false.
        //! @return An error message.
        //!
        const std::string& errorMessage() const {return _error;}

        //!
        //! Return actual file name of shared library.
        //! @return The actual file name of shared library.
        //!
        const std::string& fileName() const {return _filename;}

        //!
        //! Get the value of an exported symbol inside the shared library.
        //! @param [in] name Symbol name.
        //! @return The symbol value or 0 on error. When the symbol is an
        //! address, the returned value is a virtual memory address inside
        //! the current process.
        //!
        void* getSymbol(const std::string& name) const;

    protected:
        //!
        //! Try to load an alternate file if the shared library is not yet loaded.
        //! @param [in] filename Shared library file name.
        //!
        void load(const std::string& filename);

        //!
        //! Force unload, even if permanent
        //!
        void unload();

    private:
        // Unreachable ops
        SharedLibrary() = delete;
        SharedLibrary(const SharedLibrary&) = delete;
        SharedLibrary& operator=(const SharedLibrary&) = delete;

        // Private members
        ReportInterface& _report;
        std::string _filename;
        std::string _error;
        bool _is_loaded;
        bool _permanent;
#if defined (__windows)
        ::HMODULE _module;
#else
        void* _dl; // dlopen/dlclose handle
#endif
    };
}
