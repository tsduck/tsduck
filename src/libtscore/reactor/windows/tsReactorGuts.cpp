//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Windows-specific implementation using I/O completion queues.
//
// Online references:
//
// - https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
// - https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations
// - https://learn.microsoft.com/en-us/windows/win32/sync/asynchronous-procedure-calls
// - https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport
// - https://learn.microsoft.com/en-us/windows/win32/fileio/postqueuedcompletionstatus
// - https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult
//
//----------------------------------------------------------------------------

#include "tsReactor.h"
#include "tsCanary.h"
#include "tsNames.h"
#include "tsSysUtils.h"
#include "tsWinUtils.h"


//----------------------------------------------------------------------------
// Reactor::Guts, the system-specific internal data structure.
//----------------------------------------------------------------------------

class ts::Reactor::Guts : public GutsBase
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor.
    Guts(Reactor& parent) : GutsBase(parent) {}

    // Implementation of Reactor methods.
    virtual ~Guts() override {}
    virtual bool open() override;
    virtual bool close(bool silent) override;
    virtual bool isActiveEvent(EventId id) override;
    virtual void processEventLoop() override;
    virtual void* newTimer(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat) override;
    virtual bool cancelTimer(EventId id, bool silent) override;
    virtual void* newEvent(ReactorHandlerInterface* handler) override;
    virtual bool signalEvent(EventId id) override;
    virtual bool deleteEvent(EventId id, bool silent) override;
    virtual void* newAsynchronousIO(ReactorHandlerInterface* handler, SysSocketType sock) override;
    virtual bool cancelAsynchronousIO(EventId id, bool silent) override;
    virtual bool cancelAndWaitAsynchronousIO(EventId id, NonBlockingDevice::IOSB& iosb, bool silent) override;
    virtual bool deleteAsynchronousIO(EventId id, bool silent) override;

private:
    // EventData is the data for an EventId.
    class EventData
    {
        TS_DEFAULT_COPY_MOVE(EventData);
    public:
        Canary                   canary {};          // First field in memory.
        Reactor::Guts*           guts = nullptr;     // Link to parent reactor, when called in APC.
        EventType                type = EVT_NONE;    // Event type (can be read+write with epoll).
        bool                     repeat = false;     // Repeatable timer or notification.
        ReactorHandlerInterface* handler = nullptr;  // Generic handler.
        ::HANDLE                 handle = nullptr;   // General-purpose handle.

        // The constructor automatically register the EventData.
        EventData(Guts*, EventType);
        EventData() = delete;
    };

    // A canary is more efficient when at the start of its data structure.
    static_assert(offsetof(EventData, canary) == 0);

    // Guts private fields.
    std::set<EventData*> _events {};              // Existing allocated events.
    ::HANDLE             _iocp_handle = nullptr;  // Handle for the I/O completion port.

    // Deregister and delete an EventData.
    void deleteEventData(EventData*);

    // Check that an EventData pointer is valid. If event type is not EVT_NONE, check it.
    bool validateEventData(EventData* evd, EventType type, bool silent);

    // Close the system part of an event.
    bool sysDeleteTimer(EventData*, bool silent);
    bool sysDeleteEvent(EventData*, bool silent);

    // Asynchronous timer completion routine.
    static void APIENTRY WinTimerCompletion(::LPVOID arg, ::DWORD timer_low, ::DWORD time_high);
};


//----------------------------------------------------------------------------
// Guts allocation.
//----------------------------------------------------------------------------

ts::Reactor::GutsBase* ts::Reactor::allocateGuts()
{
    return new Guts(*this);
}


//----------------------------------------------------------------------------
// EventData management.
//----------------------------------------------------------------------------

// EventData constructor automatically registers it.
ts::Reactor::Guts::EventData::EventData(Guts* g, EventType t) :
    guts(g),
    type(t)
{
    guts->_reactor.trace(u"new EventData: %X", uintptr_t(this));
    guts->_events.insert(this);
}

// Deregister and delete an EventData.
void ts::Reactor::Guts::deleteEventData(EventData* evd)
{
    _events.erase(evd);
    _reactor.trace(u"delete EventData: %X", uintptr_t(this));
    delete evd;
}

