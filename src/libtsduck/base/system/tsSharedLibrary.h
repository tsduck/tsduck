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
//!
//!  @file
//!  Shared library handling (.so on Linux, .dylib on macOS, .dll on Windows)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Bit masks options to load shared libraries.
    //!
    enum class SharedLibraryFlags : uint16_t {
        NONE      = 0x00,  //!< No option
        PERMANENT = 0x01,  //!< The shared library remains active when the SharedLibrary object is destroyed (unloaded otherwise).
    };

    //!
    //! Shared library handling (.so on Linux, .dylib on macOS, .dll on Windows).
    //! @ingroup system
    //!
    class TSDUCKDLL SharedLibrary
    {
        TS_NOBUILD_NOCOPY(SharedLibrary);
    public:
        //!
        //! Constructor: Load a shared library
        //! @param [in] filename Shared library file name.
        //! @param [in] flags Shared library options.
        //! @param [in,out] report Where to report errors.
        //! @see SharedLibraryFlags
        //!
        explicit SharedLibrary(const UString& filename, SharedLibraryFlags flags = SharedLibraryFlags::NONE, Report& report = NULLREP);

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
        const UString& errorMessage() const {return _error;}

        //!
        //! Return actual file name of shared library.
        //! @return The actual file name of shared library.
        //!
        const UString& fileName() const {return _filename;}

        //!
        //! Get the value of an exported symbol inside the shared library.
        //! @param [in] name Symbol name, using 8-bit characters, not Unicode.
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
        void load(const UString& filename);

        //!
        //! Force unload, even if permanent
        //!
        void unload();

    private:
        // Private members
        Report&            _report;
        UString            _filename;
        UString            _error;
        bool               _is_loaded;
        SharedLibraryFlags _flags;

#if defined(TS_WINDOWS)
        ::HMODULE _module;
#else
        void* _dl; // dlopen/dlclose handle
#endif
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::SharedLibraryFlags);
