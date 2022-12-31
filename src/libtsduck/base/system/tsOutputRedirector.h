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
        OutputRedirector(const UString& name,
                         Args& args,
                         std::ostream& stream = std::cout,
                         std::ios::openmode mode = std::ios::binary);

        //!
        //! Destructor, the output redirection is terminated and restore the previous stream.
        //!
        ~OutputRedirector();

    private:
        std::ostream&   _stream;
        std::streambuf* _previous;
        std::ofstream   _file;
    };
}
