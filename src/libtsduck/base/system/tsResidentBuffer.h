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
//!  Implementation of memory buffer locked in physical memory.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Implementation of memory buffer locked in physical memory.
    //! @tparam T Type of the buffer element.
    //! @ingroup system
    //!
    template <typename T = uint8_t>
    class ResidentBuffer
    {
        TS_NOBUILD_NOCOPY(ResidentBuffer);
    public:
        //!
        //! Constructor, based on required amount of elements.
        //! Abort application if memory allocation fails.
        //!
        //! Do not abort if memory locking fails. Some operating systems may place
        //! limitations on the amount of memory to lock. On DragonFlyBSD, the mlock()
        //! system call is reserved to the superuser and memory locking always fails
        //! with normal users. Consequently, failing to lock a memory buffer in
        //! physical memory is not a real error which prevents the application from
        //! working. At worst, there could be performance implications in case of
        //! page faults.
        //!
        //! @param [in] elem_count Number of @a T elements.
        //!
        ResidentBuffer(size_t elem_count);

        //!
        //! Destructor.
        //!
        ~ResidentBuffer();

        //!
        //! Check if the buffer is actually locked.
        //! @return True if the buffer is actually locked, false if locking failed.
        //!
        bool isLocked() const
        {
            return _is_locked;
        }

        //!
        //! Get error code when not locked
        //! @return The system error code when locking failed.
        //!
        SysErrorCode lockErrorCode() const
        {
            return _error_code;
        }

        //!
        //! Return base address of the buffer.
        //! @return The address of the first @a T element in the buffer.
        //!
        T* base() const
        {
            return _base;
        }

        //!
        //! Return the number of elements in the buffer.
        //! @return The number of @a T elements in the buffer.
        //!
        size_t count() const
        {
            return _elem_count;
        }

    private:
        char*        _allocated_base;   // First allocated address
        char*        _locked_base;      // First locked address (mlock, page boundary)
        T*           _base;             // Same as _locked_base with type T*
        size_t       _allocated_size;   // Allocated size (ts_malloc)
        size_t       _locked_size;      // Locked size (mlock, multiple of page size)
        size_t       _elem_count;       // Element count in locked region
        bool         _is_locked;        // False if mlock failed.
        SysErrorCode _error_code;       // Lock error code
    };
}

#include "tsResidentBufferTemplate.h"
