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
//!  Abstract interface to read raw data from a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! Abstract interface to read raw data from a stream.
    //! @ingroup system
    //!
    class TSDUCKDLL AbstractReadStreamInterface
    {
    public:
        //!
        //! Read partial data from the stream.
        //! Wait and read at least one byte. Don't try to read exactly @a max_size bytes.
        //! If @a ret_size is less than @a max_bytes, it is possible to read more.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [out] ret_size Returned input size in bytes.
        //! If zero, end of file has been reached or an error occurred.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool readStreamPartial(void* addr, size_t max_size, size_t& ret_size, Report& report) = 0;

        //!
        //! Read complete data from the stream.
        //! Wait and read exactly @a max_size bytes. If @a ret_size is less than @a max_bytes,
        //! it is not possible to read more. End of file has probably been reached.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [out] ret_size Returned input size in bytes.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool readStreamComplete(void* addr, size_t max_size, size_t& ret_size, Report& report);

        //!
        //! Read chunks of data from the stream.
        //! @param [out] addr Address of the buffer for the incoming data.
        //! @param [in] max_size Maximum size in bytes of the buffer.
        //! @param [in] chunk_size If not zero, make sure that the input size is always
        //! a multiple of @a chunk_size. If the initial read ends in the middle of a @e chunk,
        //! read again and again, up to the end of the current chunk or end of file.
        //! @param [out] ret_size Returned input size in bytes.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool readStreamChunks(void* addr, size_t max_size, size_t chunk_size, size_t& ret_size, Report& report);

        //!
        //! Check if the end of stream was reached.
        //! @return True on end of stream, false otherwise.
        //!
        virtual bool endOfStream() = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~AbstractReadStreamInterface();
    };
}
