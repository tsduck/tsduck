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

#include "tsMutex.h"
#include "tsTime.h"
#include "tsMemory.h"

// Clang can be too clever here.
// We encapsulate the calls and these checks cannot be done in this module.
// This warning has been seen on FreeBSD only.
TS_LLVM_NOWARNING(thread-safety-analysis)
TS_LLVM_NOWARNING(thread-safety-negative)

// On MacOS, we must do polling on mutex "lock with timeout".
// We use 10 ms, expressed in nanoseconds.
#if defined(TS_MAC)
    #define MUTEX_POLL_NANOSEC (10 * NanoSecPerMilliSec)
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

// The macro PTHREAD_MUTEX_INITIALIZER uses several zeroes as pointers.
TS_PUSH_WARNING()
TS_GCC_NOWARNING(zero-as-null-pointer-constant)

ts::Mutex::Mutex() :
    _created(false),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE)
#else
    _mutex(PTHREAD_MUTEX_INITIALIZER)
#endif
{
    TS_POP_WARNING()

#if defined(TS_WINDOWS)

    // Windows implementation.
    if ((_handle = ::CreateMutex(NULL, false, NULL)) == NULL) {
        throw MutexError(::GetLastError());
        return;
    }

#else

    // POSIX pthread implementation
    int error = 0;
    ::pthread_mutexattr_t attr;
    TS_ZERO(attr);

    if ((error = ::pthread_mutexattr_init(&attr)) != 0) {
        throw MutexError(u"mutex attr init", error);
    }
    if ((error = ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        throw MutexError(u"mutex attr set type", error);
    }
    if ((error = ::pthread_mutex_init(&_mutex, &attr)) != 0) {
        throw MutexError(u"mutex init", error);
    }
    if ((error = ::pthread_mutexattr_destroy(&attr)) != 0) {
        throw MutexError(u"mutex attr destroy", error);
    }

#endif

    _created = true;
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::Mutex::~Mutex()
{
    if (_created) {
#if defined(TS_WINDOWS)
        ::CloseHandle(_handle);
#else
        ::pthread_mutex_destroy(&_mutex);
#endif
        _created = false;
    }
}


//----------------------------------------------------------------------------
// This method attempts an immediate pthread "try lock".
//----------------------------------------------------------------------------

#if defined(TS_UNIX)
bool ts::Mutex::tryLock()
{
    const int error = ::pthread_mutex_trylock(&_mutex);
    if (error == 0) {
        return true; // success, locked
    }
    else if (error == EBUSY) {
        return false; // mutex already locked
    }
    else {
        throw MutexError(u"mutex try lock", error);
    }
}
#endif


//----------------------------------------------------------------------------
// Acquire a mutex. Block until granted or timeout.
// Return true on success and false on error.
//----------------------------------------------------------------------------

bool ts::Mutex::acquire(MilliSecond timeout)
{
    if (!_created) {
        return false;
    }

#if defined(TS_WINDOWS)

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
        // Unconditional lock, wait forever if necessary.
        if ((error = ::pthread_mutex_lock(&_mutex)) == 0) {
            return true; // success
        }
        else {
            throw MutexError(u"mutex lock", error);
        }
    }
    else if (timeout == 0) {
        // Immediate "try lock".
        return tryLock();
    }
    else {
        // Non-zero finite timeout.
#if defined(TS_MAC)
        // Mac implementation of POSIX does not include pthread_mutex_timedlock.
        // We have to fall back to the infamous method of polling :((
        // Timeout in absolute time:
        const NanoSecond due = Time::UnixClockNanoSeconds(CLOCK_REALTIME, timeout);
        for (;;) {
            // Poll once, try to lock.
            if (tryLock()) {
                return true; // locked
            }
            // How many nanoseconds until due time:
            NanoSecond remain = due - Time::UnixClockNanoSeconds(CLOCK_REALTIME);
            if (remain <= 0) {
                return false; // could not lock before timeout
            }
            // Sleep time:
            remain = std::min<NanoSecond>(remain, MUTEX_POLL_NANOSEC);
            ::timespec tspec;
            tspec.tv_sec = time_t(remain / NanoSecPerSec);
            tspec.tv_nsec = long(remain % NanoSecPerSec);
            if (::nanosleep(&tspec, nullptr) < 0 && errno != EINTR) {
                // Actual error, not interrupted by a signal
                throw MutexError(u"nanosleep error", errno);
            }
        }
#else
        // Standard real-time POSIX implementation using pthread_mutex_timedlock.
        // Timeout absolute time as a struct timespec:
        ::timespec time;
        Time::GetUnixClock(time, CLOCK_REALTIME, timeout);
        // Timed lock:
        if ((error = ::pthread_mutex_timedlock(&_mutex, &time)) == 0) {
            return true; // success
        }
        else if (error == ETIMEDOUT) {
            return false; // not locked after timeout
        }
        else {
            throw MutexError(u"mutex timed lock", error);
        }
#endif
    }

#endif

    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(unreachable-code-return)
    return false; // not granted
    TS_POP_WARNING()
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
#if defined(TS_WINDOWS)
        return ::ReleaseMutex(_handle) != 0;
#else
        return ::pthread_mutex_unlock(&_mutex) == 0;
#endif
    }
}
