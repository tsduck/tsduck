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
//
//  User interrupt handling (Ctrl+C).
//
//----------------------------------------------------------------------------

#include "tsUserInterrupt.h"
#include "tsSingletonManager.h"
#include "tsSysUtils.h"


ts::UserInterrupt* volatile ts::UserInterrupt::_active_instance = 0;


//----------------------------------------------------------------------------
// Handler on UNIX platforms. Invoked in signal context.
//----------------------------------------------------------------------------

#if !defined(__windows)
void ts::UserInterrupt::sysHandler(int sig)
{
    // There should be one active instance but just check...
    UserInterrupt* ui = _active_instance;
    if (ui == 0) {
        return;
    }

    // Atomic set
    ui->_got_sigint = 1;

    // Note that sem_post is the only known-synchronization mechanism which is
    // allowed in a signal handler. This is the reason why we use a semaphore
    // instead of any pthread mechanism.

    if (::sem_post(&ui->_sem_sigint) < 0) {
        ::perror("sem_post error in SIGINT handler");
        ::exit(EXIT_FAILURE);
    }
}
#endif


//----------------------------------------------------------------------------
// Handler thread on UNIX platforms. Serve as clean thread context for
// application handler.
//----------------------------------------------------------------------------

#if !defined(__windows)
void ts::UserInterrupt::main()
{
    while (!_terminate) {
        // Wait for the semaphore to be signaled
        if (::sem_wait(&_sem_sigint) < 0 && errno != EINTR) {
            ::perror("sem_wait");
            ::exit(EXIT_FAILURE);
        }
        if (_got_sigint != 0) {
            _got_sigint = 0;
            // Set interrupted state
            _interrupted = true;
            // Notify the application handler
            if (_handler != 0) {
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

#if defined(__windows)
::BOOL WINAPI ts::UserInterrupt::sysHandler(__in ::DWORD dwCtrlType)
{
    // Only handle Ctrl+C events
    if (dwCtrlType != CTRL_C_EVENT) {
        return FALSE;
    }

    // There should be one active instance but just check...
    UserInterrupt* ui = _active_instance;
    if (ui == 0) {
        return FALSE;
    }

    // Set interrupted state
    ui->_interrupted = true;

    // Notify the application handler
    if (ui->_handler != 0) {
        ui->_handler->handleInterrupt();
    }

    // Deactivate on one-shot
    if (ui->_one_shot) {
        ui->deactivate();
    }

    return TRUE;
}
#endif

        
//----------------------------------------------------------------------------
// Constructor.
// If one_shot is true, the interrupt will be handled only once,
// the second time the process will be terminated.
//----------------------------------------------------------------------------

ts::UserInterrupt::UserInterrupt(InterruptHandler* handler, bool one_shot, bool auto_activate) :
#if !defined (__windows)
    // stack size: 16 kB, maximum priority
    Thread(ThreadAttributes().setStackSize(16 * 1024).setPriority(ThreadAttributes::GetMaximumPriority())),
    _terminate(false),
    _got_sigint(0),
    _sem_sigint(),
#endif
    _handler(handler),
    _one_shot(one_shot),
    _active(false),
    _interrupted(false)
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
    // If already active...
    if (_active) {
        return;
    }

    // Ensure that there is only one active instance at a time
    Guard lock(SingletonManager::Instance()->mutex);
    if (_active_instance != 0) {
        return;
    }

#if defined(__windows)

    // Install the console interrupt handler
    if (SetConsoleCtrlHandler (sysHandler, TRUE) == 0) {
        // Failure
        ErrorCode err = LastErrorCode();
        std::cerr << "* Error establishing console interrupt handler: " << ErrorCodeMessage (err) << std::endl;
        return;
    }

#else

    _terminate = false;
    _got_sigint = 0;

    // Initialize the semaphore
    if (::sem_init (&_sem_sigint, 0, 0) < 0) {
        ::perror ("Error initializing SIGINT semaphore");
        ::exit (EXIT_FAILURE);
    }

    // Establish the signal handler
    struct sigaction act;
    act.sa_handler = sysHandler;
    act.sa_flags = _one_shot ? SA_ONESHOT : 0;
    ::sigemptyset (&act.sa_mask);

    if (::sigaction (SIGINT, &act, 0) < 0) {
        ::perror ("Error setting SIGINT handler");
        ::exit (EXIT_FAILURE);
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

void ts::UserInterrupt::deactivate ()
{
    // Deactivate only if active
    Guard lock (SingletonManager::Instance()->mutex);
    if (!_active) {
        return;
    }
    assert (_active_instance == this);

#if defined (__windows)

    // Remove the console interrupt handler
    SetConsoleCtrlHandler (sysHandler, FALSE);

#else

    // Restore the signal handler to default behaviour

    struct sigaction act;
    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    ::sigemptyset (&act.sa_mask);

    if (::sigaction (SIGINT, &act, NULL) < 0) {
        ::perror ("Error resetting SIGINT handler");
        ::exit (EXIT_FAILURE);
    }

    // Signal the semaphore to unlock the monitor thread
    _terminate = true;
    if (::sem_post (&_sem_sigint) < 0) {
        ::perror ("sem_post error in SIGINT handler");
        ::exit (EXIT_FAILURE);
    }

    // Wait for the monitor thread to terminate
    waitForTermination();

    // Free resources
    if (::sem_destroy (&_sem_sigint) < 0) {
        ::perror ("Error destroying SIGINT semaphore");
        ::exit (EXIT_FAILURE);
    }

#endif

    // Now inactive
    _active = false;
    _active_instance = 0;
}
