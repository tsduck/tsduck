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
//!  User interrupt handling (Ctrl+C).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInterruptHandler.h"

#if defined(TS_UNIX)
    #include "tsThread.h"
    #include "tsBeforeStandardHeaders.h"
    #include <semaphore.h>
    #include <signal.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {

    //!
    //! An instance of this class handles the Ctrl+C user interrupt.
    //! @ingroup system
    //!
    //! There must be at most one active instance at a time.
    //!
    //! Can be used in two ways:
    //! - Interrupt notification through one InterruptHandler
    //! - Interrupt polling through isInterrupted()/resetInterrupted().
    //!
    class TSDUCKDLL UserInterrupt
#if defined(TS_UNIX)
        : private Thread
#endif
    {
        TS_NOBUILD_NOCOPY(UserInterrupt);
    public:
        //!
        //! Constructor.
        //! @param [in] handler Address of interrupt handler. Can be null.
        //! @param [in] one_shot If true, the interrupt will be handled only once,
        //! the second time the process will be terminated.
        //! @param [in] auto_activate If true, the interrupt handling is immediately activated.
        //!
        UserInterrupt(InterruptHandler* handler, bool one_shot, bool auto_activate);

        //!
        //! Destructor, auto-deactivate.
        //!
#if defined(TS_UNIX)
        virtual ~UserInterrupt() override;
#else
        ~UserInterrupt();
#endif

        //!
        //! Check if this interrupt handler is active.
        //! @return True if this interrupt handler is active.
        //!
        bool isActive() const {return _active;}

        //!
        //! Check if this interrupt was triggered.
        //! @return True if this interrupt handler was triggered.
        //!
        bool isInterrupted() const {return _interrupted;}

        //!
        //! Reset interrupt state.
        //! Now, isInterrupted() will return false, until the next time the interrupt is triggered.
        //!
        void resetInterrupted() {_interrupted = false;}

        //!
        //! Activate this interrupt handler.
        //! Only one handler can be active at a time. This method does nothing
        //! if this handler or another handler is already active.
        //!
        void activate();

        //!
        //! Deactivate this interrupt handler.
        //!
        void deactivate();

    private:
#if defined(TS_WINDOWS)

        static ::BOOL WINAPI sysHandler(__in ::DWORD dwCtrlType);

#elif defined(TS_UNIX)

        static void sysHandler(int sig);
        virtual void main() override;  // ts::Thread implementation

        volatile ::sig_atomic_t _terminate;
        volatile ::sig_atomic_t _got_sigint;
#if defined(TS_MAC)
        std::string             _sem_name;
        ::sem_t*                _sem_address;
#else
        ::sem_t                 _sem_instance;
#endif

#endif

        InterruptHandler* _handler;
        bool              _one_shot;
        bool              _active;
        volatile bool     _interrupted;

        // There is only one active instance at a time
        static UserInterrupt* volatile _active_instance;
    };
}
