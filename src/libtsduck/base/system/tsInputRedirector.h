//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input file redirector.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! A class to redirect an input stream.
    //! @ingroup system
    //!
    //! The constructor redirects a specific input stream (@c std::cin by default) from
    //! a given file. The destructor automatically restores the previous input stream.
    //!
    //! If the file name is empty, no redirection occurs, making this mechanism
    //! quite useful for optional redirection based on command line arguments.
    //!
    class TSDUCKDLL InputRedirector
    {
     TS_NOBUILD_NOCOPY(InputRedirector);
    public:
        //!
        //! Constructor, the input redirection is automatically started.
        //! @param [in] name File name from which the input is redirected.
        //! If empty, the input stream @a stream is not redirected.
        //! @param [in,out] args Used to report errors and exit application on error.
        //! @param [in,out] stream The input stream to redirect, @c std::cin by default.
        //! @param [in,out] mode Mode to use to open the file, @c std::ios::binary by default.
        //!
        InputRedirector(const fs::path& name,
                        Args& args,
                        std::istream& stream = std::cin,
                        std::ios::openmode mode = std::ios::binary);

        //!
        //! Destructor, the input redirection is terminated and restore the previous stream.
        //!
        ~InputRedirector();

    private:
        std::istream&   _stream;
        std::streambuf* _previous = nullptr;
        std::ifstream   _file {};
    };
}
