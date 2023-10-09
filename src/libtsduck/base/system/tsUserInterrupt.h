//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

        volatile ::sig_atomic_t _terminate {0};
        volatile ::sig_atomic_t _got_sigint {0};
#if defined(TS_MAC)
        std::string             _sem_name {};
        ::sem_t*                _sem_address {SEM_FAILED};
#else
        ::sem_t                 _sem_instance {};
#endif
#endif

        InterruptHandler* _handler {nullptr};
        bool              _one_shot {false};
        bool              _active {false};
        volatile bool     _interrupted {false};

        // There is only one active instance at a time
        static UserInterrupt* volatile _active_instance;
    };
}
