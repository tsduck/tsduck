//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Output file redirector.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! A class to redirect an output stream.
    //! @ingroup system
    //!
    //! The constructor redirects a specific output stream (@c std::cout by default) to
    //! a given file. The destructor automatically restores the previous output stream.
    //!
    //! If the file name is empty, no redirection occurs, making this mechanism
    //! quite useful for optional redirection based on command line arguments.
    //!
    class TSDUCKDLL OutputRedirector
    {
        TS_NOBUILD_NOCOPY(OutputRedirector);
    public:
        //!
        //! Constructor, the output redirection is automatically started.
        //! @param [in] name File name to which the output is redirected.
        //! If empty, the output stream @a stream is not redirected.
        //! @param [in,out] args Used to report errors and exit application on error.
        //! @param [in,out] stream The output stream to redirect, @c std::cout by default.
        //! @param [in,out] mode Mode to use to open the file, @c std::ios::binary by default.
        //!
        OutputRedirector(const fs::path& name,
                         Args& args,
                         std::ostream& stream = std::cout,
                         std::ios::openmode mode = std::ios::binary);

        //!
        //! Destructor, the output redirection is terminated and restore the previous stream.
        //!
        ~OutputRedirector();

    private:
        std::ostream&   _stream;
        std::streambuf* _previous = nullptr;
        std::ofstream   _file {};
    };
}
