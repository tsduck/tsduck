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
#include "tsReporterBase.h"
#include "tsIPUtils.h"

//
// Depending on the operating system, we use distinct forms of kernel event queues.
// This is not simply an internal detail of the Reactor class. It influences the
// way "reactive classes" will interact with a Reactor regarding I/O's.
//
#if !defined(DOXYGEN)
    #if defined(TS_LINUX)
        // Use epoll(), a Linux specific feature.
        // In the future, we may consider
        #define TS_USE_EPOLL 1
    #elif defined(TS_MAC) || defined(TS_BSD)
        // Use kqueue(), as found on macOS and all BSD systems.
        #define TS_USE_KQUEUE 1
    #elif defined(TS_WINDOWS)
        // Use I/O completion ports, a Windows feature.
        #define TS_USE_IOCP 1
    #else
        #error "Reactor is not supported on this operating system"
    #endif
#endif

#if defined(TS_USE_EPOLL) || defined(TS_USE_KQUEUE) || defined(DOXYGEN)
    //!
    //! This macro is defined when the Reactor uses a non-blocking I/O model.
    //! Reactive classes which manage I/O shall repeatedly attempt I/O operations as long
    //! as they succeed. When they fail with a "would block" status, the reactive class
    //! shall request the Reactor to be notified when the I/O becomes possible.
    //!
    #define TS_USE_NON_BLOCKING_IO 1
#endif

#if defined(TS_USE_IOCP) || defined(DOXYGEN)
    //!
    //! This macro is defined when the Reactor uses an asynchronous I/O model.
    //! Reactive classes which manage I/O shall start I/O operations and, if the operation
    //! completes with a "pending" status, the reactive class shall request the Reactor to
    //! be notified when the I/O completes. In the meantime, the reactive class shall ensure
    //! that the I/O buffers remain valid, as they are used in the background by the I/O.
    //!
    #define TS_USE_ASYNCHRONOUS_IO 1
