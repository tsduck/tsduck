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
//!  A mutex implementation which is compatible with the ts::Thread class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMutexInterface.h"
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
    class TSDUCKDLL Mutex: public MutexInterface
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
        virtual ~Mutex() override;

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
        virtual bool acquire(MilliSecond timeout = Infinite) override;

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
        virtual bool release() override;

    private:
        // Private members
        bool _created;
#if defined(TS_WINDOWS)
        ::HANDLE _handle;
#else
        ::pthread_mutex_t _mutex;
        friend class Condition;

        //!
        //! This method attempts an immediate pthread "try lock".
        //! @return true on success and false if can't lock now.
        //!
        bool tryLock();
#endif
    };
}
