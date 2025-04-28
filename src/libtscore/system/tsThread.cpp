//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsThread.h"
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
    _attributes(attributes)
{
#if !defined(TS_WINDOWS) &&  defined(GPROF)
    // When using gprof, get the initial profiling timer.
    _itimer_valid = ::getitimer(ITIMER_PROF, &_itimer) == 0;
#endif
}

ts::Thread::~Thread()
{
    // Make sure that the parent class has completed the waitForTermination() or has never started the thread.
    // Get the mutex on checking the started flag but release it before waiting.
    _mutex.lock();
    if (_started) {
        std::cerr << std::endl
                  << "*** Internal error, Thread subclass \"" << _typename
                  << "\" did not wait for its termination, probably safe, maybe not..."
                  << std::endl << std::endl << std::flush;
        _mutex.unlock();
        waitForTermination();
    }
    else {
        _mutex.unlock();
    }
}


//----------------------------------------------------------------------------
// Get/set the class type name.
//----------------------------------------------------------------------------

ts::UString ts::Thread::getTypeName() const
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    const UString name(_typename);
    return name;
}

void ts::Thread::setTypeName(const UString& name)
{
    std::lock_guard<std::recursive_mutex> lock(_mutex);
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
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    attributes = _attributes;
}


//----------------------------------------------------------------------------
// Set new attributes to the thread.
//----------------------------------------------------------------------------

bool ts::Thread::setAttributes(const ThreadAttributes& attributes)
{
    // New attributes are accepted as long as we did not start
    std::lock_guard<std::recursive_mutex> lock(_mutex);
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
    std::lock_guard<std::recursive_mutex> lock(_mutex);

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
    std::lock_guard<std::recursive_mutex> lock(_mutex);

    // Void if already started
    if (_started) {
        return false;
    }

    // Make sure the type name is defined, at least with the default name.
    setTypeName();

#if defined(TS_WINDOWS)

    // Windows implementation.
    // Create the thread in suspended state.
    _handle = ::CreateThread(nullptr, _attributes._stackSize, Thread::ThreadProc, this, CREATE_SUSPENDED, &_thread_id);
    if (_handle == nullptr) {
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
        const size_t size = round_up(std::max<size_t>(PTHREAD_STACK_MIN, _attributes._stackSize), SysInfo::Instance().memoryPageSize());
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
        std::lock_guard<std::recursive_mutex> lock(_mutex);

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
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        _started = false;
        _waiting = false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Dynamically resolve SetThreadDescription() on Windows.
// Implemented as a static function to allow "initialize once" later.
//
// On Windows, SetThreadDescription() is used to set the thread name.
// This function in defined in Kernel32.dll on recent versions of Windows.
// On older versions, referencing this symbol fails. According to
// https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
// on Windows Server 2016, Windows 10 LTSB 2016 and Windows 10 version 1607,
// SetThreadDescription is only available by Run Time Dynamic Linking in
// KernelBase.dll. So, we try Kernel32 first, then KernelBase. Eventually,
// it may not be defined at all and we return a null pointer.
//
// Kernel32 and KernelBase are supposed to be already loaded in any user
// process, so we directly use GetModuleHandle() instead of ts::SharedLibrary.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
namespace {

    // Profile for SetThreadDescription().
    // Note: WINAPI is mandatory on Win32, otherwise calling the function crashes. This is the default with x64.
    using SetThreadDescriptionProfile = ::HRESULT (WINAPI*)(::HANDLE hThread, ::PCWSTR lpThreadDescription);

    // Dynamically resolve SetThreadDescription() on Windows.
    SetThreadDescriptionProfile GetSetThreadDescription()
    {
        void* addr = nullptr;
        const ::HMODULE k32 = ::GetModuleHandleA("Kernel32.dll");
        if (k32 != nullptr) {
            addr = ::GetProcAddress(k32, "SetThreadDescription");
        }
        if (addr == nullptr) {
            const ::HMODULE kbase = ::GetModuleHandleA("KernelBase.dll");
            if (kbase != nullptr) {
                addr = ::GetProcAddress(kbase, "SetThreadDescription");
            }
        }
        return reinterpret_cast<SetThreadDescriptionProfile>(addr);
    }
}
#endif


//----------------------------------------------------------------------------
// Static method. Actual starting point of threads. Parameter is "this".
//----------------------------------------------------------------------------

void ts::Thread::mainWrapper()
{
    // Set thread name. For debug or trace purpose only.
    UString name;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        name = _attributes.getName();
        if (name.empty()) {
            name = _typename;
            // Thread names are limited on some systems, remove unnecessary prefix.
            if (name.starts_with(u"ts::")) {
                name.erase(0, 4);
            }
            name.substitute(u"::", u".");
        }
    }
    if (!name.empty()) {
#if defined(TS_LINUX)
        ::prctl(PR_SET_NAME, name.toUTF8().c_str());
#elif defined(TS_MAC)
        ::pthread_setname_np(name.toUTF8().c_str());
#elif defined(TS_FREEBSD) || defined(TS_DRAGONFLYBSD)
        ::pthread_setname_np(_pthread, name.toUTF8().c_str());
#elif defined(TS_WINDOWS)
        // Thread-safe init-safe static data pattern:
        static const SetThreadDescriptionProfile SetThreadDescriptionAddr = GetSetThreadDescription();
        if (SetThreadDescriptionAddr != nullptr) {
            SetThreadDescriptionAddr(::GetCurrentThread(), name.wc_str());
        }
#endif
    }

    try {
        main();
    }
    catch (const std::exception& e) {
        std::cerr << "*** Internal error, thread aborted: " << e.what() << std::endl;
        if (_attributes.getExitOnException()) {
            std::cerr << "*** Aborting application" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
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
