//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUserInterrupt.h"
#include "tsSysUtils.h"

ts::UserInterrupt* volatile ts::UserInterrupt::_active_instance = nullptr;

// On UNIX platforms, we use a semaphore (sem_t). On macOS, the address of the
// semaphore is returned by sem_open. On other UNIX, the semaphore instance is
// initialized by sem_init.
#if defined(TS_UNIX)
    #if defined(TS_MAC)
         #define SEM_PARAM(obj) ((obj)->_sem_address)
    #else
         #define SEM_PARAM(obj) (&((obj)->_sem_instance))
    #endif
#endif

// On BSD, sigaction(2) uses the flag named SA_RESETHAND instead of SA_ONESHOT.
#if (defined(TS_MAC) || defined(TS_BSD)) && !defined(SA_ONESHOT)
    #define SA_ONESHOT SA_RESETHAND
#endif

// A local mutex to avoid multiple activations.
TS_STATIC_MUTEX(std::recursive_mutex, ActivationMutex);


//----------------------------------------------------------------------------
// Handler on UNIX platforms. Invoked in signal context.
//----------------------------------------------------------------------------

#if defined(TS_UNIX)
void ts::UserInterrupt::sysHandler(int sig)
{
    // There should be one active instance but just check...
    UserInterrupt* ui = _active_instance;
    if (ui == nullptr) {
        return;
    }

    // Atomic set
    ui->_got_sigint = 1;

    // Note that sem_post is the only known-synchronization mechanism which is
    // allowed in a signal handler. This is the reason why we use a semaphore
    // instead of any pthread mechanism.

    if (::sem_post(SEM_PARAM(ui)) < 0) {
        ::perror("sem_post error in SIGINT handler");
        std::exit(EXIT_FAILURE);
    }
}
#endif


//----------------------------------------------------------------------------
// Handler thread on UNIX platforms. Serve as clean thread context for
// application handler.
//----------------------------------------------------------------------------

#if defined(TS_UNIX)
void ts::UserInterrupt::main()
{
    while (_terminate == 0) {
        // Wait for the semaphore to be signaled
        if (::sem_wait(SEM_PARAM(this)) < 0 && errno != EINTR) {
            ::perror("sem_wait");
            std::exit(EXIT_FAILURE);
        }
        if (_got_sigint != 0) {
            _got_sigint = 0;
            // Set interrupted state
            _interrupted = true;
            // Notify the application handler
            if (_handler != nullptr) {
                _handler->handleInterrupt();
            }
            if (_one_shot) {
                break;
            }
        }
    }
}
#endif


//----------------------------------------------------------------------------
// Handler on Windows platforms. Invoked in the context of a system thread.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
::BOOL WINAPI ts::UserInterrupt::sysHandler(__in ::DWORD dwCtrlType)
{
    switch (dwCtrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT: {
            // All these event somehow indicate a user's will to terminate.
            // There should be one active instance but just check...
            UserInterrupt* const ui = _active_instance;
            if (ui != nullptr) {
                // Set interrupted state
                ui->_interrupted = true;

                // Notify the application handler
                if (ui->_handler != nullptr) {
                    ui->_handler->handleInterrupt();
                }

                // Process one-shot interrupt.
                if (ui->_one_shot) {
                    ui->deactivate();
                }
            }
            // Signal fully handled, stop now.
            return true;
        }
        default: {
            // This event is not handled, let next handler work on it.
            return false;
        }
    }
}
#endif


//----------------------------------------------------------------------------
// Constructor.
// If one_shot is true, the interrupt will be handled only once,
// the second time the process will be terminated.
//----------------------------------------------------------------------------

ts::UserInterrupt::UserInterrupt(InterruptHandler* handler, bool one_shot, bool auto_activate) :
#if defined(TS_UNIX)
    // stack size: 16 kB, maximum priority
    Thread(ThreadAttributes().setStackSize(16 * 1024).setPriority(ThreadAttributes::GetMaximumPriority())),
#if defined(TS_MAC)
    _sem_name(UString::Format(u"tsduck-%d-%d", getpid(), ptrdiff_t(this)).toUTF8()),
