//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_INTERFACE(AbstractReadStreamInterface);
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
    };
}
