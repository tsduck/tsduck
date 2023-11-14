//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A mutex implementation which is compatible with the ts::Thread class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsException.h"

namespace ts {
    //!
    //! A mutex implementation which is compatible with the ts::Thread class.
    //! @ingroup thread
    //!
    //! The concrete class ts::Mutex is a mutex implementation which
    //! is compatible with the ts::Thread class. This means that concurrent
    //! instances of ts::Thread can synchronize mutual exclusion on shared
    //! resources using instances of ts::Mutex.
    //!
    //! This mutex implementation is @e recursive, i.e. nested pairs of
    //! acquire() / release() are allowed on the instances. There must be
    //! exactly one release() for each nested invocation of acquire().
    //!
    //! The implementation of this class is operating system dependent,
    //! just like ts::Thread.
    //!
    //! Note: With the adoption of C++11 as base requirement for TSDuck, this
    //! class shall no longer be used for new code. Use the C++11 standard
    //! concurrency classes.
    //!
    class TSDUCKDLL Mutex
    {
        TS_NOCOPY(Mutex);
    public:
        //!
        //! Fatal low-level mutex error.
        //!
        TS_DECLARE_EXCEPTION(MutexError);

        //!
        //! Default constructor.
        //!
        //! @throw MutexError In case of operating system error,
        //! when the underlying system objects could not be created.
        //!
        Mutex();

        //!
        //! Destructor.
        //!
        ~Mutex();

        //!
        //! Acquire the mutex with a timeout.
        //!
        //! If the mutex is already acquired by another instance of ts::Thread,
        //! acquire() hangs until the mutex is released or the timeout expires,
        //! whichever comes first.
        //!
        //! If the mutex is already acquired by the current ts::Thread, acquire()
        //! completes immediately. For each successful call to acquire() in the
        //! current ts::Thread, there must be exactly one call to release().
        //!
        //! If a ts::Thread which holds a mutex terminates before it has released
        //! the mutex, this mutex is automatically released.
        //!
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @return True on success and false on error or when the timeout expires.
        //!
        bool acquire(MilliSecond timeout = Infinite);

        //!
        //! Release the mutex.
        //!
        //! For each successful call to acquire(), there must be exactly one call to release().
        //!
        //! @b Important: If a mutex is destroyed while it is still acquired, the results are
        //! unpredictible. Experience has shown than destroying a mutex while it is acquired
        //! is harmless on Windows, macOS and Linux with glibc. However, on Linux with musl
        //! libc (Alpine Linux for instance), failing to call release() as many times as
        //! acquire() leads to random memory corruptions and crashes.
        //!
        //! For this reason, it is recommended to never use acquire() and release() directly
        //! and use the GuardMutex class instead.
        //!
        //! @return true on success and false on error.
        //!
        bool release();

    private:
        // Private members
        bool _created = false;
#if defined(TS_WINDOWS)
        ::HANDLE _handle = INVALID_HANDLE_VALUE;
#else
        TS_PUSH_WARNING()
        TS_GCC_NOWARNING(zero-as-null-pointer-constant)
        ::pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
        TS_POP_WARNING()

        friend class Condition;

        //!
        //! This method attempts an immediate pthread "try lock".
        //! @return true on success and false if can't lock now.
        //!
        bool tryLock();
#endif
    };
}
