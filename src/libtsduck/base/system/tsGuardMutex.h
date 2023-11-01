//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Automatic guard class for mutex (ts::MutexInterface).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullMutex.h"
#include "tsException.h"
#include "tsFatal.h"

namespace ts {
    //!
    //! Automatic guard class for mutex.
    //! @ingroup thread
    //!
    //! This is a template class which can be instantiated using any class which
    //! implements acquire() and release() as declared in classes Mutex and NullMutex.
    //! @tparam MUTEX A mutex class with acquire() and release() methods.
    //!
    //! Previously, Mutex and NullMutex where subclasses of a common MutexInterface
    //! class which defined acquire() and release() as virtual methods. GuardMutex
    //! was a non-template class using MutexInterface. However, the downside of
    //! virtual classes is that the virtual methods were called, even with NullMutex.
    //! This creates a useless overhead in non thread-safe usage of GuardMutex.
    //! To eliminate this overhead, Mutex and NullMutex are no longer virtual classes,
    //! they no longer share a common superclass, and GuardMutex became a template class.
    //! @see GuardMutex
    //!
    //! This class implements the @e guard design pattern for mutex.
    //! The common pitfall in the usage of resources which must be explicitly
    //! released after having been acquired is the absence of release.
    //! This can be an omission in the code (no invocation of @c release() at all)
    //! or a premature exit from the code sequence between the acquire and
    //! release operations (exception or @c return statement).
    //!
    //! Example:
    //! @code
    //! Mutex mutex;
    //! ...
    //! mutex.acquire();
    //! ...
    //! // some exception occurs here and release() is never invoked.
    //! ...
    //! mutex.release();
    //! @endcode
    //!
    //! The @e guard design pattern mitigates this risk. Each critical
    //! sequence is a code block. Within the code block, an ancillary
    //! object, the @e guard, is created and refers to the common mutex
    //! object. The constructor of the guard automatically acquires the
    //! mutex and its destructor automatically releases the mutex.
    //!
    //! There is no need for an explicit release of the mutex.
    //! If there is an exception or a premature exit of the code block,
    //! the C++ language guarantees that the destructor of the guard
    //! will be invoked and that the mutex will be released in all cases.
    //!
    //! Example:
    //! @code
    //! Mutex mutex;
    //! ...
    //! {
    //!     MutexGuard guard(mutex); // mutex acquired
    //!     ...
    //!     // some exception occurs here, no problem, don't worry
    //!     ...
    //! } // guard's destructor invoked, mutex always released
    //! @endcode
    //!
    template <class MUTEX>
    class TemplateGuardMutex
    {
        TS_NOBUILD_NOCOPY(TemplateGuardMutex);
    public:
        //!
        //! Fatal low-level mutex guard error.
        //!
        TS_DECLARE_EXCEPTION(GuardMutexError);

        //!
        //! Constructor, automatically acquire the mutex with a timeout.
        //!
        //! With a non-infinite timeout, there is no guarantee that the mutex is
        //! locked after construction. The user has to invoke isLocked() to check
        //! that the mutex was actually acquired before the timeout expired.
        //!
        //! @param [in,out] mutex A reference on the mutex object to acquire.
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex.
        //! @exception ts::GuardMutex::GuardMutexError Thrown whenever an error occurs
        //! during the acquisition of the mutex. Exceeding the timeout is not
        //! error, the object is successfully constructed but isLocked() will
        //! return false.
        //!
        TemplateGuardMutex(MUTEX& mutex, MilliSecond timeout = Infinite);

        //!
        //! Destructor, automatically release the mutex.
        //!
        //! @exception ts::GuardMutex::GuardMutexError Thrown whenever an error occurs
        //! during the release of the mutex, i.e. when
        //! ts::MutexInterface::release() returns false.
        //!
        ~TemplateGuardMutex();

        //!
        //! Check if the mutex was actually locked.
        //!
        //! This method is useful only with the object was constructed with a
        //! timeout. When the constructor without timeout was used, this method
        //! always return true.
        //!
        //! @return True if the mutex was successfully acquired and false if the timeout expired.
        //!
        bool isLocked() const { return _is_locked; }

        //!
        //! Force an early unlock of the mutex.
        //! @return True if the mutex has been successfully unlocked.
        //!
        bool unlock();

    private:
        MUTEX& _mutex;
        bool _is_locked = false;
    };

    class Mutex;

    //!
    //! Instantiation of TemplateGuardMutex on Mutex.
    //!
    typedef TemplateGuardMutex<Mutex> GuardMutex;

    // Template specialization on NullMutex: reduce overhead to nothing.
    //! @cond doxygen
    template<> class TemplateGuardMutex<NullMutex>
    {
    public:
        TemplateGuardMutex(NullMutex& mutex, MilliSecond timeout = Infinite) {}
    };
    //! @endcond
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Constructor
template <class MUTEX>
ts::TemplateGuardMutex<MUTEX>::TemplateGuardMutex(MUTEX& mutex, MilliSecond timeout) :
    _mutex(mutex)
{
    _is_locked = mutex.acquire(timeout);
    if (timeout == Infinite && !_is_locked) {
        throw GuardMutexError(u"failed to acquire mutex");
    }
}

// Destructor
TS_PUSH_WARNING()
TS_LLVM_NOWARNING(dtor-name)
template <class MUTEX>
ts::TemplateGuardMutex<MUTEX>::~TemplateGuardMutex()
{
    if (_is_locked) {
        _is_locked = !_mutex.release();
        if (_is_locked) {
            // With C++11, destructors are no longer allowed to throw an exception.
            static const char err[] = "\n\n*** Fatal error: GuardMutex failed to release mutex in destructor, aborting...\n\n";
            static constexpr size_t err_size = sizeof(err) - 1;
            FatalError(err, err_size);
        }
    }
}
TS_POP_WARNING()

// Force an early unlock of the mutex.
template <class MUTEX>
bool ts::TemplateGuardMutex<MUTEX>::unlock()
{
    if (_is_locked) {
        _is_locked = !_mutex.release();
        return !_is_locked;
    }
    else {
        // The mutex was not locked in the first place.
        return false;
    }
}
