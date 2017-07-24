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

#pragma once
#include "tsGuard.h"


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>::~SafePtr()
{
    if (_shared != 0 && _shared->detach ()) {
        _shared = 0;
    }
}


//----------------------------------------------------------------------------
// Assignment between safe pointers.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>& ts::SafePtr<T,MUTEX>::operator= (const SafePtr<T,MUTEX>& sp)
{
    if (_shared != sp._shared) {
        _shared->detach ();
        _shared = sp._shared->attach ();
    }
    return *this;
}


//----------------------------------------------------------------------------
// Assignment from a standard pointer T*.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>& ts::SafePtr<T,MUTEX>::operator=(T* p)
{
    _shared->detach ();
    _shared = new SafePtrShared (p);
    return *this;
}


//----------------------------------------------------------------------------
// Destructor. Deallocate actual object (if any).
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
ts::SafePtr<T,MUTEX>::SafePtrShared::~SafePtrShared()
{
    if (_ptr != 0) {
        delete _ptr;
        _ptr = 0;
    }
}


//----------------------------------------------------------------------------
// Sets the pointer value to 0 and returns its old value.
// Do not deallocate the object.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
T* ts::SafePtr<T,MUTEX>::SafePtrShared::release()
{
    Guard lock (_mutex);
    T* previous (_ptr);
    _ptr = 0;
    return previous;
}


//----------------------------------------------------------------------------
// Deallocate previous pointer and sets the pointer to specified value.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
void ts::SafePtr<T,MUTEX>::SafePtrShared::reset (T* p)
{
    Guard lock (_mutex);
    if (_ptr != 0) {
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
    Guard lock (_mutex);
    return _ptr;
}


//----------------------------------------------------------------------------
// Get the reference count value.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
int ts::SafePtr<T,MUTEX>::SafePtrShared::count()
{
    Guard lock (_mutex);
    return _ref_count;
}


//----------------------------------------------------------------------------
// Check for NULL on SafePtr object
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
bool ts::SafePtr<T,MUTEX>::SafePtrShared::isNull()
{
    Guard lock (_mutex);
    return _ptr == 0;
}


//----------------------------------------------------------------------------
// Perform a class upcast
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
template <typename ST>
ts::SafePtr<ST,MUTEX> ts::SafePtr<T,MUTEX>::SafePtrShared::upcast()
{
    Guard lock (_mutex);
    ST* sp = _ptr;
    _ptr = 0;
    return SafePtr<ST,MUTEX> (sp);
}


//----------------------------------------------------------------------------
// Perform a class downcast
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
template <typename ST>
ts::SafePtr<ST,MUTEX> ts::SafePtr<T,MUTEX>::SafePtrShared::downcast()
{
    Guard lock (_mutex);
#if defined (V_SYS_NO_RTTI)
    ST* sp = reinterpret_cast<ST*> (_ptr);
#else
    ST* sp = dynamic_cast<ST*> (_ptr);
#endif
    if (sp != 0) {
        // Successful downcast, the original safe pointer is released.
        _ptr = 0;
    }
    return SafePtr<ST,MUTEX> (sp);
}


//----------------------------------------------------------------------------
// Change mutex type.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
template <typename NEWMUTEX>
ts::SafePtr<T,NEWMUTEX> ts::SafePtr<T,MUTEX>::SafePtrShared::changeMutex()
{
    Guard lock (_mutex);
    T* sp = _ptr;
    _ptr = 0;
    return SafePtr<T,NEWMUTEX> (sp);
}


//----------------------------------------------------------------------------
// Increment reference count and return this.
//----------------------------------------------------------------------------

template <typename T, class MUTEX>
typename ts::SafePtr<T,MUTEX>::SafePtrShared* ts::SafePtr<T,MUTEX>::SafePtrShared::attach()
{
    Guard lock (_mutex);
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
        Guard lock (_mutex);
        refcount = --_ref_count;
    }
    if (refcount == 0) {
        delete this;
        return true;
    }
    return false;
}
