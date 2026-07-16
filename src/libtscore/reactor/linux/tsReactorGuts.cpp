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
//----------------------------------------------------------------------------

#include "tsReactor.h"
#include "tsCanary.h"
#include "tsSysUtils.h"

#include "tsBeforeStandardHeaders.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/syscall.h>
#include "tsAfterStandardHeaders.h"


//----------------------------------------------------------------------------
// Reactor::EventData, the data for an EventId.
//----------------------------------------------------------------------------

class ts::Reactor::EventData
{
    TS_DEFAULT_COPY_MOVE(EventData);
public:
    Canary                   canary {};               // First field in memory.
    EventType                type = EVT_NONE;         // Event type (can be read+write with epoll).
    bool                     repeat = false;          // Repeatable timer or notification.
    ReactorHandlerInterface* handler = nullptr;       // Generic handler for all events except read.
    ReactorHandlerInterface* read_handler = nullptr;  // Read-notify handler.
    int                      fd = -1;                 // General-purpose file descriptor.
    ::pid_t                  pid = -1;                // Process id.

    EventData() = default;
};

ts::Reactor::EventData* ts::Reactor::allocateEventData()
{
    return new EventData;
}

void ts::Reactor::deallocateEventData(EventData* evd)
{
    delete evd;
}


//----------------------------------------------------------------------------
// Reactor::Guts, the system-specific internal data structure.
//----------------------------------------------------------------------------

class ts::Reactor::Guts: public GutsBase
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor.
    Guts(Reactor& parent) : GutsBase(parent) {}

    // Implementation of Reactor methods.
    virtual ~Guts() override {}
    virtual bool open() override;
    virtual bool close(bool silent) override;
    virtual void processEventLoop() override;
    virtual void getAllHandlers(std::set<ReactorHandlerInterface*>& handlers) override;
    virtual void* newTimer(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat) override;
    virtual bool cancelTimer(EventId id, bool silent) override;
    virtual void* newEvent(ReactorHandlerInterface* handler) override;
    virtual bool signalEvent(EventId id) override;
    virtual bool deleteEvent(EventId id, bool silent) override;
    virtual void* newProcessIdTermination(ReactorHandlerInterface* handler, SysProcessIdType pid) override;
    virtual bool cancelProcessTermination(EventId id, bool silent) override;
    virtual void* newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock) override;
    virtual bool deleteReadNotify(EventId id, bool silent) override;
    virtual void* newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock) override;
    virtual bool deleteWriteNotify(EventId id, bool silent) override;

private:
    // File descriptor for epoll().
    int _epoll_fd = -1;

    // Using epoll, read and write for the same file descriptor share the same EventData.
    // We need a map from file descriptor to EventData to retrieve the EventData from a file descriptor.
    std::map<int,EventData*> _file_descs_map {};

    // List of process termination events which could not be registered, presumably because the process was already dead.
    std::list<EventData*> _dead_processes {};

    // Retrieve or allocate/register an event data for a given file descriptor.
    // There is only one EventData per file descriptor, sharing EVT_READ and EVT_WRITE.
    // On return, new_type is true is the EventData is new or if it didn't contain that EventType.
    EventData* newEventData(EventType type, int fd, bool& new_type);

    // Deregister and delete an EventData if no longer used after removing an event type.
    void deleteEventData(EventData*, EventType);

    // Common code for epoll_ctl().
    bool callEpollCtl(int op, int fd, uint32_t events, void* data, const UChar* name, bool silent = false);

    // Close the system part of an event.
    bool sysDeleteTimer(EventData*, bool silent);
    bool sysDeleteEvent(EventData*, bool silent);
    bool sysDeleteProcess(EventData*, bool silent);
    bool sysDeleteRead(EventData*, bool silent);
    bool sysDeleteWrite(EventData*, bool silent);

    // In all implementations of EventData, the first field must be a Canary.
    static_assert(offsetof(ts::Reactor::EventData, canary) == 0);
};

// Guts allocation.
ts::Reactor::GutsBase* ts::Reactor::allocateGuts()
{
    return new Guts(*this);
}


//----------------------------------------------------------------------------
// EventData management.
//----------------------------------------------------------------------------