#endif

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
    class TSCOREDLL Reactor : public ReporterBase
    {
        TS_NOCOPY(Reactor);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        Reactor(Report* report = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        Reactor(ReporterBase* delegate);

        //!
        //! Destructor.
        //! All resources, such as pending timers or user events, are deleted.
        //!
        virtual ~Reactor() override;

        //!
        //! This static function returns the reactive I/O model.
        //! @return True when asynchronous I/O are used, false when non-blocking I/O are used.
        //!
        static consteval bool UseAsynchronousIO()
        {
        #if defined(TS_USE_ASYNCHRONOUS_IO) && !defined(TS_USE_NON_BLOCKING_IO)
            return true;
        #elif defined(TS_USE_NON_BLOCKING_IO) && !defined(TS_USE_ASYNCHRONOUS_IO)
            return false;
        #else
            #error "invalid asynchronous vs. non-blocking configuration"
        #endif
        }

        //!
        //! This static function returns the reactive I/O model.
        //! @return True when non-blocking I/O are used, false when asynchronous I/O are used.
        //!
        static consteval bool UseNonBlockingIO() { return !UseAsynchronousIO(); }

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
        EventId newTimer(ReactorHandlerInterface* handler, cn::duration<Rep, Period> duration, bool repeat)
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
        EventId newEvent(ReactorHandlerInterface* handler);

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
        // ASYNCHRONOUS I/O EVENTS
        //--------------------------------------------------------------------

        //!
        //! Add in the reactor a notification of asynchronous I/O on a system file descriptor or handle.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the asynchronous I/O model.
        //! @param [in] handler Address of a handler to call when any asynchronous I/O completes on the specified system handle.
        //! Return an error if set as @c nullptr.
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return The identity of the asynchronous I/O. Invalid in case of error.
        //!
        EventId newAsynchronousIO(ReactorHandlerInterface* handler, SysSocketType sock);

        //!
        //! Cancel all pending asynchronous I/O on a system file descriptor or handle.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the asynchronous I/O model.
        //! @param [in] id Asynchronous I/O id, as previously returned by newAsynchronousIO().
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        //! IMPORTANT: After canceling all asynchronous I/O, the application shall wait for the reception
        //! of all I/O completions (presumably with an error status) before releasing the data buffers.
        //! @see NonBlockingDevice
        //!
        bool cancelAsynchronousIO(EventId id, bool silent = false);

        //!
        //! Cancel one specific pending asynchronous I/O.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the asynchronous I/O model.
        //! Warning: This is a blocking call. It shall be used in case of trouble only.
        //! @param [in] id Asynchronous I/O id, as previously returned by newAsynchronousIO().
        //! @param [in,out] iosb The asynchronous I/O status block.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool cancelAndWaitAsynchronousIO(EventId id, NonBlockingDevice::IOSB& iosb, bool silent = false);

        //!
        //! Delete a notification of asynchronous I/O.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the asynchronous I/O model.
        //! @param [in] id Asynchronous I/O id, as previously returned by newAsynchronousIO().
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool deleteAsynchronousIO(EventId id, bool silent = false);

        //--------------------------------------------------------------------
        // NON-BLOCKING I/O EVENTS
        //--------------------------------------------------------------------

        //!
        //! Add in the reactor a notification of read-ready on a system file descriptor.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the non-blocking I/O model.
        //! @param [in] handler Address of a handler to call when the operation is ready. Return an error if set as @c nullptr.
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return The identity of the user event. Invalid in case of error.
        //!
        EventId newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock);

        //!
        //! Delete a notification of read-ready or read-completion.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the non-blocking I/O model.
        //! @param [in] id Event to delete.
        //! @param [in] silent If true, do not report errors through the logger.
        //! @return True on success, false on error.
        //!
        bool deleteReadNotify(EventId id, bool silent = false);

        //!
        //! Add in the reactor a notification of write-ready or read-completion on a system file descriptor.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the non-blocking I/O model.
        //! @param [in] handler Address of a handler to call when the operation is ready. Return an error if set as @c nullptr.
        //! @param [in] sock A system-specific file descriptor or handle. This can be a socket or something else.
        //! @return The identity of the user event. Invalid in case of error.
        //!
        EventId newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock);

        //!
        //! Delete a notification of write-ready or write-completion.
        //! This method is normally never used in applications. It is used only by "reactive I/O classes", in the non-blocking I/O model.
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
                report().info(_trace_prefix + fmt, std::forward<ArgMixIn>(args)...);
            }
        }

    private:
        // All dirty system-specific stuff is moved into a Guts internal structure.
        // Basic common error checking is done at Reactor level. The rest is done in Guts.
        // The virtual functions which only apply to non-blocking or asynchronous I/O have
        // a default implementation which return an error. An implementation of Reactor only
        // need to implement the supported model.
        class TSCOREDLL GutsBase
        {
        protected:
            Reactor& _reactor;  // Parent reactor.
        public:
            GutsBase(Reactor& parent) : _reactor(parent) {}
            virtual ~GutsBase();
            virtual bool open() = 0;
            virtual bool close(bool silent) = 0;
            virtual void processEventLoop() = 0;
            virtual void* newTimer(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat) = 0;
            virtual bool cancelTimer(EventId id, bool silent) = 0;
            virtual void* newEvent(ReactorHandlerInterface* handler) = 0;
            virtual bool signalEvent(EventId id) = 0;
            virtual bool deleteEvent(EventId id, bool silent) = 0;
            virtual void* newAsynchronousIO(ReactorHandlerInterface* handler, SysSocketType sock);
            virtual bool cancelAsynchronousIO(EventId id, bool silent);
            virtual bool cancelAndWaitAsynchronousIO(EventId id, NonBlockingDevice::IOSB& iosb, bool silent);
            virtual bool deleteAsynchronousIO(EventId id, bool silent);
            virtual void* newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock);
            virtual bool deleteReadNotify(EventId id, bool silent);
            virtual void* newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock);
            virtual bool deleteWriteNotify(EventId id, bool silent);
        };

        // The class Guts is defined as a subclass of GutBase in the system-specific source code.
        class Guts;
        GutsBase* allocateGuts();

        // Types of source events. Can be used as bit mask in rare cases (read+write with epoll for instance).
        enum EventType : uint8_t {
            EVT_NONE  = 0x00,
            EVT_TIMER = 0x01,
            EVT_EVENT = 0x02,
            EVT_READ  = 0x04,
            EVT_WRITE = 0x08,
            EVT_ASYNC = 0x10,
        };
        static const Names& EventTypeNames();

        // EventData is the data for an EventId. Its address is in the internal pointer of the EventId.
        // In all implementation, the first field must be a Canary.
        class EventData;
        EventData* allocateEventData();
        void deallocateEventData(EventData*);

        // Reactor private members.
        GutsBase* _guts = nullptr;
        bool      _is_open = false;
        bool      _exit_requested = false;   // Exit event loop when possible.
        bool      _exit_success = true;      // Exit status for event loop.
        std::set<EventData*> _events {};     // Existing allocated events.

        static const bool    _active_trace;  // Check if trace() shall report messages.
        static const UString _trace_prefix;  // Prefix of trace() messages.

        // Deletion of EventData: An EventData can be triggered (event, timer, I/O) and then canceled or deleted
        // by the application. However, the trigger remains in the kernel queue (epoll, kqueue, IOCP) and will be
        // immediately reported the next time the kernel queue is called to wait for events. Additionally, when
        // processing a list of triggered event, the application may cancel or delete an event which is already
        // reported in the current list of triggered events, but farther in the list, not yet processed.
        // To tackle this, we keep a list of recently canceled/deleted events so that if these events are reported
        // by the kernel queue, we can safely ignore them. The "recently deleted" events are those which were
        // deleted just before getting the current event list or during the processing of previous events in
        // the current event list. We keep two sets of events: those which were deleted during the processing
        // of the current event lists, and a superset of it which also contains the event which were deleted
        // during the processing of the previous event list.
        std::set<EventData*> _deleted_current {};
        std::set<EventData*> _deleted_previous_current {};

        // This method adjusts _deleted_previous_current and _deleted_current after processing a set of events
        // as returned by the kernel queue.
        void endOfEventProcessing()
        {
            // Swap the sets of deleted events.
            _deleted_previous_current.swap(_deleted_current);
            _deleted_current.clear();
        }

        // Allocate a new EventData that is not a reuse of a recently deallocated one in _deleted_previous_current.
        // This is useful if the new data structure immediately generates a new event. Otherwise it could be ignored.
        // The new event is registered in _events. The "type" argument is informational only, for trace messages.
        EventData* newEventData(EventType type);

        // Deregister and delete an EventData. The "type" argument is informational only, for trace messages.
        void deleteEventData(EventData* evd, EventType type);

        // Implementation of public template methods.
        EventId newTimerImpl(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat);

        // Verify that the reactor is initialized.
        bool checkOpen(bool silent);

        // Check that an EventData pointer is valid.
        bool validateEventData(EventData* evd, bool silent);

        // All implementations are based on some form of kernel queue. The core of the event loop is a wait system
        // call which uses an array of system structures to receive the completed events. The vector of these
        // structures must be at least as large as the number of pending events, which is potentially different
        // at each wait call. We try to avoid too many reallocations by keeping some minimal number of events.
        static constexpr size_t MIN_WAIT_EVENTS = 16;

        // Adjust the size of the vector of system structures from the number of pending events in the reactor.
        template <class SYSTEM_EVENTS>
        void adjustEventVector(SYSTEM_EVENTS& system_events)
        {
            if (system_events.size() < _events.size() || system_events.size() > _events.size() + 10 * MIN_WAIT_EVENTS) {
                // Current vector is too small or significantly too large.
                system_events.resize(_events.size() + MIN_WAIT_EVENTS);
            }
        }
    };
}
