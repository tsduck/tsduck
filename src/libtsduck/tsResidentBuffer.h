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
//
//  Implementation of memory buffer locked in physical memory.
//  The template argument T is the basic element.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    template <typename T = uint8_t>
    class ResidentBuffer
    {
    public:
        // Constructor, based on required amount of T elements.
        // Abort application is memory allocation fails.
        // Do not abort is memory locking fails.
        ResidentBuffer (size_t elem_count);

        // Destructor
        ~ResidentBuffer();

        // Check if the buffer is actually locked.
        bool isLocked() const {return _is_locked;}

        // Get error code when not locked
        ErrorCode lockErrorCode() const {return _error_code;}

        // Return base and element count.
        T* base() const {return _base;}
        size_t count() const {return _elem_count;}

    private:
        // Unreachable constructors and operators.
        ResidentBuffer ();
        ResidentBuffer (const ResidentBuffer&);
        ResidentBuffer& operator= (const ResidentBuffer&);

        // Private members:
        char*     _allocated_base;   // First allocated address
        char*     _locked_base;      // First locked address (mlock, page boundary)
        T*        _base;             // Same as _locked_base with type T*
        size_t    _allocated_size;   // Allocated size (ts_malloc)
        size_t    _locked_size;      // Locked size (mlock, multiple of page size)
        size_t    _elem_count;       // Element count in locked region
        bool      _is_locked;        // False if mlock failed.
        ErrorCode _error_code;       // Lock error code
    };

}

#include "tsResidentBufferTemplate.h"