// Retrieve or allocate/register an event data for a given file descriptor.
ts::Reactor::EventData* ts::Reactor::Guts::newEventData(EventType type, int fd, bool& new_type)
{
    // Check if the file descriptor already has an EventData.
    if (fd >= 0) {
        const auto it = _file_descs_map.find(fd);
        if (it != _file_descs_map.end()) {
            if (Canary::Error(&it->second->canary) == nullptr) {
                // Found a valid EventData for the same file descriptor. Add the event.
                assert(it->second->fd == fd);
                new_type = (it->second->type & type) != type;
                it->second->type = EventType(type | it->second->type);
                _reactor.trace(u"reuse EventData @%X, type 0x%X", uintptr_t(it->second), it->second->type);
                return it->second;
            }
            else {
                // Cleanup an old EventData. Bug?
                _reactor.trace(u"remove obsolete EventData @%X for file descriptor %d", uintptr_t(it->second), fd);
                _file_descs_map.erase(it);
            }
        }
    }

    // Allocate and register a new EventData.
    EventData* evd = _reactor.newEventData(type);
    evd->type = type;
    new_type = true;
    if (fd >= 0) {
        evd->fd = fd;
        _file_descs_map.insert(std::make_pair(fd, evd));
    }
    return evd;
}

// Deregister and delete an EventData if no longer used after removing an event type.
void ts::Reactor::Guts::deleteEventData(EventData* evd, EventType type)
{
    // Clear only the specified event type.
    evd->type = EventType(evd->type & ~type);
    if (evd->type != 0) {
        // Another event type remains, don't delete the EventData, don't remove the association with the file descriptor.
        _reactor.trace(u"keep EventData: %X, remaining type %X", uintptr_t(evd), evd->type);
    }
    else {
        // The EventData is no longer used.
        if (evd->fd >= 0) {
            _file_descs_map.erase(evd->fd);
        }
        _reactor.deleteEventData(evd, type);
    }
}


//----------------------------------------------------------------------------
// Get all registered handlers.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::getAllHandlers(std::set<ReactorHandlerInterface*>& handlers)
{
    for (const auto& ev : _reactor._events) {
        if (ev != nullptr) {
            handlers.insert(ev->handler);
            handlers.insert(ev->read_handler);
        }
    }
}


