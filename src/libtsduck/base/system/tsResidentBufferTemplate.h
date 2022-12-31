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
//
//  Implementation of memory buffer locked in physical memory.
//  The template argument T is the basic element.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsIntegerUtils.h"
#include "tsSysInfo.h"
#include "tsFatal.h"

#if defined(TS_UNIX)
    #include "tsBeforeStandardHeaders.h"
    #include <sys/mman.h>
    #include "tsAfterStandardHeaders.h"
#endif


//----------------------------------------------------------------------------
// Constructor, based on required amount of T elements.
// Abort application is memory allocation fails.
// Do not abort is memory locking fails.
//----------------------------------------------------------------------------

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


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

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
