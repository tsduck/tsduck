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

#include "tsMutex.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::Mutex::Mutex () throw (MutexError) :
    _created (false)
{
#if defined (__windows)

    // Windows implementation.
    if ((_handle = ::CreateMutex(NULL, FALSE, NULL)) == NULL) {
        throw MutexError(::GetLastError());
        return;
    }

#else

    // POSIX pthread implementation
    int error;
    ::pthread_mutexattr_t attr;

    if ((error = ::pthread_mutexattr_init (&attr)) != 0) {
        throw MutexError ("mutex attr init", error);
    }
    if ((error = ::pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        throw MutexError ("mutex attr set type", error);
    }
    if ((error = ::pthread_mutex_init (&_mutex, &attr)) != 0) {
        throw MutexError ("mutex init", error);
    }
    if ((error = ::pthread_mutexattr_destroy (&attr)) != 0) {
        throw MutexError ("mutex attr destroy", error);
    }
    
#endif

    _created = true;
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Mutex::~Mutex ()
{
    if (_created) {
#if defined(__windows)
        ::CloseHandle(_handle);
#else
        ::pthread_mutex_destroy(&_mutex);
#endif
        _created = false;
    }
}


//----------------------------------------------------------------------------
// Acquire a mutex. Block until granted or timeout.
// Return true on success and false on error.
//----------------------------------------------------------------------------

bool ts::Mutex::acquire(MilliSecond timeout) throw(MutexError)
{
    if (!_created) {
        return false;
    }
    
#if defined(__windows)

    const ::DWORD wintimeout(timeout == Infinite ? INFINITE : ::DWORD(timeout));
    switch (::WaitForSingleObject(_handle, wintimeout)) {
        case WAIT_OBJECT_0:
        case WAIT_ABANDONED:
            // WAIT_ABANDONED means granted but after the previous owner thread
            // terminated without properly releasing the mutex.
            return true;
        case WAIT_TIMEOUT:
            // Successful call but not locked
            return false;
    }

#else

    int error;
    if (timeout == Infinite) {
        if ((error = ::pthread_mutex_lock (&_mutex)) == 0)
            return true; // success
        else
            throw MutexError ("mutex lock", error);
    }
    else if (timeout == 0) {
        if ((error = ::pthread_mutex_trylock (&_mutex)) == 0)
            return true; // success
        else if (error == EBUSY)
            return false; // mutex already locked
        else
            throw MutexError ("mutex lock", error);
    }
    else {
        ::timespec time;
        if ((error = ::clock_gettime (CLOCK_REALTIME, &time)) != 0)
            throw MutexError ("clock gettime error", - error);
        // Timeout absolute time in nano-seconds:
        int64_t nanoseconds = int64_t (time.tv_nsec) +
            int64_t (time.tv_sec) * NanoSecPerSec +
            timeout * 1000000;
        // Timeout absolute time as a struc timespec:
        time.tv_nsec = long (nanoseconds % NanoSecPerSec);
        time.tv_sec = time_t (nanoseconds / NanoSecPerSec);
        // Timed lock:
        if ((error = ::pthread_mutex_timedlock (&_mutex, &time)) == 0)
            return true; // success
        else if (error == ETIMEDOUT)
            return false; // not locked after timeout
        else
            throw MutexError ("mutex timed lock", error);
    }

#endif

    return false; // not granted
}


//----------------------------------------------------------------------------
// Release a lock.
// Return true on success and false on error.
//----------------------------------------------------------------------------

bool ts::Mutex::release()
{
    if (!_created) {
        return false;
    }
    else {
#if defined (__windows)
        return ::ReleaseMutex(_handle) != 0;
#else
        return ::pthread_mutex_unlock(&_mutex) == 0;
#endif
    }
}
