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
#include "tsIntegerUtils.h"
#include "tsSysInfo.h"
#include "tsFatal.h"

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
        bool isLocked() const { return _is_locked; }

        //!
        //! Get error code when not locked
        //! @return The system error code when locking failed.
        //!
        SysErrorCode lockErrorCode() const { return _error_code; }

        //!
        //! Return base address of the buffer.
        //! @return The address of the first @a T element in the buffer.
        //!
        T* base() const { return _base; }

        //!
        //! Return the number of elements in the buffer.
        //! @return The number of @a T elements in the buffer.
        //!
        size_t count() const { return _elem_count; }

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


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Constructor, based on required amount of T elements.
template <typename T>
ts::ResidentBuffer<T>::ResidentBuffer(size_t elem_count) :
    _allocated_base(nullptr),
    _locked_base(nullptr),
    _base(nullptr),
    _allocated_size(0),
    _locked_size(0),
    _elem_count(elem_count),
    _is_locked(false),
    _error_code(SYS_SUCCESS)
{
    const size_t requested_size = elem_count * sizeof(T);
    const size_t page_size = SysInfo::Instance()->memoryPageSize();

    // Allocate enough space to include memory pages around the requested size

    _allocated_size = requested_size + 2 * page_size;
    _allocated_base = new char[_allocated_size];

    // Locked space starts at next page boundary after allocated base:
    // Its size is the next multiple of page size after requested_size:
    // Be sure to use size_t (unsigned) instead of ptrdiff_t (signed)
    // to perform arithmetics on pointers because we use modulo operations.

    assert(sizeof(size_t) == sizeof(char_ptr));
    _locked_base = char_ptr(round_up(size_t(_allocated_base), page_size));
    _locked_size = round_up(requested_size, page_size);

    _base = new (_locked_base) T[elem_count];

    // Integrity checks

    assert(_allocated_base <= _locked_base);
    assert(_locked_base < _allocated_base + page_size);
    assert(_locked_base + _locked_size <= _allocated_base + _allocated_size);
    assert(requested_size <= _locked_size);
    assert(_locked_size <= _allocated_size);
    assert(size_t(_locked_base) % page_size == 0);
    assert(size_t(_locked_base) == size_t(_base));
    assert(char_ptr(_base + elem_count) <= _locked_base + _locked_size);
    assert(_locked_size % page_size == 0);

#if defined(TS_WINDOWS)

    // Windows implementation.

    // Get the current working set of the process.
    // If working set too low, try to extend working set.
    ::SIZE_T wsmin, wsmax;
    if (::GetProcessWorkingSetSize(::GetCurrentProcess(), &wsmin, &wsmax) == 0) {
        _error_code = LastSysErrorCode();
    }
    else if (size_t(wsmin) < 2 * _locked_size) {
        wsmin = ::SIZE_T(2 * _locked_size);
        wsmax = std::max(wsmax, ::SIZE_T(4 * _locked_size));
        if (::SetProcessWorkingSetSize(::GetCurrentProcess(), wsmin, wsmax) == 0) {
            _error_code = LastSysErrorCode();
        }
    }

    // Lock in virtual memory
    _is_locked = ::VirtualLock(_locked_base, _locked_size) != 0;
    if (!_is_locked && _error_code == SYS_SUCCESS) {
        _error_code = LastSysErrorCode();
    }

#else

    // UNIX implementation

    _is_locked = ::mlock(_locked_base, _locked_size) == 0;
    _error_code = _is_locked ? SYS_SUCCESS : LastSysErrorCode();

#endif
}

// Destructor
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename T>
ts::ResidentBuffer<T>::~ResidentBuffer()
{
    // Unlock from physical memory
    if (_is_locked) {
#if defined(TS_WINDOWS)
        ::VirtualUnlock(_locked_base, _locked_size);
#else
        ::munlock(_locked_base, _locked_size);
#endif
    }

    // Free memory
    if (_allocated_base != nullptr) {
        delete[] _allocated_base;
    }

    // Reset state (it explicit call of destructor)
    _allocated_base = nullptr;
    _locked_base = nullptr;
    _base = nullptr;
    _allocated_size = 0;
    _locked_size = 0;
    _elem_count = 0;
    _is_locked = false;
}
TS_POP_WARNING()
