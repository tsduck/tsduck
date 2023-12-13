//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Application shared libraries
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSharedLibrary.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Representation of an application shared library.
    //! @ingroup system
    //!
    class TSDUCKDLL ApplicationSharedLibrary: public SharedLibrary
    {
        TS_NOBUILD_NOCOPY(ApplicationSharedLibrary);
    public:
        //!
        //! Constructor.
        //! @param [in] filename Shared library file name. Directory and suffix are optional.
        //! If @a filename contains a directory, the specified file is used directly, with
        //! and without suffix (.so, .dll). If @a filename is just a name without directory,
        //! search the file in a list of directories as defined in GetSearchPath().
        //! In each directory, a file with @a prefix is searched. Then, if not found, without prefix.
        //! Finally, when everything failed, @a filename is searched with the default system lookup mechanism.
        //! @param [in] prefix Prefix to add to @a filename if the file is not found.
        //! @param [in] library_path Name of an environment variable, an optional list of directories to search,
        //! similar to @c LD_LIBRARY_PATH.
        //! @param [in] flags Shared library options.
        //! @param [in,out] report Where to report errors.
        //! @see GetSearchPath()
        //!
        explicit ApplicationSharedLibrary(const fs::path& filename,
                                          const UString& prefix = UString(),
                                          const UString& library_path = UString(),
                                          SharedLibraryFlags flags = SharedLibraryFlags::NONE,
                                          Report& report = NULLREP);

        //!
        //! Virtual destructor.
        //!
        virtual ~ApplicationSharedLibrary() override;

        //!
        //! The module name is derived from the file name without the prefix.
        //! @return The module name.
        //!
        UString moduleName() const;

        //!
        //! Get the prefix.
        //! @return The file name prefix.
        //!
        UString prefix() const { return _prefix; }

        //!
        //! Get the list of directories where to search application shared libraries or plugins.
        //! The ordered list of directories is:
        //! - All directories in @a library_path environment variable (if the name is not empty).
        //! - Directory of the current executable.
        //! - Directories ../lib64/tsduck and ../lib64 from current executable (64-bit UNIX only).
        //! - Directories ../lib/tsduck and ../lib from current executable (UNIX only).
        //! - All directories in %Path% environment variable (Windows only).
        //! @param [out] directories List of directories in search order.
        //! @param [in] library_path Name of an environment variable, an optional list of directories to search,
        //! similar to @c LD_LIBRARY_PATH.
        //!
        static void GetSearchPath(UStringList& directories, const UString& library_path = UString());

        //!
        //! Get a list of plugins.
        //! @param [out] files List of shared library files.
        //! @param [in] prefix Prefix for plugin names.
        //! @param [in] library_path Name of an environment variable, an optional list of directories to search, similar to @c LD_LIBRARY_PATH.
        //!
        static void GetPluginList(UStringVector& files, const UString& prefix, const UString& library_path = UString());

    private:
        UString _prefix {};
    };
}
