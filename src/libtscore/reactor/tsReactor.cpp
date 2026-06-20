//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Depending on the operating system, we use distinct forms of kernel event
// queues.
//
// Implementation notes:
//
// - With epoll (Linux), the "interest list" has only one entry per file
//   descriptor. Read and write notifications are set in the same event entry.
//   If read notification is set and we need write notification, we must modify
//   the existing event entry for that file descriptor.
//
// - With kqueue (macOS), an entry in the "interest list" is defined by the
//   pair file descriptor + filter. Read and write notifications are distinct
//   filters. Therefore, read and write notifications for the same file
//   descriptor are distinct event entries.
//
// Online references:
//
// - https://people.freebsd.org/~jlemon/papers/kqueue.pdf
// - https://freebsdfoundation.org/wp-content/uploads/2014/05/Kqueue-Madness.pdf
// - https://habr.com/en/articles/600123/
//
//----------------------------------------------------------------------------

#include "tsReactor.h"
#include "tsEnvironment.h"
#include "tsCanary.h"
#include "tsFatal.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Static constants.
//----------------------------------------------------------------------------

const bool ts::Reactor::_active_trace = !ts::GetEnvironment(u"TS_REACTOR_TRACE").empty();
const ts::UString ts::Reactor::_trace_prefix(u"[reactor-trace] ");


//----------------------------------------------------------------------------
// Types of source events.
//----------------------------------------------------------------------------

const ts::Names& ts::Reactor::EventTypeNames()
{
    static const Names data {
        {u"none",      EVT_NONE},
        {u"timer",     EVT_TIMER},
        {u"event",     EVT_EVENT},
        {u"read",      EVT_READ},
        {u"write",     EVT_WRITE},
        {u"async-I/O", EVT_ASYNC},
    };
    return data;
}


//----------------------------------------------------------------------------
// Reactor constructors and destructor.
//----------------------------------------------------------------------------

ts::Reactor::Reactor(Report* report, Object* owner) :
    ReporterBase(report, owner),
    _guts(allocateGuts())
{
    CheckNonNull(_guts);
}

ts::Reactor::Reactor(ReporterBase* delegate, Object* owner) :
    ReporterBase(delegate, owner),
    _guts(allocateGuts())
{
    CheckNonNull(_guts);
}

