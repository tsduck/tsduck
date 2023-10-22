//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface to write raw data on a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

namespace ts {
    //!
    //! Abstract interface to write raw data on a stream.
    //! @ingroup system
    //!
    class TSDUCKDLL AbstractWriteStreamInterface
    {
        TS_INTERFACE(AbstractWriteStreamInterface);
    public:
        //!
        //! Write data to the stream.
        //! All bytes are written to the stream, blocking or retrying when necessary.
        //! @param [in] addr Address of the data to write.
        //! @param [in] size Size in bytes of the data to write.
        //! @param [out] written_size Actually written size in bytes.
        //! Can be less than @a size in case of error in the middle of the write.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, Report& report) = 0;
    };
}
