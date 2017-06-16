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
//!  Application shared libraries
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSharedLibrary.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Representation of an application shared library.
    //!
    class TSDUCKDLL ApplicationSharedLibrary: public SharedLibrary
    {
    public:
        //!
        //! Constructor. 
        //!
        //! The load order is the following:
        //! - Default system lookup using filename string.
        //! - If filename is a base name (no directory), search it into the same directory as the executable.
        //! - Same as previous with specified prefix in base name.
        //! In all cases, if the filename does not contain a suffix, the standard system suffix (.so or .dll) is added.
        //!
        //! @param [in] filename Share library file name. Directory and suffix are optional.
        //! @param [in] prefix Prefix to add to @a filename if the file is not found.
        //! @param [in] permanent If false (the default), the shared library is unloaded from the current process
        //! when this object is destroyed. If true, the shared library remains active.
        //! @param [in,out] report Where to report errors.
        //!
        ApplicationSharedLibrary(const std::string& filename,
                                 const std::string& prefix = "",
                                 bool permanent = false,
                                 ReportInterface& report = NULLREP);

        //!
        //! The module name is derived from the file name without the prefix.
        //! @return The module name.
        //!
        std::string moduleName() const;

        //!
        //! Get the prefix.
        //! @return The file name prefix.
        //!
        std::string prefix() const {return _prefix;}

    private:
        std::string _prefix;
    };
}
