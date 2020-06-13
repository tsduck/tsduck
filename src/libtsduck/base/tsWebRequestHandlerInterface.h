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
//!  Web request handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class WebRequest;

    //!
    //! Web request handler interface.
    //! @ingroup net
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified of Web data transfer.
    //!
    class TSDUCKDLL WebRequestHandlerInterface
    {
    public:
        //!
        //! This hook is invoked at the beginning of the transfer.
        //! The application may inspect the response headers from @a request.
        //! @param [in] request The Web request.
        //! @param [in] size Potential content size in bytes. This size is just
        //! a hint, not a guaranteed size. Zero if the content size is unknown.
        //! @return True to proceed, false to abort the transfer.
        //!
        virtual bool handleWebStart(const WebRequest& request, size_t size);

        //!
        //! This hook is invoked when a data chunk is available.
        //! @param [in] request The Web request.
        //! @param [in] data Address of data chunk.
        //! @param [in] size Size of data chunk.
        //! @return True to proceed, false to abort the transfer.
        //!
        virtual bool handleWebData(const WebRequest& request, const void* data, size_t size) = 0;

        //!
        //! This hook is invoked at the end of the transfer.
        //! @param [in] request The Web request.
        //! @return True to indicate success, false to indicate error.
        //!
        virtual bool handleWebStop(const WebRequest& request);

        //!
        //! Virtual destructor.
        //!
        virtual ~WebRequestHandlerInterface() = default;
    };
}
