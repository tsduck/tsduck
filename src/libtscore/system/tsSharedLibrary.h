//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Shared library handling (.so on Linux, .dylib on macOS, .dll on Windows)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsEnumUtils.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Bit masks options to load shared libraries.
    //! @ingroup system
    //!
    enum class SharedLibraryFlags : uint16_t {
        NONE      = 0x00,  //!< No option
        PERMANENT = 0x01,  //!< The shared library remains active when the SharedLibrary object is destroyed (unloaded otherwise).
    };

    //!
    //! Shared library handling (.so on Linux, .dylib on macOS, .dll on Windows).
    //! @ingroup libtscore system
    //!
    class TSCOREDLL SharedLibrary
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
        explicit SharedLibrary(const fs::path& filename, SharedLibraryFlags flags = SharedLibraryFlags::NONE, Report& report = NULLREP);

        //!
        //! Destructor.
        //! Unload the shared library if @a permanent was false.
        //!
        virtual ~SharedLibrary();

        //!
        //! Check if the library was successfully loaded.
        //! @return True if the library was successfully loaded.
        //!
        bool isLoaded() const { return _is_loaded; }

        //!
        //! Return a message describing the constructor error.
        //! Useful when isLoaded() == false.
        //! @return An error message.
        //!
        const UString& errorMessage() const { return _error; }

        //!
        //! Return actual file name of shared library.
        //! @return The actual file name of shared library.
        //!
        const fs::path& fileName() const { return _filename; }

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
        void load(const fs::path& filename);

        //!
        //! Force unload, even if permanent
        //!
        void unload();

    private:
        // Private members
        Report&            _report;
        fs::path           _filename {};
        UString            _error {};
        bool               _is_loaded = false;
        SharedLibraryFlags _flags = SharedLibraryFlags::NONE;

#if defined(TS_WINDOWS)
        ::HMODULE _module = nullptr;
#else
        void* _dl = nullptr; // dlopen/dlclose handle
#endif
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::SharedLibraryFlags);