// Check that an EventData pointer is valid.
bool ts::Reactor::Guts::validateEventData(EventData* evd, EventType type, bool silent)
{
    const UChar* cause = Canary::Error(&evd->canary);
    if (cause == nullptr && !_events.contains(evd)) {
        cause = u"EventData no longer in use in Reactor";
    }
    if (cause != nullptr) {
        _reactor.report().log(SilentLevel(silent), u"reactor internal error: invalid EventData pointer, %s", cause);
        return false;
    }
    else if (type != EVT_NONE && evd->type != type) {
        _reactor.report().log(SilentLevel(silent), u"reactor internal error: unexpected EventData type, expected %s, got %d",
                              EventTypeNames().name(type), EventTypeNames().name(evd->type));
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Check if an event is still active in the reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::isActiveEvent(EventId id)
{
    return _events.contains(reinterpret_cast<EventData*>(id._ptr));
}


//----------------------------------------------------------------------------
// Open the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::open()
{
    // Create an empty I/O completion port.
    assert(!WinHandleValid(_iocp_handle));
    _iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    if (WinHandleValid(_iocp_handle)) {
        _events.clear();
        return true;
    }
    else {
        _reactor.report().error(u"error in CreateIoCompletionPort(): %s", SysErrorCodeMessage());
        _iocp_handle = nullptr;
        return false;
    }
}


//----------------------------------------------------------------------------
// Close the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::close(bool silent)
{
    bool success = true;

    // Force system close of pending events, if there are any.
    for (auto evd : _events) {
        if (validateEventData(evd, EVT_NONE, silent)) {
            if (evd->type == EVT_EVENT) {
                sysDeleteEvent(evd, silent);
            }
            else if (evd->type == EVT_TIMER) {
                sysDeleteTimer(evd, silent);
            }
            // Free the EventData only if it was validated. Cases where an invalid EventData remains
            // in a reactor are bugs. In that case, we don't try to delete it (a memory leak is still
            // better than a potential double-free which may lead to memory corruption).
            delete evd;
        }
        else {
            // Invalid EventData, errors already displayed.
            success = false;
        }
    }
    _events.clear();

    // Close the I/O completion port.
    assert(WinHandleValid(_iocp_handle));
    ::CloseHandle(_iocp_handle);
    _iocp_handle = nullptr;
    return success;
}


//----------------------------------------------------------------------------
// Process events until exit is requested.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::processEventLoop()
{
    // List of signalled events.
    std::vector<::OVERLAPPED_ENTRY> sysevents(MIN_WAIT_EVENTS);

    // Main event loop.
    while (!_reactor._exit_requested) {

        // Adjust the size of the system structures describing events to wait for.
        _reactor.adjustEventVector(_events, sysevents);

        // Wait for events. No timeouts, use timers to timeout.
        // Important: specify to wait in alertable state (last parameter) to allow timer APC.
        ::ULONG retcount = 0;
        size_t evcount = 0;
        if (::GetQueuedCompletionStatusEx(_iocp_handle, &sysevents[0], ::ULONG(sysevents.size()), &retcount, INFINITE, true)) {
            evcount = std::min(sysevents.size(), size_t(retcount));
        }
        else if (::GetLastError() == STATUS_USER_APC) {
            // The alertable state was interrupted by a user APC (here probably a timer completion), retry event loop.
            continue;
        }
        else {
            // Actual error.
            _reactor.report().error(u"error while waiting on I/O completion port: %s", SysErrorCodeMessage());
            _reactor._exit_requested = true;
            _reactor._exit_success = false;
            break;
        }
        _reactor.trace(u"processEventLoop: got %d events", evcount);

        // Process all returned events.
        for (size_t i = 0; i < evcount && !_reactor._exit_requested; i++) {

            // Get and check the associated EventData block.
            auto& sysev(sysevents[i]);
            EventData* sysevd = reinterpret_cast<EventData*>(sysev.lpCompletionKey);
            if (!validateEventData(sysevd, EVT_NONE, false)) {
                continue;
            }
            const EventId id(sysevd);

            // Warning: events can be invalidated if the user-handler updates the reactor. So, keep a copy of it.
            const EventData evd(*sysevd);

            // Process the event, based on a copy of the EventData block.
            if (evd.type == EVT_EVENT) {
                // Call the user event callback.
                if (evd.handler != nullptr) {
                    evd.handler->handleUserEvent(_reactor, id);
                }
            }
            else if (evd.type == EVT_TIMER) {
                if (!evd.repeat) {
                    // In case of one-shot event, the timer must be explicitly closed.
                    sysDeleteTimer(sysevd, true);
                    // Manually remove the one-shot timer before calling the handler.
                    deleteEventData(sysevd);
                }
                // Call the timer callback.
                if (evd.handler != nullptr) {
                    evd.handler->handleTimer(_reactor, id);
                }
            }
            else if (evd.type == EVT_ASYNC) {
                // Asynchronous I/O. Try to find the IOSB from the OVERLAPPED address.
                NonBlockingDevice::IOSB* iosb = NonBlockingDevice::IOSB::ParentIOSB(sysev.lpOverlapped);
                if (iosb == nullptr) {
                    _reactor.report().error(u"reactor received an asynchronous I/O completion without identified IOSB");
                }
                else if (evd.handler != nullptr) {
                    // Translate the content of the OVERLAPPED.
                    int error_code = SYS_SUCCESS;
                    ::DWORD io_size = 0;
                    if (!::GetOverlappedResult(evd.handle, &iosb->overlap, &io_size, false)) {
                        // When the I/O completed on error, GetOverlappedResult set LastError and returns false.
                        // In case of I/O cancelation, the error status is ERROR_OPERATION_ABORTED.
                        error_code = int(::GetLastError());
                        if (error_code == ERROR_OPERATION_ABORTED) {
                            // If the operation was aborted, make it an explicit portable code.
                            error_code = SYS_CANCELED;
                        }
                    }
                    evd.handler->handleAsynchronousIO(_reactor, id, *iosb, size_t(io_size), error_code);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Add a timer.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newTimer(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat)
{
    // Create the EventData
    EventData* evd = new EventData(this, EVT_TIMER);
    evd->handler = handler;
    evd->repeat = repeat;

    // Create a waitable timer.
    evd->handle = ::CreateWaitableTimerW(nullptr, true, nullptr);
    if (!WinHandleValid(evd->handle)) {
        _reactor.report().error(u"error creating waitable timer: %s", SysErrorCodeMessage());
        deleteEventData(evd);
        return nullptr;
    }

    // Build the timer expiration time: Use 100 nanosecond intervals.
    // - Positive values indicate UTC absolute time in the format described by FILETIME.
    // - Negative values indicate relative time.
    ::LARGE_INTEGER expiration;
    expiration.QuadPart = -(duration.count() * 10'000); // 10,000 100-ns units = 1 ms

    // Set the timer expîration.
    if (!::SetWaitableTimer(evd->handle, &expiration, ::ULONG(repeat ? duration.count() : 0), Reactor::Guts::WinTimerCompletion, evd, false)) {
        _reactor.report().error(u"error setting waitable timer: %s", SysErrorCodeMessage());
        ::CloseHandle(evd->handle);
        deleteEventData(evd);
        return nullptr;
    }

    return evd;
}


//----------------------------------------------------------------------------
// Cancel a timer.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::cancelTimer(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, EVT_TIMER, silent);
    if (success) {
        success = sysDeleteTimer(evd, silent);
        deleteEventData(evd);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteTimer(EventData* evd, bool silent)
{
    // Cancel the timer if pending. Ignore errors (timer may not be pending).
    // Note that CancelWaitableTimer cancels outstanding APCs.
    // Therefore, WinTimerCompletion won't be called on this one, even is already pending.
    ::CancelWaitableTimer(evd->handle);
    ::CloseHandle(evd->handle);
    evd->handle = nullptr;
    return true;
}


//----------------------------------------------------------------------------
// Asynchronous timer completion routine.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::WinTimerCompletion(::LPVOID arg, ::DWORD timer_low, ::DWORD time_high)
{
    // We are in asynchronous context, don't access the Reactor structures, don't display errors.
    EventData* evd = reinterpret_cast<EventData*>(arg);
    if (Canary::Error(&evd->canary) == nullptr) {
        // Push a notification using the EventData.
        ::PostQueuedCompletionStatus(evd->guts->_iocp_handle, 0, ::ULONG_PTR(evd), nullptr);
    }
}


//----------------------------------------------------------------------------
// Add a user event in the reactor.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newEvent(ReactorHandlerInterface* handler)
{
    // Create the EventData
    EventData* evd = new EventData(this, EVT_EVENT);
    evd->handler = handler;

    // An event has no specific system handle. The event is explicitly pushed in the I/O completion queue.
    return evd;
}


//----------------------------------------------------------------------------
// Signal a user event in the reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::signalEvent(EventId id)
{
    // Important: this method can be invoked from any thread.
    // Simply push a notification using the EventData.
    if (::PostQueuedCompletionStatus(_iocp_handle, 0, ::ULONG_PTR(id._ptr), nullptr)) {
        return true;
    }
    else {
        _reactor.report().error(u"error triggering user event in PostQueuedCompletionStatus: %s", SysErrorCodeMessage());
        return false;
    }
}


//----------------------------------------------------------------------------
// Delete a user event.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteEvent(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, EVT_EVENT, silent);
    if (success) {
        success = sysDeleteEvent(evd, silent);
        deleteEventData(evd);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteEvent(EventData* evd, bool silent)
{
    // Nothing to close at system level.
    return true;
}


//----------------------------------------------------------------------------
// Add a notification of asynchronous I/O on a system handle.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newAsynchronousIO(ReactorHandlerInterface* handler, SysSocketType sock)
{
    // Create the EventData
    EventData* evd = new EventData(this, EVT_ASYNC);
    evd->handler = handler;
    evd->handle = ::HANDLE(sock);

    // Add the system handle to the I/O completion port.
    if (!WinHandleValid(::CreateIoCompletionPort(::HANDLE(sock), _iocp_handle, ::ULONG_PTR(evd), 0))) {
        _reactor.report().error(u"error adding handle to I/O completion port: %s", SysErrorCodeMessage());
        deleteEventData(evd);
        return nullptr;
    }
    return evd;
}


//----------------------------------------------------------------------------
// Cancel all pending asynchronous I/O on a system file descriptor or handle.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::cancelAsynchronousIO(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, EVT_ASYNC, silent);
    if (success) {
        // Cancel all I/O on the associated handle.
        success = ::CancelIoEx(evd->handle, nullptr);
        int err = LastSysErrorCode();
        // If there is no asynchronous I/O to cancel (ERROR_NOT_FOUND), this is not an error.
        if (!success && err == ERROR_NOT_FOUND) {
            success = true;
        }
        if (!success) {
            _reactor.report().log(SilentLevel(silent), u"error canceling asynchronous I/O: %s", SysErrorCodeMessage());
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Cancel and wait one pending asynchronous I/O (blocking call).
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::cancelAndWaitAsynchronousIO(EventId id, NonBlockingDevice::IOSB& iosb, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, EVT_ASYNC, silent);
    if (success) {
        success = ::CancelIoEx(evd->handle, &iosb.overlap);
        int err = LastSysErrorCode();
        if (!success && err == ERROR_NOT_FOUND) {
            // No asynchronous I/O to cancel, this is not an error.
            return true;
        }
        if (success) {
            // Wait for the completion of the canceled I/O.
            ::DWORD io_size = 0;
            success = ::GetOverlappedResult(evd->handle, &iosb.overlap, &io_size, true);
            err = LastSysErrorCode();
        }
        if (!success) {
            _reactor.report().log(SilentLevel(silent), u"error canceling asynchronous I/O: %s", SysErrorCodeMessage());
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Delete a notification of asynchronous I/O.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteAsynchronousIO(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, EVT_ASYNC, silent);
    if (success) {
        // Here, we face another stupid design idea of Microsoft: Once we have added a device handle
        // to an I/O completion port, there is no way to remove it. We need to wait for the closing
        // of the handle. So, if an application calls deleteAsynchronousIO() but continues to use
        // the handle, this reactor will continue to receive notifications for this handle. We rely
        // on the fact that the EventData is no longer allocated to detect that these completions
        // should be ignored.
        deleteEventData(evd);
    }
    return success;
}