//----------------------------------------------------------------------------
// Open and initialize the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::open()
{
    _file_descs_map.clear();
    _dead_processes.clear();

    // Initialize the epoll event queue.
    assert(_epoll_fd < 0);
    if ((_epoll_fd = ::epoll_create1(EPOLL_CLOEXEC)) < 0) {
        _reactor.report().error(u"error creating epoll queue: %s", SysErrorCodeMessage());
        return false;
    }
    else {
        _reactor.trace(u"open epoll, fd %d", _epoll_fd);
        // Unlike kqueue, the epoll file descriptor is inherited by a child created with fork().
        // Register it to be closed in child process after fork().
        AddCloseOnForkExec(_epoll_fd, false);
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
    for (auto evd : _reactor._events) {
        if (_reactor.validateEventData(evd, silent)) {
            if (evd->type == EVT_EVENT) {
                sysDeleteEvent(evd, silent);
            }
            else if (evd->type == EVT_TIMER) {
                sysDeleteTimer(evd, silent);
            }
            else if (evd->type == EVT_PROC) {
                sysDeleteProcess(evd, silent);
            }
            else {
                // Read and write can be on the same EventData.
                if ((evd->type & EVT_READ) != 0) {
                    sysDeleteRead(evd, silent);
                }
                if ((evd->type & EVT_WRITE) != 0) {
                    sysDeleteWrite(evd, silent);
                }
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
    _reactor._events.clear();
    _file_descs_map.clear();
    _dead_processes.clear();

    // Cleanup epoll queue.
    assert(_epoll_fd >= 0);
    RemoveCloseOnForkExec(_epoll_fd);
    _reactor.trace(u"close epoll, fd %d", _epoll_fd);
    ::close(_epoll_fd);
    _epoll_fd = -1;
    return success;
}


//----------------------------------------------------------------------------
// Process events until exit is requested.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::processEventLoop()
{
    // List of signalled events.
    std::vector<::epoll_event> sysevents(MIN_WAIT_EVENTS);

    // Main event loop.
    while (!_reactor._exit_requested) {

        // Initial step before waiting: process dead processes which could not be registered in the epoll.
        while (!_dead_processes.empty()) {
            // Dequeue dead process.
            EventData* sysevd = _dead_processes.front();
            _dead_processes.pop_front();

            // Manually remove the event before calling the handler. So, keep a copy of it.
            const EventId id(sysevd);
            const EventData evd(*sysevd);
            deleteEventData(sysevd, EVT_PROC);

            // Call waitpid to reap the process, just in case. Don't wait (WNOHANG), ignore error.
            int st = 0;
            ::waitpid(evd.pid, &st, WNOHANG);

            // Close the pidfd.
            ::close(evd.fd);

            // Call the callback.
            if (evd.handler != nullptr) {
                evd.handler->handleProcessTermination(_reactor, id, evd.pid);
            }
        }

        // Handle exit request from a handler.
        if (_reactor._exit_requested) {
            break;
        }

        // Adjust the size of the system structures describing events to wait for.
        _reactor.adjustEventVector(sysevents);

        // Wait for events. No timeouts, use timers to timeout.
        _reactor.trace(u"epoll: ---- wait for events");
        size_t evcount = 0;
        const int status = ::epoll_wait(_epoll_fd, &sysevents[0], int(sysevents.size()), -1);
        if (status >= 0) {
            evcount = std::min(sysevents.size(), size_t(status));
        }
        else if (errno == EINTR) {
            // Interrupted by a signal, retry event loop.
            continue;
        }
        else {
            _reactor.report().error(u"error while waiting for epoll: %s", SysErrorCodeMessage());
            _reactor._exit_requested = true;
            _reactor._exit_success = false;
            break;
        }
        _reactor.trace(u"epoll: ---- got %d events", evcount);

        // Process all returned events.
        for (size_t i = 0; i < evcount && !_reactor._exit_requested; i++) {

            // Get the associated EventData block.
            auto& sysev(sysevents[i]);
            EventData* sysevd = reinterpret_cast<EventData*>(sysev.data.ptr);

            // Check that this event wasn't deleted in the processing of a previous event.
            if (_reactor._deleted_previous_current.contains(sysevd)) {
                _reactor.trace(u"epoll: event #%d, ignoring deleted EventData @%X", i, uintptr_t(sysevd));
                continue;
            }

            // Check the integrity of the EventData.
            if (!_reactor.validateEventData(sysevd, false)) {
                _reactor.trace(u"epoll: event #%d, dropped EventData @%X", i, uintptr_t(sysevd));
                continue;
            }
            const EventId id(sysevd);

            // Warning: events can be invalidated if the user-handler updates the reactor. So, keep a copy of it.
            const EventData evd(*sysevd);

            // Process the event, based on a copy of the EventData block.
            if (evd.type == EVT_EVENT) {
                _reactor.trace(u"epoll: event #%d, user event completed", i);
                // Try to read from the event file descriptor to verify that the event is actually pending. Loop on EAGAIN.
                uint64_t data = 0;
                bool success = ::read(evd.fd, &data, sizeof(data)) > 0;
                if (!success && !NonBlockingDevice::IsPendingStatus(errno)) {
                    // Actual error, delete the event.
                    _reactor.report().error(u"error reading user event: %s", SysErrorCodeMessage());
                    sysDeleteEvent(sysevd, false);
                    deleteEventData(sysevd, EVT_EVENT);
                }
                if (success && evd.handler != nullptr) {
                    evd.handler->handleUserEvent(_reactor, id);
                }
            }
            else if (evd.type == EVT_TIMER) {
                _reactor.trace(u"epoll: event #%d, timer completed", i);
                // Try to read the number of expirations.
                uint64_t data = 0;
                bool success = ::read(evd.fd, &data, sizeof(data)) > 0;
                if (!success && !NonBlockingDevice::IsPendingStatus(errno)) {
                    // Actual error, delete the timer.
                    _reactor.report().error(u"error reading timer: %s", SysErrorCodeMessage());
                    sysDeleteTimer(sysevd, false);
                    deleteEventData(sysevd, EVT_TIMER);
                }
                if (success) {
                    if (!evd.repeat) {
                        // In case of one-shot event, the timer must be explicitly closed.
                        sysDeleteTimer(sysevd, true);
                        // Manually remove the one-shot timer before calling the handler.
                        deleteEventData(sysevd, EVT_TIMER);
                    }
                    // Call the timer callback.
                    if (evd.handler != nullptr) {
                        evd.handler->handleTimer(_reactor, id);
                    }
                }
            }
            else if (evd.type == EVT_PROC) {
                _reactor.trace(u"epoll: event #%d, process terminated", i);
                // Try to wait on the process.
                ::siginfo_t info;
                TS_ZERO(info);
                if (::waitid(P_PIDFD, evd.fd, &info, WEXITED | WNOHANG) < 0) {
                    // Actual error, delete the process termination.
                    _reactor.report().error(u"error reading process state: %s", SysErrorCodeMessage());
                    sysDeleteProcess(sysevd, false);
                    deleteEventData(sysevd, EVT_PROC);
                    ::close(evd.fd);
                }
                else if (info.si_pid != 0) {
                    // The process is terminated (not a spurious wake-up).
                    sysDeleteProcess(sysevd, false);
                    deleteEventData(sysevd, EVT_PROC);
                    // Close the pidfd.
                    ::close(evd.fd);
                    // Call the callback.
                    if (evd.handler != nullptr) {
                        evd.handler->handleProcessTermination(_reactor, id, evd.pid);
                    }
                }
            }
            else {
                _reactor.trace(u"epoll: event #%d, completed I/O", i);
                // Read and write can be on the same EventData.
                bool event_is_valid = true;

                // Process read events first.
                if ((evd.type & EVT_READ) != 0) {
                    int error_code = SYS_SUCCESS;
                    bool call_handler = true;

                    if ((sysev.events & EPOLLIN) == 0) {
                        // No read event, this can be an error or a write notification.
                        if ((sysev.events & EPOLLERR) != 0) {
                            error_code = SYS_ERROR;
                        }
                        else {
                            call_handler = false; // must be a write event only.
                        }
                    }

                    // Call the I/O callback. Caution: use read_handler, not handler.
                    if (call_handler && evd.read_handler != nullptr) {
                        evd.read_handler->handleReadReady(_reactor, id, error_code);
                    }

                    // Check if the read handler has removed the event. In that case, don't try to call the write handler.
                    // Also check if the same address has been reallocated to another EventData.
                    event_is_valid = _reactor._events.contains(sysevd) && sysevd->fd == evd.fd;
                }

                // Then, process write events.
                if (event_is_valid && (evd.type & EVT_WRITE) != 0) {
                    int error_code = SYS_SUCCESS;
                    bool call_handler = true;

                    if ((sysev.events & EPOLLOUT) == 0) {
                        // No write event, this can be an error or a read notification.
                        if ((sysev.events & EPOLLERR) != 0) {
                            error_code = SYS_ERROR;
                        }
                        else {
                            call_handler = false; // must be a read event only.
                        }
                    }

                    // Call the I/O callback.
                    if (call_handler && evd.handler != nullptr) {
                        evd.handler->handleWriteReady(_reactor, id, error_code);
                    }
                }
            }
        }
        _reactor.trace(u"epoll: ---- end of event processing");
        _reactor.endOfEventProcessing();
    }
}


//----------------------------------------------------------------------------
// Common code for epoll_ctl().
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::callEpollCtl(int op, int fd, uint32_t events, void* data, const UChar* name, bool silent)
{
    _reactor.trace(u"epoll_ctl(%d, %d, %d, {0x%X, 0x%X}): %s", _epoll_fd, op, fd, events, uintptr_t(data), name);

    ::epoll_event event;
    TS_ZERO(event);
    event.events = events;
    event.data.ptr = data;

    if (::epoll_ctl(_epoll_fd, op, fd, &event) < 0) {
        _reactor.report().log(SilentLevel(silent), u"error %s in epoll: %s", name, SysErrorCodeMessage());
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Add a timer.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newTimer(ReactorHandlerInterface* handler, cn::milliseconds duration, bool repeat)
{
    // Create the EventData
    bool new_type = false;
    EventData* evd = newEventData(EVT_TIMER, -1, new_type);
    evd->handler = handler;
    evd->repeat = repeat;

    // On Linux, timers are file descriptors which can be monitored.
    evd->fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (evd->fd < 0) {
        _reactor.report().error(u"error in timerfd_create: %s", SysErrorCodeMessage());
        deleteEventData(evd, EVT_TIMER);
        return nullptr;
    }

    // Need to register the file descriptor we set in EventData.
    _file_descs_map.insert(std::make_pair(evd->fd, evd));

    // Register time in epoll queue.
    if (!callEpollCtl(EPOLL_CTL_ADD, evd->fd, EPOLLIN | EPOLLET, evd, u"adding timer")) {
        ::close(evd->fd);
        deleteEventData(evd, EVT_TIMER);
        return nullptr;
    }

    // Time definition of the timer.
    ::itimerspec its;
    TS_ZERO(its);
    its.it_value.tv_sec = timespec_sec_t(cn::duration_cast<cn::seconds>(duration).count());
    its.it_value.tv_nsec = timespec_nsec_t(cn::duration_cast<cn::nanoseconds>(duration).count() % cn::nanoseconds::period::den);
    if (repeat) {
        // Repeat at same interval as initial timeout.
        its.it_interval = its.it_value;
    }
    if (::timerfd_settime(evd->fd, 0, &its, nullptr) < 0) {
        _reactor.report().error(u"error in timerfd_settime: %s", SysErrorCodeMessage());
        callEpollCtl(EPOLL_CTL_DEL, evd->fd, 0, nullptr, u"removing timer");
        ::close(evd->fd);
        deleteEventData(evd, EVT_TIMER);
        return nullptr;
    }

    // Make sure the file descriptor is closed on fork().
    AddCloseOnForkExec(evd->fd, false);
    return evd;
}


//----------------------------------------------------------------------------
// Cancel a timer.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::cancelTimer(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = _reactor.validateEventData(evd, silent);
    if (success) {
        // Cancel and delete the timer.
        success = sysDeleteTimer(evd, silent);
        deleteEventData(evd, EVT_TIMER);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteTimer(EventData* evd, bool silent)
{
    // Remove timer from epoll queue.
    if (!callEpollCtl(EPOLL_CTL_DEL, evd->fd, 0, nullptr, u"removing timer", silent)) {
        return false;
    }
    else {
        RemoveCloseOnForkExec(evd->fd);
        ::close(evd->fd);
        return true;
    }
}


//----------------------------------------------------------------------------
// Add a user event in the reactor.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newEvent(ReactorHandlerInterface* handler)
{
    bool new_type = false;
    EventData* evd = newEventData(EVT_EVENT, -1, new_type);
    evd->handler = handler;

    // Open a event file descriptor.
    evd->fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evd->fd < 0) {
        _reactor.report().error(u"error creating user event with eventfd(): %s", SysErrorCodeMessage());
        deleteEventData(evd, EVT_EVENT);
        return nullptr;
    }

    // Need to register the file descriptor we set in EventData.
    _file_descs_map.insert(std::make_pair(evd->fd, evd));

    // Register event in epoll queue.
    if (!callEpollCtl(EPOLL_CTL_ADD, evd->fd, EPOLLIN | EPOLLET, evd, u"adding user event")) {
        ::close(evd->fd);
        deleteEventData(evd, EVT_EVENT);
        return nullptr;
    }

    // Make sure the file descriptor is closed on fork().
    AddCloseOnForkExec(evd->fd, false);
    return evd;
}


//----------------------------------------------------------------------------
// Signal a user event in the reactor.
// Important: this method can be invoked from any thread.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::signalEvent(EventId id)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);

    // Write the 64-bit value 1 in the event file descriptor.
    uint64_t value = 1;
    if (::write(evd->fd, &value, sizeof(value)) < 0) {
        _reactor.report().error(u"error triggering user event in epoll: %s", SysErrorCodeMessage());
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
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success =_reactor.validateEventData(evd, silent);
    if (success) {
        success = sysDeleteEvent(evd, silent);
        deleteEventData(evd, EVT_EVENT);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteEvent(EventData* evd, bool silent)
{
    // Remove event from epoll queue.
    if (!callEpollCtl(EPOLL_CTL_DEL, evd->fd, 0, nullptr, u"removing user event", silent)) {
        return false;
    }
    else {
        RemoveCloseOnForkExec(evd->fd);
        ::close(evd->fd);
        return true;
    }
}


//----------------------------------------------------------------------------
// Add a process termination event in the reactor, using a process id.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newProcessIdTermination(ReactorHandlerInterface* handler, SysProcessIdType pid)
{
    // Create the EventData
    bool new_type = false;
    EventData* evd = newEventData(EVT_PROC, -1, new_type);
    evd->handler = handler;
    evd->pid = pid;

    // Open a file descriptor on the process.
    evd->fd = ::syscall(SYS_pidfd_open, pid, 0);
    if (evd->fd < 0) {
        // Error. Consider that the process is already dead. Directly enqueue the notification.
        _reactor.trace(u"cannot open process %d, direct notification", pid);
        _dead_processes.push_back(evd);
    }
    else {
        // Need to register the file descriptor we set in EventData.
        _file_descs_map.insert(std::make_pair(evd->fd, evd));

        // Register time in epoll queue.
        if (!callEpollCtl(EPOLL_CTL_ADD, evd->fd, EPOLLIN | EPOLLET, evd, u"adding process termination")) {
            ::close(evd->fd);
            deleteEventData(evd, EVT_PROC);
            return nullptr;
        }
    }
    return evd;
}


//----------------------------------------------------------------------------
// Cancel a process termination event.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::cancelProcessTermination(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = _reactor.validateEventData(evd, silent);
    if (success) {
        // Cancel and delete the process event.
        success = sysDeleteProcess(evd, silent);
        deleteEventData(evd, EVT_PROC);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteProcess(EventData* evd, bool silent)
{
    // Remove pidfd from epoll queue.
    if (!callEpollCtl(EPOLL_CTL_DEL, evd->fd, 0, nullptr, u"removing process termination", silent)) {
        return false;
    }
    else {
        // Close the pidfd (no longer needed).
        ::close(evd->fd);
        return true;
    }
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newReadNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    // Create the EventData
    bool new_type = false;
    EventData* evd = newEventData(EVT_READ, sock, new_type);
    evd->read_handler = handler;

    // Add or update event in epoll queue.
    if (new_type) {
        // The file descriptor was not already monitored for read.
        const bool also_write = (evd->type & EVT_WRITE) != 0;
        const int op = also_write ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        const uint32_t events = EPOLLIN | EPOLLET | (also_write ? int(EPOLLOUT) : 0);
        if (!callEpollCtl(op, evd->fd, events, evd, u"adding read notification")) {
            if (!also_write) {
                // The EventData was just created, remove it.
                deleteEventData(evd, EVT_READ);
            }
            return nullptr;
        }
    }
    return evd;
}


//----------------------------------------------------------------------------
// Delete a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteReadNotify(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = _reactor.validateEventData(evd, silent);
    if (success) {
        success = sysDeleteRead(evd, silent);
        deleteEventData(evd, EVT_READ);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteRead(EventData* evd, bool silent)
{
    // Delete or update event in epoll queue.
    const bool also_write = (evd->type & EVT_WRITE) != 0;
    const int op = also_write ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    const uint32_t events = also_write ? (EPOLLOUT | EPOLLET) : 0;
    return callEpollCtl(op, evd->fd, events, evd, u"removing read notification");
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of write-ready or read-completion.
//----------------------------------------------------------------------------

void* ts::Reactor::Guts::newWriteNotify(ReactorHandlerInterface* handler, SysSocketType sock)
{
    // Create the EventData
    bool new_type = false;
    EventData* evd = newEventData(EVT_WRITE, sock, new_type);
    evd->handler = handler;

    // Add or update event in epoll queue.
    if (new_type) {
        // The file descriptor was not already monitored for write.
        const bool also_read = (evd->type & EVT_READ) != 0;
        const int op = also_read ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        const uint32_t events = EPOLLOUT | EPOLLET | (also_read ? int(EPOLLIN) : 0);
        if (!callEpollCtl(op, evd->fd, events, evd, u"adding write notification")) {
            if (!also_read) {
                // The EventData was just created, remove it.
                deleteEventData(evd, EVT_WRITE);
            }
            return nullptr;
        }
    }
    return evd;
}


//----------------------------------------------------------------------------
// Delete a notification of write-ready or write-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::deleteWriteNotify(EventId id, bool silent)
{
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    bool success = _reactor.validateEventData(evd, silent);
    if (success) {
        success = sysDeleteWrite(evd, silent);
        deleteEventData(evd, EVT_WRITE);
    }
    return success;
}

bool ts::Reactor::Guts::sysDeleteWrite(EventData* evd, bool silent)
{
    // Delete or update event in epoll queue.
    const bool also_read = (evd->type & EVT_READ) != 0;
    const int op = also_read ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    const uint32_t events = also_read ? (EPOLLIN | EPOLLET) : 0;
    return callEpollCtl(op, evd->fd, events, evd, u"removing write notification");
}
