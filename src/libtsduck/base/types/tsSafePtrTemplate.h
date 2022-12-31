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

#pragma once


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>::~SafePtr()
{
    if (_shared != nullptr && _shared->detach()) {
        _shared = nullptr;
    }
}
TS_POP_WARNING()


//----------------------------------------------------------------------------
// Assignment between safe pointers.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>& ts::SafePtr<T,MUTEX>::operator=(const SafePtr<T,MUTEX>& sp)
{
    if (_shared != sp._shared) {
        _shared->detach();
        _shared = sp._shared->attach();
    }
    return *this;
}

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>& ts::SafePtr<T,MUTEX>::operator=(SafePtr<T,MUTEX>&& sp) noexcept
{
    if (_shared != sp._shared) {
        if (_shared != nullptr) {
            _shared->detach();
        }
        _shared = sp._shared;
        sp._shared = nullptr;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Assignment from a standard pointer T*.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>& ts::SafePtr<T,MUTEX>::operator=(T* p)
{
    _shared->detach();
    _shared = new SafePtrShared(p);
    return *this;
}


//----------------------------------------------------------------------------
// Destructor. Deallocate actual object (if any).
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>::SafePtrShared::~SafePtrShared()
{
    if (_ptr != nullptr) {
        delete _ptr;
        _ptr = nullptr;
    }
}


//----------------------------------------------------------------------------
// Sets the pointer value to 0 and returns its old value.
// Do not deallocate the object.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
T* ts::SafePtr<T,MUTEX>::SafePtrShared::release()
{
    GuardMutex lock(_mutex);
    T* previous = _ptr;
    _ptr = nullptr;
    return previous;
}


//----------------------------------------------------------------------------
// Deallocate previous pointer and sets the pointer to specified value.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
void ts::SafePtr<T,MUTEX>::SafePtrShared::reset(T* p)
{
    GuardMutex lock(_mutex);
    if (_ptr != nullptr) {
        delete _ptr;
    }
    _ptr = p;
}


//----------------------------------------------------------------------------
// Get the pointer value.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
T* ts::SafePtr<T,MUTEX>::SafePtrShared::pointer()
{
    GuardMutex lock(_mutex);
    return _ptr;
}


//----------------------------------------------------------------------------
// Get the reference count value.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
int ts::SafePtr<T,MUTEX>::SafePtrShared::count()
{
    GuardMutex lock(_mutex);
    return _ref_count;
}


//----------------------------------------------------------------------------
// Check for null pointer on SafePtr object
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
bool ts::SafePtr<T,MUTEX>::SafePtrShared::isNull()
{
    GuardMutex lock(_mutex);
    return _ptr == nullptr;
}


//----------------------------------------------------------------------------
// Increment reference count and return this.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
typename ts::SafePtr<T,MUTEX>::SafePtrShared* ts::SafePtr<T,MUTEX>::SafePtrShared::attach()
{
    GuardMutex lock(_mutex);
    _ref_count++;
    return this;
}


//----------------------------------------------------------------------------
// Decrement reference count and deallocate this if needed.
// Return true is deleted, false otherwise.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
bool ts::SafePtr<T,MUTEX>::SafePtrShared::detach()
{
    int refcount;
    {
        GuardMutex lock(_mutex);
        refcount = --_ref_count;
    }
    if (refcount == 0) {
        delete this;
        return true;
    }
    return false;
}
