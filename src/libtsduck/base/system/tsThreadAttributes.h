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
//!  Declare the ts::ThreadAttributes class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Set of attributes for a thread object (ts::Thread).
    //! @ingroup thread
    //!
    class TSDUCKDLL ThreadAttributes
    {
    public:
        //!
        //! Default constructor (all attributes have their default values).
        //!
        ThreadAttributes();

        //!
        //! Set the thread name.
        //!
        //! This is just an informational name, depending on the operating system.
        //!
        //! @param [in] name Thread name.
        //! @return A reference to this object.
        //!
        ThreadAttributes& setName(const UString& name)
        {
            _name = name;
            return *this;
        }

        //!
        //! Get the thread name.
        //!
        //! @return The thread name.
        //!
        UString getName() const
        {
            return _name;
        }

        //!
        //! Set the stack size in bytes for the thread.
        //!
        //! It is guaranteed that specifying zero as the stack size of a thread
        //! will in fact use the default stack size value for the operating system.
        //!
        //! @param [in] size Stack size in bytes.
        //! @return A reference to this object.
        //!
        ThreadAttributes& setStackSize(size_t size)
        {
            _stackSize = size;
            return *this;
        }

        //!
        //! Get the stack size in bytes for the thread.
        //!
        //! @return The stack size in bytes for the thread.
        //! When zero is returned, this means that the default stack
        //! size for this operating system will be used.
        //!
        size_t getStackSize() const
        {
            return _stackSize;
        }

        //!
        //! Set the <i>delete when terminated flag</i> for the thread.
        //!
        //! The <i>delete when terminated flag</i> is used to automatically
        //! delete ts::Thread objects when the thread execution terminates.
        //! More precisely, when the ts::Thread::main() method terminates,
        //! the @c delete operator is invoked on the ts::Thread object.
        //!
        //! The default value for this flag is @c false.
        //!
        //! To set the <i>delete when terminated flag</i> to @c true, the
        //! following condition are required:
        //! @li The ts::Thread object must be dynamically allocated using
        //!     the @c new operator.
        //! @li Once the thread execution is started, i.e. after invoking
        //!     ts::Thread::start(), the ts::Thread object must no longer
        //!     be referenced from other executing thread.
        //!
        //! The <i>delete when terminated flag</i> is especially useful
        //! to launch dynamically allocated background threads and forget
        //! about them.
        //!
        //! @param [in] dwt Delete when terminated flag for the thread.
        //! @return A reference to this object.
        //!
        ThreadAttributes& setDeleteWhenTerminated(bool dwt)
        {
            _deleteWhenTerminated = dwt;
            return *this;
        }

        //!
        //! Get the <i>delete when terminated flag</i> for the thread.
        //!
        //! @return The delete when terminated flag for the thread.
        //! @see setDeleteWhenTerminated()
        //!
        bool getDeleteWhenTerminated() const
        {
            return _deleteWhenTerminated;
        }

        //!
        //! Set the priority for the thread.
        //!
        //! This class tries to present a system independent view of thread priorities.
        //! A priority is simply an @c int value. The higher the value is, the more
        //! priority the thread have.
        //!
        //! The minimal and maximum priorities are given by @link GetMinimumPriority()
        //! @endlink and @link GetMaximumPriority() @endlink. The default priority
        //! is given by @link GetNormalPriority() @endlink.
        //!
        //! The number of available priorities depends on the operating system and,
        //! sometimes, on the execution context of the operating system.
        //!
        //! <h3>Thread priorities on Microsoft Windows</h3>
        //!
        //! On Microsoft Windows, there are 7 different priorities. They are named
        //! <i>IDLE, LOWEST, BELOW_NORMAL, NORMAL, ABOVE_NORMAL, HIGHEST</i> and
        //! <i>TIME_CRITICAL</i> in the Microsoft literature. They are represented
        //! using 0 to 6 in this class.
        //!
        //! <h3>Thread priorities on Linux</h3>
        //!
        //! On Linux, the number of priorities depends on the <i>scheduling policy</i>
        //! of the current thread. See the man page of @c sched_setscheduler(2) for
        //! more details.
        //!
        //! The default scheduling policy (<i>other</i>, also known as <i>TS</i> for
        //! Time Sharing) assigns the same base priority to all processes and threads,
        //! regardless of the thread priorities which are set in the application. In
        //! this scheduling policy, only one priority is available, the same for all
        //! threads.
        //!
        //! To run a real-time process with discriminating priorities, it is necessary to
        //! run as root and start the command with a non-default scheduling policy. For
        //! instance, the following command runs an application using <i>FIFO</i> scheduling
        //! policy with a base priority 50:
        //! @code
        //! sudo chrt -f 50 /path/to/executable arguments ...
        //! @endcode
        //!
        //! The following command displays the detailed priorities and scheduling
        //! policies of all processes and threads:
        //! @code
        //! ps -m -o pid,lwp,euser,pcpu,pmem,vsz,cls,pri,rtprio,cmd -a
        //! @endcode
        //!
        //! @param [in] priority The priority for the thread.
        //! If the specified priority is lower than the operating system minimum
        //! priority, the actual priority is set to the minimum value.
        //! Similarly, if the specified priority is greater than the operating system
        //! maximum priority, the actual priority is set to the maximum value.
        //! @return A reference to this object.
        //!
        ThreadAttributes& setPriority(int priority);

        //!
        //! Get the priority for the thread.
        //!
        //! @return The priority for the thread.
        //! @see setPriority()
        //!
        int getPriority() const
        {
            return _priority;
        }

        //!
        //! Get the minimum priority for a thread in this context of the operating system.
        //! @return The minimum priority for a thread.
        //! @see setPriority()
        //!
        static inline int GetMinimumPriority()
        {
            return GetPriority(_minimumPriority);
        }

        //!
        //! Get a low priority for a thread in this context of the operating system.
        //! This is a priority which is typically between GetMinimumPriority() and GetNormalPriority().
        //! @return A "low" priority for a thread.
        //! @see setPriority()
        //!
        static inline int GetLowPriority()
        {
            return GetPriority(_lowPriority);
        }

        //!
        //! Get the @e normal priority for a thread in this context of the operating system.
        //! This is the priority of a thread which is neither advantaged nor disadvantaged.
        //! @return The normal priority for a thread.
        //! @see setPriority()
        //!
        static inline int GetNormalPriority()
        {
            return GetPriority(_normalPriority);
        }

        //!
        //! Get a high priority for a thread in this context of the operating system.
        //! This is a priority which is typically between GetNormalPriority() and GetMaximumPriority().
        //! @return A "high" priority for a thread.
        //! @see setPriority()
        //!
        static inline int GetHighPriority()
        {
            return GetPriority(_highPriority);
        }

        //!
        //! Get the maximum priority for a thread in this context of the operating system.
        //! @return The maximum priority for a thread.
        //! @see setPriority()
        //!
        static inline int GetMaximumPriority()
        {
            return GetPriority(_maximumPriority);
        }

    private:
        size_t  _stackSize;
        bool    _deleteWhenTerminated;
        int     _priority;
        UString _name;

        //
        // These fields describe the operating system priority range.
        // There is no need to explicitly synchronize access to them:
        // - Synchronization is required only in the presence of multiple threads.
        // - To create a thread, you need to create a ThreadAttribute object first.
        // - The constructor of the first ThreadAttribute object initializes them.
        // - Consequently, when these data are accessed in a thread other than the
        //   main thread, the flag _PriorityInitialized is already true.
        //
        static volatile bool _priorityInitialized;
        static int _minimumPriority;
        static int _lowPriority;
        static int _normalPriority;
        static int _highPriority;
        static int _maximumPriority;

        //!
        //! Initializes the operating system priority range.
        //!
        static void InitializePriorities();

        //!
        //! Get one of the predefined priorities.
        //! Static priorities are initialized the first time.
        //! @param [in] staticPriority Reference of a predefined priority.
        //! @return The requested priority value.
        //!
        static int GetPriority(const int& staticPriority);

        //! @cond nodoxygen
        friend class Thread;
        //! @endcond

#if defined(TS_WINDOWS)
        // This static method is used by the implementation of ts::Thread on Windows
        // to obtain the actual Win32 priority value.
        static int Win32Priority(int priority);
#elif defined(TS_UNIX)
        // This static method is used by the implementation of ts::Thread on Unix
        // to obtain the scheduling policy to use for this process.
        static int PthreadSchedulingPolicy();
#endif
    };
}