#endif
#endif
    _handler(handler),
    _one_shot(one_shot)
{
    if (auto_activate) {
        activate();
    }
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::UserInterrupt::~UserInterrupt()
{
    deactivate();
}


//----------------------------------------------------------------------------
// Activate.
// If one_shot is true, the interrupt will be handled only once,
// the second time the process will be terminated.
//----------------------------------------------------------------------------

void ts::UserInterrupt::activate()
{
    // Ensure that there is only one active instance at a time
    std::lock_guard<std::recursive_mutex> lock(ActivationMutex());
    if (_active) {
        return;
    }
    if (_active_instance != nullptr) {
        return;
    }

#if defined(TS_WINDOWS)

    // Install the console interrupt handler
    if (::SetConsoleCtrlHandler(sysHandler, true) == 0) {
        // Failure
        const int err = LastSysErrorCode();
        std::cerr << "* Error establishing console interrupt handler: " << SysErrorCodeMessage(err) << std::endl;
        return;
    }

#elif defined(TS_UNIX)

    _terminate = false;
    _got_sigint = 0;

    // Initialize the semaphore.
#if defined(TS_MAC)
    // MacOS no longer supports unnamed semaphores, we need to use a named one.
    _sem_address = ::sem_open(_sem_name.c_str(), O_CREAT, 0700, 0);
    if (_sem_address == SEM_FAILED || _sem_address == nullptr) {
        ::perror("Error initializing SIGINT semaphore");
        std::exit(EXIT_FAILURE);
    }
#else
    if (::sem_init(&_sem_instance, 0, 0) < 0) {
        ::perror("Error initializing SIGINT semaphore");
        std::exit(EXIT_FAILURE);
    }
#endif

    // Establish the signal handler
    struct sigaction act;
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(disabled-macro-expansion)
    act.sa_handler = sysHandler;
    TS_POP_WARNING()
    act.sa_flags = _one_shot ? SA_ONESHOT : 0;
    sigemptyset(&act.sa_mask);

    // Catch SIGINT (user ctlr-C), SIGQUIT (quit) and SIGTERM (terminate, kill command).
    if (::sigaction(SIGINT, &act, nullptr) < 0 || ::sigaction(SIGQUIT, &act, nullptr) < 0 || ::sigaction(SIGTERM, &act, nullptr) < 0) {
        ::perror("Error setting interrupt signal handler");
        std::exit(EXIT_FAILURE);
    }

    // Start the monitor thread
    start();

#endif

    // Now active
    _active_instance = this;
    _active = true;
}


//----------------------------------------------------------------------------
// Deactivate
//----------------------------------------------------------------------------

void ts::UserInterrupt::deactivate()
{
    // Deactivate only if active.
    std::lock_guard<std::recursive_mutex> lock(ActivationMutex());
    if (!_active) {
        return;
    }
    assert(_active_instance == this);

#if defined(TS_WINDOWS)

    // Remove the console interrupt handler
    ::SetConsoleCtrlHandler(sysHandler, false);

    // Restore normal Ctrl-C processing
    ::SetConsoleCtrlHandler(nullptr, false);

#elif defined(TS_UNIX)

    // Restore the signal handler to default behaviour

    struct sigaction act;
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(disabled-macro-expansion)
    act.sa_handler = SIG_DFL;
    TS_POP_WARNING()
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    if (::sigaction(SIGINT, &act, nullptr) < 0 || ::sigaction(SIGQUIT, &act, nullptr) < 0 || ::sigaction(SIGTERM, &act, nullptr) < 0) {
        ::perror("Error resetting interrupt signal handler");
        std::exit(EXIT_FAILURE);
    }

    // Signal the semaphore to unlock the monitor thread
    _terminate = 1;
    if (::sem_post(SEM_PARAM(this)) < 0) {
        ::perror("sem_post error in SIGINT handler");
        std::exit(EXIT_FAILURE);
    }

    // Wait for the monitor thread to terminate
    waitForTermination();

    // Free resources
#if defined(TS_MAC)
    if (::sem_close(_sem_address) < 0) {
        ::perror("sem_close error on SIGINT semaphore");
        std::exit(EXIT_FAILURE);
    }
    if (::sem_unlink(_sem_name.c_str()) < 0) {
        ::perror("sem_unlink error on SIGINT semaphore");
        std::exit(EXIT_FAILURE);
    }
#else
    if (::sem_destroy(&_sem_instance) < 0) {
        ::perror("Error destroying SIGINT semaphore");
        std::exit(EXIT_FAILURE);
    }
#endif
#endif

    // Now inactive
    _active = false;
    _active_instance = nullptr;
}
