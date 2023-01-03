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

#include "tsThread.h"
#include "tsThreadLocalObjects.h"
#include "tsGuardMutex.h"
#include "tsMemory.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsIntegerUtils.h"

#if defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <sys/prctl.h>
    #include "tsAfterStandardHeaders.h"
#endif

#if defined(TS_NETBSD) && !defined(PTHREAD_STACK_MIN)
    #define PTHREAD_STACK_MIN (::sysconf(_SC_THREAD_STACK_MIN))
#endif


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::Thread::Thread() :
    Thread(ThreadAttributes())
{
}

ts::Thread::Thread(const ThreadAttributes& attributes) :
    _attributes(attributes),
    _mutex(),
    _typename(),
    _started(false),
    _waiting(false),
#if defined(TS_WINDOWS)
    _handle(INVALID_HANDLE_VALUE),
    _thread_id(0)
#else
#if defined(GPROF)
    // When using gprof, get the initial profiling timer.
    _itimer(),
    _itimer_valid(::getitimer(ITIMER_PROF, &_itimer) == 0),
#endif
    _pthread()
#endif
{
}

ts::Thread::~Thread()
{
    // Make sure that the parent class has completed the waitForTermination() or has never started the thread.
    // First, get the started attribute but release
    GuardMutex lock(_mutex);
    if (_started) {
        std::cerr << std::endl
                  << "*** Internal error, Thread subclass \"" << _typename
                  << "\" did not wait for its termination, probably safe, maybe not..."
                  << std::endl << std::endl << std::flush;
        lock.unlock();
        waitForTermination();
    }
}


//----------------------------------------------------------------------------
// Get/set the class type name.
//----------------------------------------------------------------------------

ts::UString ts::Thread::getTypeName() const
{
    GuardMutex lock(_mutex);
    const UString name(_typename);
    return name;
}

void ts::Thread::setTypeName(const UString& name)
{
    GuardMutex lock(_mutex);
    if (!name.empty()) {
        // An actual name is given, use it.
        _typename = name;
    }
    else if (_typename.empty()) {
        // No name already set, no name specified, use RTTI class name.
        _typename = ClassName(typeid(*this));
    }
}


//----------------------------------------------------------------------------
// Yield execution of the current thread.
//----------------------------------------------------------------------------

void ts::Thread::Yield()
{
#if defined(TS_WINDOWS)
    ::SwitchToThread();
#elif defined(TS_MAC) || defined(TS_ANDROID) || defined(_GLIBCXX_USE_SCHED_YIELD)
    ::sched_yield();
#else
    ::pthread_yield();
#endif
}


//----------------------------------------------------------------------------
// Get a copy of the attributes of the thread.
//----------------------------------------------------------------------------

void ts::Thread::getAttributes(ThreadAttributes& attributes)
{
    GuardMutex lock(_mutex);
    attributes = _attributes;
}


//----------------------------------------------------------------------------
// Set new attributes to the thread.
//----------------------------------------------------------------------------

bool ts::Thread::setAttributes(const ThreadAttributes& attributes)
{
    GuardMutex lock(_mutex);

    // New attributes are accepted as long as we did not start
    if (_started) {
        return false;
    }
    else {
        _attributes = attributes;
        return true;
    }
}


//----------------------------------------------------------------------------
// Check if the caller is running in the context of this thread.
//----------------------------------------------------------------------------

bool ts::Thread::isCurrentThread() const
{
    // Critical section on flags
    GuardMutex lock(_mutex);

    // We cannot be running in the thread if it is not started
    return _started && isCurrentThreadUnchecked();
}


//----------------------------------------------------------------------------
// Internal version of isCurrentThread(), bypass checks
//----------------------------------------------------------------------------

bool ts::Thread::isCurrentThreadUnchecked() const
{
#if defined(TS_WINDOWS)
    return ::GetCurrentThreadId() == _thread_id;
#else
    return ::pthread_equal(::pthread_self(), _pthread) != 0;
#endif
}


//----------------------------------------------------------------------------
// Start the thread.
//----------------------------------------------------------------------------

