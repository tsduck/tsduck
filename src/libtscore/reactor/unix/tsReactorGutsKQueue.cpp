//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Reactor implementation using kqueue.
//
//----------------------------------------------------------------------------

#include "tsReactor.h"

// Kqueue is used on macOS and all BSD systems. We do not have a source directory
// which is common to, and limited to, macOS and BSD. So, we use the unix source
// directory but we need to condition the rest of the source file.
#if defined(TS_USE_KQUEUE)

#include "tsCanary.h"
#include "tsSysUtils.h"
#include "tsBeforeStandardHeaders.h"
#include <sys/event.h>
#include "tsAfterStandardHeaders.h"

// Aliases for types in kevent structure (not the same types on all platforms).
// Don't use "::kevent" as type name because ::kevent() is also a system call.
using kevent_t = struct kevent;
using kevent_ident_t = decltype(kevent_t::ident);
using kevent_filter_t = decltype(kevent_t::filter);
using kevent_flags_t = decltype(kevent_t::flags);
using kevent_fflags_t = decltype(kevent_t::fflags);
using kevent_data_t = decltype(kevent_t::data);
using kevent_udata_t = decltype(kevent_t::udata);


//----------------------------------------------------------------------------
// Reactor::Guts, the system-specific internal data structure.
//----------------------------------------------------------------------------

class ts::Reactor::Guts : public Reactor::GutsBase
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
    virtual void* newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock) override;
    virtual bool deleteReadNotify(EventId id, bool silent) override;
    virtual void* newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock) override;
    virtual bool deleteWriteNotify(EventId id, bool silent) override;

private:
    // EventData is the data for an EventId.
    class EventData
    {
        TS_DEFAULT_COPY_MOVE(EventData);
    public:
        Canary                   canary {};          // First field in memory.
        EventType                type = EVT_NONE;    // Event type.
        bool                     repeat = false;     // Repeatable timer or notification.
        ReactorHandlerInterface* handler = nullptr;  // Application handler.
        int                      fd = -1;            // General-purpose file descriptor.

        EventData() = default;
    };

    // A canary is more efficient when at the start of its data structure.
    static_assert(offsetof(EventData, canary) == 0);

    // Guts private fields.
    std::set<EventData*> _events {};       // Existing allocated events.
    int                  _kqueue_fd = -1;  // File descriptor for kqueue().

    // Using kqueue, read and write for the same file descriptor have distinct EventData.
    // We need a map from file-descriptor/event-type to EventData to retrieve the EventData.
    std::map<uint64_t,EventData*> _file_descs_map {};

    // We store the event type in the MSB of the index. We assume that a file descriptor is always less than 2^56.
    static uint64_t FileDescIndex(EventType type, int fd) { return (uint64_t(type) << 56) | uint64_t(fd); }

    // Retrieve or allocate/register an event data for a given file descriptor and event type.
    EventData* newEventData(EventType type, int fd);

    // Deregister and delete an EventData.
    void deleteEventData(EventData*);

    // Check that an EventData pointer is valid.
    bool validateEventData(EventData* evd, bool silent);

    // Common code for event deletion.
    // When ident_is_evd is true, the kqueue ident is the EventData pointer. When false, the kqueue ident is the file descriptor.
    bool deleteFilter(EventId, EventType, bool ident_is_evd, ::kevent_filter_t, bool silent);
    bool sysDelete(EventData*, bool ident_is_evd, ::kevent_filter_t, bool silent);
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