ts::Reactor::~Reactor()
{
    if (_guts != nullptr) {
        if (_is_open) {
            close(true);
        }
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Base class for Guts.
//----------------------------------------------------------------------------

#define TS_NO_AIO  _reactor.report().error(u"asynchronous I/O are not supported on this system")
#define TS_NO_NBIO _reactor.report().error(u"non-blocking I/O are not supported on this system")

ts::Reactor::GutsBase::~GutsBase() {}
void* ts::Reactor::GutsBase::newAsynchronousIO(ReactorHandlerInterface*, SysSocketType) { TS_NO_AIO; return nullptr; }
bool ts::Reactor::GutsBase::cancelAsynchronousIO(EventId, bool) { TS_NO_AIO; return false; }
bool ts::Reactor::GutsBase::cancelAndWaitAsynchronousIO(EventId, NonBlockingDevice::IOSB&, bool) { TS_NO_AIO; return false; }
bool ts::Reactor::GutsBase::deleteAsynchronousIO(EventId, bool) { TS_NO_AIO; return false; }
void* ts::Reactor::GutsBase::newReadNotify(ReactorHandlerInterface*, SysSocketType) { TS_NO_NBIO; return nullptr; }
bool ts::Reactor::GutsBase::deleteReadNotify(EventId, bool) { TS_NO_NBIO; return false; }
void* ts::Reactor::GutsBase::newWriteNotify(ReactorHandlerInterface*, SysSocketType) { TS_NO_NBIO; return nullptr; }
bool ts::Reactor::GutsBase::deleteWriteNotify(EventId id, bool silent) { TS_NO_NBIO; return false; }


//----------------------------------------------------------------------------
// Verify that the reactor is initialized.
//----------------------------------------------------------------------------

bool ts::Reactor::checkOpen(bool silent)
{
    if (!_is_open) {
        report().log(SilentLevel(silent), u"reactor is not initialized");
    }
    return _is_open;
}


//----------------------------------------------------------------------------
// Allocate a new EventData that is not a reuse of a recently deallocated one.
//----------------------------------------------------------------------------

ts::Reactor::EventData* ts::Reactor::newEventData(EventType type)
{
    // Allocate a new EventData. However, we don't want to reuse a previous memory which is flagged as "deleted".
    // So, we allocate EventData until one is not part of the deleted ones and we free (again) those which were
    // already flagged as deleted. Report some traces to evaluate the cost of this heavy-handed algorithms.
    EventData* evd = allocateEventData();
    std::set<EventData*> to_free;
    while (_deleted_previous_current.contains(evd)) {
        to_free.insert(evd);
        evd = allocateEventData();
    }
    for (EventData* e : to_free) {
        deallocateEventData(e);
    }

    // Register the new EventData.
    _events.insert(evd);
    trace(u"new EventData: @%X, type 0x%X, after %d failed attempts", uintptr_t(evd), type, to_free.size());
    return evd;
}


//----------------------------------------------------------------------------
// Deregister and delete an EventData.
//----------------------------------------------------------------------------

void ts::Reactor::deleteEventData(EventData* evd, EventType type)
{
    // Deregister from active EventData.
    _events.erase(evd);

    // Keep track of recently deallocated EventData.
    _deleted_current.insert(evd);
    _deleted_previous_current.insert(evd);

    // Actual deallocation.
    trace(u"delete EventData: @%X, type 0x%X", uintptr_t(evd), type);
    deallocateEventData(evd);
}


//----------------------------------------------------------------------------
// Check that an EventData pointer is valid.
//----------------------------------------------------------------------------

bool ts::Reactor::validateEventData(EventData* evd, bool silent)
{
    // In all implementations of EventData, the first field must be a Canary.
    const UChar* cause = Canary::Error(reinterpret_cast<Canary*>(evd));
    if (cause == nullptr && !_events.contains(evd)) {
        cause = u"EventData no longer in use in Reactor";
    }
    if (cause != nullptr) {
        report().log(SilentLevel(silent), u"reactor internal error: invalid EventData pointer, %s", cause);
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Check if an event is still active in the reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::isActiveEvent(EventId id)
{
    return id.isValid() && _events.contains(reinterpret_cast<EventData*>(id._ptr));
}


//----------------------------------------------------------------------------
// Open and initialize the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::open()
{
    if (_is_open) {
        report().error(u"reactor already open");
        return false;
    }
    else {
        _events.clear();
        _deleted_current.clear();
        _deleted_previous_current.clear();
        return _is_open = _guts->open();
    }
}


//----------------------------------------------------------------------------
// Close the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::close(bool silent)
{
    const bool success = checkOpen(silent) && _guts->close(silent);
    _is_open = false;
    return success;
}


//----------------------------------------------------------------------------
// Exit processEventLoop() as soon as possible.
//----------------------------------------------------------------------------

void ts::Reactor::exitEventLoop(bool success)
{
    if (checkOpen(false)) {
        _exit_requested = true;
        if (!success) {
            _exit_success = false;
        }
    }
}


//----------------------------------------------------------------------------
// Process events until exit is requested.
//----------------------------------------------------------------------------

bool ts::Reactor::processEventLoop()
{
    if (!checkOpen(false)) {
        return false;
    }
    else {
        _exit_requested = false;
        _exit_success = true;
        _guts->processEventLoop();
        return _exit_success;
    }
}


//----------------------------------------------------------------------------
// Add/cancel a timer.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newTimerImpl(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat)
{
    EventId id;
    if (checkOpen(false)) {
        if (duration <= cn::milliseconds::zero()) {
            report().error(u"invalid reactor timer value");
        }
        else {
            id._ptr = _guts->newTimer(handler, duration, repeat);
        }
    }
    return id;
}

bool ts::Reactor::cancelTimer(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->cancelTimer(id, silent);
}


//----------------------------------------------------------------------------
// Add/signal/delete a user event in the reactor.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newEvent(ReactorHandlerInterface* handler)
{
    EventId id;
    if (checkOpen(false)) {
        id._ptr = _guts->newEvent(handler);
    }
    return id;
}

bool ts::Reactor::signalEvent(EventId id)
{
    // Important: this method can be invoked from any thread.
    return checkOpen(false) && id.isValid() && _guts->signalEvent(id);
}

bool ts::Reactor::deleteEvent(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->deleteEvent(id, silent);
}


//----------------------------------------------------------------------------
// Asynchronous I/O notifications.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newAsynchronousIO(ReactorHandlerInterface* handler, SysSocketType sock)
{
    EventId id;
    if (checkOpen(false)) {
        id._ptr = _guts->newAsynchronousIO(handler, sock);
    }
    return id;
}

bool ts::Reactor::cancelAsynchronousIO(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->cancelAsynchronousIO(id, silent);
}

bool ts::Reactor::cancelAndWaitAsynchronousIO(EventId id, NonBlockingDevice::IOSB& iosb, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->cancelAndWaitAsynchronousIO(id, iosb, silent);
}

bool ts::Reactor::deleteAsynchronousIO(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->deleteAsynchronousIO(id, silent);
}


//----------------------------------------------------------------------------
// Non-blocking read/write notifications.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    EventId id;
    if (checkOpen(false)) {
        id._ptr = _guts->newReadNotify(handler, sock);
    }
    return id;
}

bool ts::Reactor::deleteReadNotify(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->deleteReadNotify(id, silent);
}

ts::EventId ts::Reactor::newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    EventId id;
    if (checkOpen(false)) {
        id._ptr = _guts->newWriteNotify(handler, sock);
    }
    return id;
}

bool ts::Reactor::deleteWriteNotify(EventId id, bool silent)
{
    return checkOpen(silent) && id.isValid() && _guts->deleteWriteNotify(id, silent);
}