bool ts::Thread::start()
{
    // Critical section on flags
    GuardMutex lock(_mutex);

    // Void if already started
    if (_started) {
        return false;
    }

    // Make sure the type name is defined, at least with the default name.
    setTypeName();

#if defined(TS_WINDOWS)

    // Windows implementation.
    // Create the thread in suspended state.
    _handle = ::CreateThread(NULL, _attributes._stackSize, Thread::ThreadProc, this, CREATE_SUSPENDED, &_thread_id);
    if (_handle == NULL) {
        return false;
    }

    // Set the thread priority
    ::BOOL status = ::SetThreadPriority(_handle, ThreadAttributes::Win32Priority(_attributes._priority));
    if (status == 0) {
        ::CloseHandle(_handle);
        return false;
    }

    // Release the thread
    if (::ResumeThread(_handle) == ::DWORD(-1)) {
        ::CloseHandle(_handle);
        return false;
    }

#else

    // POSIX pthread implementation.
    // Create thread attributes.
    ::pthread_attr_t attr;
    TS_ZERO(attr);
    if (::pthread_attr_init(&attr) != 0) {
        return false;
    }

    // Set required stack size.
    if (_attributes._stackSize > 0) {
        // Round to a multiple of the page size. This is required on macOS.
        const size_t size = round_up(std::max<size_t>(PTHREAD_STACK_MIN, _attributes._stackSize), SysInfo::Instance()->memoryPageSize());
        if (::pthread_attr_setstacksize(&attr, size) != 0) {
            ::pthread_attr_destroy(&attr);
            return false;
        }
    }

    // Set scheduling policy identical as current process.
    if (::pthread_attr_setschedpolicy(&attr, ThreadAttributes::PthreadSchedulingPolicy()) != 0) {
        ::pthread_attr_destroy(&attr);
        return false;
    }

    // Set scheduling priority.
    ::sched_param sparam;
    TS_ZERO(sparam);
    sparam.sched_priority = _attributes._priority;
    if (::pthread_attr_setschedparam(&attr, &sparam) != 0) {
        ::pthread_attr_destroy(&attr);
        return false;
    }

    // Use explicit scheduling attributes, do not inherit them from the current thread.
    // Apparently not supported on Android before API version 28.
#if !defined(__ANDROID_API__) || __ANDROID_API__ >= 28
    if (::pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        ::pthread_attr_destroy(&attr);
        return false;
    }
#endif

    // Create the thread
    if (::pthread_create(&_pthread, &attr, Thread::ThreadProc, this) != 0) {
        ::pthread_attr_destroy(&attr);
        return false;
    }

    // Destroy thread attributes
    ::pthread_attr_destroy(&attr);

#endif

    // Mark the thread as started.
    _started = true;

    return true;
}


//----------------------------------------------------------------------------
// Wait for thread termination.
//----------------------------------------------------------------------------

bool ts::Thread::waitForTermination()
{
    // Critical section on flags
    {
        GuardMutex lock(_mutex);

        // Void if already terminated
        if (!_started) {
            return true;
        }

        // If "delete when terminated" is true, we cannot wait.
        // The thread will cleanup itself.
        if (_attributes._deleteWhenTerminated) {
            return false;
        }

        // We cannot wait for ourself, it would dead-lock.
        if (isCurrentThreadUnchecked()) {
            return false;
        }

        // Only one waiter thread allowed
        if (_waiting) {
            return false;
        }

        // Mark as being waited
        _waiting = true;
    }

    // Actually wait for the thread
#if defined(TS_WINDOWS)
    ::WaitForSingleObject(_handle, INFINITE);
    ::CloseHandle(_handle);
#else
    ::pthread_join(_pthread, nullptr);
#endif

    // Critical section on flags
    {
        GuardMutex lock(_mutex);
        _started = false;
        _waiting = false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Static method. Actual starting point of threads. Parameter is "this".
//----------------------------------------------------------------------------

void ts::Thread::mainWrapper()
{
    // Set thread name. For debug or trace purpose only.
    UString name(_attributes.getName());
    if (name.empty()) {
        name = _typename;
        // Thread names are limited on some systems, remove unnecessary prefix.
        if (name.startWith(u"ts::")) {
            name.erase(0, 4);
        }
        name.substitute(u"::", u".");
    }
    if (!name.empty()) {
#if defined(TS_LINUX)
        ::prctl(PR_SET_NAME, name.toUTF8().c_str());
#elif defined(TS_MAC)
        ::pthread_setname_np(name.toUTF8().c_str());
#elif defined(TS_FREEBSD) || defined(TS_DRAGONFLYBSD)
        ::pthread_setname_np(_pthread, name.toUTF8().c_str());
#elif defined(TS_WINDOWS)
        ::SetThreadDescription(::GetCurrentThread(), name.wc_str());
#endif
    }

    try {
        main();
    }
    catch (const std::exception& e) {
        std::cerr << "*** Internal error, thread aborted: " << e.what() << std::endl;
    }
    ThreadLocalObjects::Instance()->deleteLocalObjects();
}

#if defined(TS_WINDOWS)

::DWORD WINAPI ts::Thread::ThreadProc(::LPVOID parameter)
{
    Thread* thread = reinterpret_cast<Thread*>(parameter);

    // Execute thread code.
    thread->mainWrapper();

    // Perform auto-deallocation
    if (thread->_attributes._deleteWhenTerminated) {
        ::CloseHandle(thread->_handle);
        thread->_started = false;
        delete thread;
    }

    return 0;
}

#else

void* ts::Thread::ThreadProc(void* parameter)
{
    Thread* thread = reinterpret_cast<Thread*>(parameter);

#if defined(GPROF)
    // When using gprof, set the same profiling timer as the calling thread.
    if (thread->_itimer_valid) {
        ::setitimer(ITIMER_PROF, &thread->_itimer, nullptr);
    }
#endif

    // Execute thread code.
    thread->mainWrapper();

    // Perform auto-deallocation
    if (thread->_attributes._deleteWhenTerminated) {
        ::pthread_detach(thread->_pthread);
        thread->_started = false;
        delete thread;
    }

    return nullptr;
}

#endif