// Retrieve or allocate/register an event data for a given file descriptor.
ts::Reactor::Guts::EventData* ts::Reactor::Guts::newEventData(EventType type, int fd)
{
    // Check if the file descriptor already has an EventData for that event type.
    if (fd >= 0) {
        const auto it = _file_descs_map.find(FileDescIndex(type, fd));
        if (it != _file_descs_map.end()) {
            if (Canary::Error(&it->second->canary) == nullptr) {
                _reactor.trace(u"reuse EventData(%X, %X, %d)", uintptr_t(it->second), it->second->type, it->second->fd);
                assert(it->second->fd == fd);
                assert(it->second->type == type);
                return it->second;
            }
            else {
                // Cleanup an old EventData. Bug?
                _reactor.trace(u"remove obsolete EventData(%X, %X, %d)", uintptr_t(it->second), it->second->type, it->second->fd);
                _file_descs_map.erase(it);
            }
        }
    }

    // Allocate and register a new EventData.
    EventData* evd = new EventData;
    evd->type = type;
    if (fd >= 0) {
        evd->fd = fd;
        _file_descs_map.insert(std::make_pair(FileDescIndex(type, fd), evd));
    }
    _events.insert(evd);
    _reactor.trace(u"new EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);
    return evd;
}

// Deregister and delete an EventData.
void ts::Reactor::Guts::deleteEventData(EventData* evd)
{
    _reactor.trace(u"delete EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);
    if (evd->fd >= 0) {
        _file_descs_map.erase(FileDescIndex(evd->type, evd->fd));
    }
    _events.erase(evd);
    delete evd;
}

// Check that an EventData pointer is valid.
bool ts::Reactor::Guts::validateEventData(EventData* evd, bool silent)
{
    const UChar* cause = Canary::Error(&evd->canary);
    if (cause == nullptr && !_events.contains(evd)) {
        cause = u"EventData no longer in use in Reactor";
    }
    if (cause != nullptr) {
        _reactor.report().log(SilentLevel(silent), u"reactor internal error: invalid EventData pointer, %s", cause);
    }
    return cause == nullptr;
}


//----------------------------------------------------------------------------
// Check if an event is still active in the reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::isActiveEvent(EventId id)
{
    return _events.contains(reinterpret_cast<EventData*>(id._ptr));
}


//----------------------------------------------------------------------------
// Open and initialize the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::open()
{
    // Initialize the kernel queue.
    // Unlike epoll, the kqueue file descriptor is not inherited by a child created with fork().
    assert(_kqueue_fd < 0);
    if ((_kqueue_fd = ::kqueue()) < 0) {
        _reactor.report().error(u"error creating kqueue(): %s", SysErrorCodeMessage());
        return false;
    }
    else {
        _events.clear();
        _file_descs_map.clear();
        return true;
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
        if (validateEventData(evd, silent)) {
            if (evd->type == EVT_EVENT) {
                sysDelete(evd, true, EVFILT_USER, silent);
            }
            else if (evd->type == EVT_TIMER) {
                sysDelete(evd, true, EVFILT_TIMER, silent);
            }
            else if (evd->type == EVT_READ) {
                sysDelete(evd, false, EVFILT_READ, silent);
            }
            else if (evd->type == EVT_WRITE) {
                sysDelete(evd, false, EVFILT_WRITE, silent);
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
    _file_descs_map.clear();

    // Cleanup kqueue().
    assert(_kqueue_fd >= 0);
    ::close(_kqueue_fd);
    _kqueue_fd = -1;
    return success;
}


//----------------------------------------------------------------------------
// Process events until exit is requested.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::processEventLoop()
{
    // List of signalled events. The type SysEventStructure depends on the operating system.
    std::vector<::kevent_t> sysevents(MIN_WAIT_EVENTS);

    // Main event loop.
    while (!_reactor._exit_requested) {

        // Adjust the size of the system structures describing events to wait for.
        _reactor.adjustEventVector(_events, sysevents);

        // Wait for events. No timeouts, use timers to timeout.
        size_t evcount = 0;
        const int status = ::kevent(_kqueue_fd, nullptr, 0, &sysevents[0], int(sysevents.size()), nullptr);
        if (status >= 0) {
            evcount = std::min(sysevents.size(), size_t(status));
        }
        else if (errno == EINTR) {
            // Interrupted by a signal, retry event loop.
            continue;
        }
        else {
            _reactor.report().error(u"error while waiting for kevent: %s", SysErrorCodeMessage());
            _reactor._exit_requested = true;
            _reactor._exit_success = false;
            break;
        }
        _reactor.trace(u"---- processEventLoop: got %d events", evcount);

        // Process all returned events.
        for (size_t i = 0; i < evcount && !_reactor._exit_requested; i++) {

            // Get and check the associated EventData block.
            auto& sysev(sysevents[i]);
            EventData* sysevd = reinterpret_cast<EventData*>(sysev.udata);
            if (!validateEventData(sysevd, false)) {
                continue;
            }
            const EventId id(sysevd);
            _reactor.trace(u"-- processing EventData(%X, %X, %d)", uintptr_t(sysevd), sysevd->type, sysevd->fd);

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
                    // In case of one-shot event, manually remove the one-shot timer before calling the handler.
                    // With kqueue(), one-shot timers are automatically disarmed, no need to disable it.
                    deleteEventData(sysevd);
                }
                // Call the timer callback.
                if (evd.handler != nullptr) {
                    evd.handler->handleTimer(_reactor, id);
                }
            }
            else if (evd.type == EVT_READ) {
                // The data field contains either the pending I/O size or error code.
                const int error_code = (sysev.flags & EV_ERROR) ? int(sysev.data) : SYS_SUCCESS;

                // Call the I/O callback.
                if (evd.handler != nullptr) {
                    evd.handler->handleReadReady(_reactor, id, error_code);
                }
            }
            else if (evd.type == EVT_WRITE) {
                // The data field contains either the remaining output buffer size or error code.
                // We cannot really use the output buffer size, so ignore it.
                const int error_code = (sysev.flags & EV_ERROR) ? int(sysev.data) : SYS_SUCCESS;

                // Call the I/O callback.
                if (evd.handler != nullptr) {
                    evd.handler->handleWriteReady(_reactor, id, error_code);
                }
            }
            else {
                _reactor.report().warning(u"reactor got unexpected event type %X", evd.type);
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
    EventData* evd = newEventData(EVT_TIMER, -1);
    evd->handler = handler;
    evd->repeat = repeat;

    _reactor.trace(u"kqueue add EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);

    // With kqueue(), the timers ids are arbitrary values. Register the event in the kqueue.
    // In kevent, the default value for timer is in milliseconds.
    ::kevent_t ev;
    EV_SET(&ev, ::kevent_ident_t(evd), EVFILT_TIMER, EV_ADD | EV_ENABLE | (repeat ? 0 : EV_ONESHOT), 0, ::kevent_data_t(duration.count()), evd);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().error(u"error registering timer in kqueue: %s", SysErrorCodeMessage());
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
    return deleteFilter(id, EVT_TIMER, true, EVFILT_TIMER, silent);
}


//----------------------------------------------------------------------------
// Add a user event in the reactor.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newEvent(ReactorHandlerInterface* handler)
{
    EventData* evd = newEventData(EVT_EVENT, -1);
    evd->handler = handler;

    _reactor.trace(u"kqueue add EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);

    // With kqueue(), the user-defined events are arbitrary values. Register the event in the kqueue.
    // Note: user data is nullptr here, it is non-null on trigger only.
    ::kevent_t ev;
    EV_SET(&ev, ::kevent_ident_t(evd), EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().error(u"error registering user event in kqueue: %s", SysErrorCodeMessage());
        deleteEventData(evd);
        return nullptr;
    }
    return evd;
}


//----------------------------------------------------------------------------
// Signal a user event in the reactor.
// Important: this method can be invoked from any thread.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::signalEvent(EventId id)
{
    _reactor.trace(u"kqueue trigger EventData(%X)", uintptr_t(id._ptr));

    // Simple trigger, only use the kqueue file descriptor, don't do anything else.
    ::kevent_t ev;
    EV_SET(&ev, ::kevent_ident_t(id._ptr), EVFILT_USER, 0, NOTE_TRIGGER, 0, id._ptr);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().error(u"error triggering user event in kqueue: %s", SysErrorCodeMessage());
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Delete a user event.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteEvent(EventId id, bool silent)
{
    return deleteFilter(id, EVT_EVENT, true, EVFILT_USER, silent);
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    // Create the EventData
    EventData* evd = newEventData(EVT_READ, sock);
    evd->handler = handler;

    _reactor.trace(u"kqueue add EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);

    // Add the event in the kqueue.
    ::kevent_t ev;
    EV_SET(&ev, ::kevent_ident_t(sock), EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, evd);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().error(u"error registering read event in kqueue: %s", SysErrorCodeMessage());
        deleteEventData(evd);
        return nullptr;
    }
    else {
        return evd;
    }
}


//----------------------------------------------------------------------------
// Delete a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteReadNotify(EventId id, bool silent)
{
    return deleteFilter(id, EVT_READ, false, EVFILT_READ, silent);
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of write-ready or read-completion.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    // Create the EventData
    EventData* evd = newEventData(EVT_WRITE, sock);
    evd->handler = handler;

    _reactor.trace(u"kqueue add EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);

    // Add the event in the kqueue.
    ::kevent_t ev;
    EV_SET(&ev, ::kevent_ident_t(sock), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, evd);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().error(u"error registering write event in kqueue: %s", SysErrorCodeMessage());
        deleteEventData(evd);
        return nullptr;
    }
    else {
        return evd;
    }
}


//----------------------------------------------------------------------------
// Delete a notification of write-ready or write-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteWriteNotify(EventId id, bool silent)
{
    return deleteFilter(id, EVT_WRITE, false, EVFILT_WRITE, silent);
}


//----------------------------------------------------------------------------
// Common code for event deletion.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteFilter(EventId id, EventType type, bool ident_is_evd, ::kevent_filter_t filter, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = validateEventData(evd, silent);
    if (success) {
        success = sysDelete(evd, ident_is_evd, filter, silent);
        deleteEventData(evd);
    }
    return success;
}

// Close the system part of the event.
bool ts::Reactor::Guts::sysDelete(EventData* evd, bool ident_is_evd, ::kevent_filter_t filter, bool silent)
{
    _reactor.trace(u"kqueue delete EventData(%X, %X, %d)", uintptr_t(evd), evd->type, evd->fd);

    // Delete the event from the kqueue.
    ::kevent_t ev;
    const ::kevent_ident_t ident = ident_is_evd ? ::kevent_ident_t(evd) : ::kevent_ident_t(evd->fd);
    EV_SET(&ev, ident, filter, EV_DELETE | EV_DISABLE, 0, 0, nullptr);
    if (::kevent(_kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _reactor.report().log(SilentLevel(silent), u"error deleting event from kqueue: %s", SysErrorCodeMessage());
        return false;
    }
    else {
        return true;
    }
}

#endif // TS_USE_KQUEUE
