//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Send application output to a "pager" application such as "more".
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! Send application output to a "pager" application such as "more" or "less".
    //!
    //! By default, the standard output and standard error are merged and sent
    //! through a pipe to a created process. Either standard output or error or
    //! both can be redirected. If any device to be redirected is not a terminal,
    //! the function fails.
    //!
    //! The created command can be specified using the environment variable @c PAGER.
    //! By default, the application searches for commands "less" and "more" in this order;
    //!
    //! @param [in,out] report Where to log "real errors" and debug messages.
    //! @param [in] useStdout Include standard output in the redirection.
    //! @param [in] useStderr Include standard error in the redirection.
    //! @param [in] envName Name of the optional environment variable containing the pager command name.
    //! @return True on success, false on error
    //!
    TSDUCKDLL bool OutputPager(Report& report = NULLREP,
                               bool useStdout = true,
                               bool useStderr = true,
                               const UString& envName = u"PAGER");
}
