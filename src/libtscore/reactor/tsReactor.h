//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Event-driven reactor class implementing the "event loop" pattern.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReactorHandlerInterface.h"
#include "tsReport.h"
#include "tsIPUtils.h"

namespace ts {
    //!
    //! Event-driven reactor class implementing the "event loop" pattern.
    //! @ingroup libtscore reactor
    //!
    //! To be completed...
    //!
    //! Synchronisation
    //! ---------------
    //! The Reactor shall be used from one single thread. Unless specified otherwise, all methods
    //! shall be invoked from the thread of the reactor, where the event loop is run.
    //! All handlers are invoked in the context of the thread which invoked processEventLoop(),
    //! except worker handlers which are invoked in the context of a worker thread. Therefore,
    //! worker handlers are not allowed to called methods of the reactor.
    //!
    class TSCOREDLL Reactor
    {
        TS_NOBUILD_NOCOPY(Reactor);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        Reactor(Report& report);

        //!
        //! Destructor.
        //! All resources, such as pending timers or user events, are deleted.
        //!
        ~Reactor();

        //!
        //! Access the Report which is associated with the reactor.
        //! Can be called from another thread if the Report object is thread-safe.
        //! @return A reference to the associated report.
        //!
        Report& report() { return _report; }

        //!
        //! Open and initialize the Reactor.
        //! Must be invoked before running the event loop or registering events.
        //! @return True on success, false on error.
        //!
        bool open();

        //!
        //! Close the Reactor.
        //! Must not be called from within the event loop.
        //! Automatically done in the destructor.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! Check if the Reactor is open.
        //! @return True if the Reactor is open, false otherwise.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Check if an event (user-defined, timer, I/O) is still active in the reactor.
        //! @param [in] id Envent id to check.
        //! @return True if @a id is still an active event, false if it has been deleted, completed, canceled.
        //!
        bool isActiveEvent(EventId id);

        //!
        //! Process events until exit is requested.
        //! @return The status from exitEventLoop(). If exitEventLoop() is invoked several
        //! times, the returned value is false if exitEventLoop() has been called at least
        //! once with @a success being false.
        //!
        bool processEventLoop();

        //!
        //! Exit processEventLoop() as soon as possible.
        //! This method is typically invoked from a handler.
        //! @param [in] success The value that processEventLoop() shall return.
        //!
        void exitEventLoop(bool success = true);

        //--------------------------------------------------------------------
        // TIMERS
        //--------------------------------------------------------------------

        //!
        //! Add a timer in the reactor.
        //!
        //! If the timer is one-shot (the default), the timer is automatically deleted after
        //! its expiration, after the handler is invoked. If the timer is repeated, it shall
        //! be explicitly canceled if needed.
        //!
        //! @param [in] handler Address of a handler to call when the timer expires. Return an error if set as @c nullptr.
        //! @param [in] duration Duration of the timer. Must be strictly positive.
        //! @param [in] repeat If true, the timer is repeated at regular intervals, every @a duration, until canceled.
        //! @return The identity of the timer. Invalid in case of error.
        //!
        template <class Rep, class Period>
        EventId newTimer(ReactorTimerHandlerInterface* handler, cn::duration<Rep, Period> duration, bool repeat)
        {
            return newTimerImpl(handler, cn::duration_cast<cn::milliseconds>(duration), repeat);
        }

        //!
        //! Cancel a timer.
        //! @param [in] id Timer to cancel.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool cancelTimer(EventId id, bool silent = false);

        //--------------------------------------------------------------------
        // USER EVENTS
        //--------------------------------------------------------------------

        //!
        //! Add a user event in the reactor.
        //! @param [in] handler Address of a handler to call when the event is signalled. Return an error if set as @c nullptr.
        //! @return The identity of the user event. Invalid in case of error.
        //!
        EventId newEvent(ReactorEventHandlerInterface* handler);

        //!
        //! Signal a user event in the reactor.
        //! As an exception to the single-thread-reactor rule, this method can be invoked from any thread.
        //! @param [in] id Event to signal.
        //! @return True on success, false on error.
        //!
        bool signalEvent(EventId id);

        //!
        //! Delete a user event.
        //! @param [in] id Event to delete.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool deleteEvent(EventId id, bool silent = false);

        //--------------------------------------------------------------------
        // I/O EVENTS
        //--------------------------------------------------------------------

        //!
        //! Add in the reactor a notification of read-ready or read-completion on a system file descriptor.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", at low level.
        //! @param [in] handler Address of a handler to call when the operation is ready. Return an error if set as @c nullptr.
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return The identity of the user event. Invalid in case of error.
        //!
        EventId newReadNotify(ReactorReadHandlerInterface* handler, SysSocketType sock);

        //!
        //! Delete a notification of read-ready or read-completion.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", at low level.
        //! @param [in] id Event to delete.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool deleteReadNotify(EventId id, bool silent = false);

        //!
        //! Add in the reactor a notification of write-ready or read-completion on a system file descriptor.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", at low level.
        //! @param [in] handler Address of a handler to call when the operation is ready. Return an error if set as @c nullptr.
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return The identity of the user event. Invalid in case of error.
        //!
        EventId newWriteNotify(ReactorWriteHandlerInterface* handler, SysSocketType sock);

        //!
        //! Delete a notification of write-ready or write-completion.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", at low level.
        //! @param [in] id Event to delete.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool deleteWriteNotify(EventId id, bool silent = false);

        //--------------------------------------------------------------------
        // DEBUG
        //--------------------------------------------------------------------

        //!
        //! Issue a low-level trace message if environment variable TS_REACTOR_TRACE is defined with a non-empty value.
        //! This is typically use to troubleshoot the internals of a Reactor on a given platform.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void trace(const UChar* fmt, Args&&... args)
        {
            if (_active_trace) {
                _report.info(_trace_prefix + fmt, std::forward<ArgMixIn>(args)...);
            }
        }

    private:
        // All dirty system-specific stuff is moved into a Guts internal structure.
        class Guts;

        // Reactor private members.
        Report& _report;
        Guts*   _guts = nullptr;
        bool    _is_open = false;
        bool    _exit_requested = false;     // Exit event loop when possible.
        bool    _exit_success = true;        // Exit status for event loop.

        static const bool    _active_trace;  // Check if trace() shall report messages.
        static const UString _trace_prefix;  // Prefix of trace() messages.

        // Description of one resource to wait for (I/O, timer, event, etc.)
        // The EventId contains a pointer to a EventData structure.
        class EventData;

        // Types of source events. Can be used as bit mask in rare cases (read+write with epoll for instance).
        enum EventType : uint8_t {
            EVT_NONE  = 0x0000,
            EVT_TIMER = 0x0001,
            EVT_EVENT = 0x0002,
            EVT_READ  = 0x0004,
            EVT_WRITE = 0x0008,
        };
        static const Names& EventTypeNames();

        // Implementation of public template methods.
        EventId newTimerImpl(ReactorTimerHandlerInterface* handler, cn::milliseconds duration, bool repeat);

        // Verify that the reactor is initialized.
        bool checkOpen(bool silent);

        // Compute a log severity level from a "silent" flag.
        static inline int LogLevel(bool silent) { return silent ? Severity::Debug : Severity::Error; }
    };
}
