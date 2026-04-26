//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactor.h"
#include "tsSysUtils.h"
#include "tsEnvironment.h"
#include "tsFatal.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// System-specific kernel queues.
//----------------------------------------------------------------------------

// Depending on the operating system, we use distinct forms of kernel event queues.
//
// Implementation notes:
//
// - With epoll (Linux), the "interest list" has only one entry per file descriptor. Read and write notifications
//   are set in the same event entry. If read notification is set and we need write notification, we must modify
//   the existing event entry for that file descriptor.
//
// - With kqueue (macOS), an entry in the "interest list" is defined by the pair file descriptor + filter. Read and
//   write notifications are distinct filters. Therefore, read and write notifications for the same file descriptor
//   are distinct event entries. This difference between epoll and kqueue is responsible for the complex internal
//   data structure of the Reactor class.
//
// Online references:
//
// - https://people.freebsd.org/~jlemon/papers/kqueue.pdf
// - https://freebsdfoundation.org/wp-content/uploads/2014/05/Kqueue-Madness.pdf
// - https://habr.com/en/articles/600123/
// - https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
// - https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport
// - https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations
// - https://learn.microsoft.com/en-us/windows/win32/sync/asynchronous-procedure-calls

#if defined(TS_LINUX)

    // Use epoll(), a Linux specific feature.
    // Future: investigate io_uring.
    #define TS_USE_EPOLL 1
    #include "tsBeforeStandardHeaders.h"
    #include <sys/epoll.h>
    #include <sys/eventfd.h>
    #include <sys/timerfd.h>
    #include "tsAfterStandardHeaders.h"

#elif defined(TS_MAC) || defined(TS_BSD)

    // Use kqueue(), as found on macOS and all BSD systems.
    #define TS_USE_KQUEUE 1
    #include "tsBeforeStandardHeaders.h"
    #include <sys/event.h>
    #include "tsAfterStandardHeaders.h"

    // Aliases for types in kevent structure (not the same types on all platforms).
    // Don't use "::kevent" as type name because ::kevent() is also a system call.
    using kevent_t = struct kevent;
    using kevent_ident_t = decltype(kevent_t::ident);
    using kevent_data_t  = decltype(kevent_t::data);

#elif defined(TS_WINDOWS)

    // Use I/O completion ports, a Windows feature.
    #define TS_USE_IOCP 1
    #include "tsWinUtils.h"

#else
    #error "Reactor is not supported on this operating system"
#endif


//----------------------------------------------------------------------------
// Static constants.
//----------------------------------------------------------------------------

const bool ts::Reactor::_active_trace = !ts::GetEnvironment(u"TS_REACTOR_TRACE").empty();
const ts::UString ts::Reactor::_trace_prefix(u"[reactor-trace] ");


//----------------------------------------------------------------------------
// Reactor::EventData, the data for an EventId.
//----------------------------------------------------------------------------

class ts::Reactor::EventData
{
    TS_DEFAULT_COPY_MOVE(EventData);
public:
    volatile uint32_t        canary = GOOD;           // First field in memory (see [2] below).
    EventType                type = EVT_NONE;         // Event type (can be read+write with epoll).
    bool                     repeat = false;          // Repeatable timer or notification.
    ReactorHandlerInterface* gen_handler = nullptr;   // Generic handler (except read).
    ReactorHandlerInterface* read_handler = nullptr;  // Read-notify handler (see [1] below).

#if defined(TS_USE_EPOLL) || defined(TS_USE_KQUEUE)
    int      fd = -1;            // General-purpose file descriptor.
#elif defined(TS_USE_IOCP)
    Reactor* reactor = nullptr;  // Link to parent reactor, when called in APC.
    ::HANDLE handle = nullptr;   // General-purpose handle.
#endif

    // Note [1] With epoll, there is only one entry per file descriptor in the list of events to monitor. Therefore, the
    // same EventData must be used for read and write on the same file descriptor. This is done only in the epoll case.
    // In all cases, an EVT_READ event uses the field 'read_handler'. All other events use the field 'gen_handler'.
    // Thus, read and write events can share the same EventData when necessary (epoll only).

    // Note [2] All implementations (epoll, kqueue, iocp) associate some "user pointer" to an event. This pointer is the
    // address of an EventData object. We must be sure that the associated EventData object is stable (same address, still
    // valid) at completion of events. To track potential errors in our interpretation of the various operating system logics,
    // we use some internal consistency checks using canaries. Also keep that structure as plain and simple as possible:
    // no virtual stuff, no std::optional, etc.
    static constexpr uint32_t GOOD = 0x474F4F44;  // "GOOD"
    static constexpr uint32_t BAD  = 0x42414453;  // "BADS"

    // Constructor with all default values.
    EventData() = default;

    // The destructor sets the "bad" canary to detect use-after-free in case of incorrect order of usage.
    ~EventData() { canary = BAD; }

    // Check that an EventData pointer is valid. Detect incorrect understanding of system mechanisms.
    // If 'expected' is not EVT_NONE, check the event type and return the actual event type.
    // Return a static error message or nullptr if the pointer seems valid.
    static const UChar* Error(const EventData*, EventType expected, EventType& actual);
};


// Check that an EventData pointer is valid.
const ts::UChar* ts::Reactor::EventData::Error(const EventData* evd, EventType expected, EventType& actual)
{
    actual = EVT_NONE;
    if (evd == nullptr) {
        return u"null pointer";
    }
    else {
        // Attempt memory access.
        try {
            if (evd->canary != EventData::GOOD) {
                return evd->canary == EventData::BAD ? u"reuse after free" : u"not a EventData block";
            }
            actual = evd->type;
            if (expected != EVT_NONE && (evd->type & expected) != expected) {
                return u"inconsistent event type";
            }
        }
        catch (...) {
            return u"invalid address";
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Reactor::Guts, the system-specific internal data structure.
//----------------------------------------------------------------------------

class ts::Reactor::Guts
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    Reactor&             reactor;    // Parent reactor.
    Report&              report;     // Copy of reference to reactor's report.
    std::set<EventData*> events {};  // Existing allocated events.

    // Constructor.
    Guts(Reactor& parent) : reactor(parent), report(reactor._report) {}

    // Some Reactor methods are delegated to counterparts in Guts to simplify the code.
    bool open();
    bool close(bool silent);
    void processEventLoop();

    // Search or allocate and register a EventData for a given event type and file desriptor. If the entry already existed,
    // the event type is not modified. This can be used to check if the operation was already set on the file descriptor.
    EventData* newEventData(EventType type, SysSocketType sock);

    // Deregister and deallocate a EventData when necessary.
    // With epoll, where read and write share the same EventData, update the remaining type.
    void deleteEventData(EventData* evd, EventType type);

    // Check that an EventData pointer is valid, registered, and display an error message if required.
    bool validateEventData(EventData*, EventType, bool silent);

    // Close the system part of an event.
    bool sysDeleteTimer(EventData*, bool silent);
    bool sysDeleteEvent(EventData*, bool silent);
    bool sysDeleteRead(EventData*, bool silent);
    bool sysDeleteWrite(EventData*, bool silent);

    // All implementations are based on some form of kernel queue. The core of the event loop is a wait system
    // call which uses an array of system structures, SysEventStructure, to receive the completed events.
    // The address of the EventData block is stored in that structure. The vector of SysEventStructure must be
    // at least as large as the number of pending events, which is potentially different at each wait call.
    // We try to avoid too many reallocations by keeping some minimal number of events.
    static constexpr size_t MIN_WAIT_EVENTS = 16;

#if defined(TS_USE_EPOLL)

    // Returned event structure.
    using SysEventStructure = ::epoll_event;

    // Extracting the EventData block from a SysEventStructure.
    static EventData* GetEventData(const SysEventStructure& s) { return reinterpret_cast<EventData*>(s.data.ptr); }

    // Using epoll, read and write for the same file descriptor share the same EventData.
    // We need a map from file descriptor to EventData to retrieve the EventData from a file descriptor.
    std::map<int,EventData*> file_descs_map {};

    // The index in the map is the file descriptor.
    static int FileDescIndex(EventType type, int sock) { return sock; }

    // File descriptor for epoll().
    int epoll_fd = -1;

#elif defined(TS_USE_KQUEUE)

    // Returned event structure.
    using SysEventStructure = kevent_t;

    // Extracting the EventData block from a SysEventStructure.
    static EventData* GetEventData(const SysEventStructure& s) { return reinterpret_cast<EventData*>(s.udata); }

    // Using kqueue, read and write for the same file descriptor have distinct EventData.
    // We need a map from file-descriptor/event-type to EventData to retrieve the EventData.
    std::map<uint64_t,EventData*> file_descs_map {};

    // We store the event type in the MSB of the index. We assume that a file descriptor is always less than 2^56.
    static uint64_t FileDescIndex(EventType type, int sock) { return (uint64_t(type) << 56) | uint64_t(sock); }

    // File descriptor for kqueue(). Can be used in another thread (signal event).
    int kqueue_fd = -1;

#elif defined(TS_USE_IOCP)

    // Returned event structure.
    using SysEventStructure = ::OVERLAPPED_ENTRY;

    // Extracting the EventData block from a SysEventStructure.
    static EventData* GetEventData(const SysEventStructure& s) { return reinterpret_cast<EventData*>(s.lpCompletionKey); }

    // System-specific Reactor fields.
    ::HANDLE iocp_handle = nullptr;  // Handle for the I/O completion port.

    // Asynchronous timer completion routine.
    static void APIENTRY WinTimerCompletion(::LPVOID arg, ::DWORD timer_low, ::DWORD time_high);

#endif
};


//----------------------------------------------------------------------------
// Types of source events.
//----------------------------------------------------------------------------

const ts::Names& ts::Reactor::EventTypeNames()
{
    static const Names data {
        {u"none",  EVT_NONE},
        {u"timer", EVT_TIMER},
        {u"event", EVT_EVENT},
        {u"read",  EVT_READ},
        {u"write", EVT_WRITE},
    };
    return data;
}


//----------------------------------------------------------------------------
// Reactor constructors and destructor.
//----------------------------------------------------------------------------

ts::Reactor::Reactor(Report& report) :
    _report(report),
    _guts(new Guts(*this))
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
// Check if an event is still active in the reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::isActiveEvent(EventId id)
{
    return id.isValid() && _guts->events.contains(reinterpret_cast<EventData*>(id._ptr));
}


//----------------------------------------------------------------------------
// Allocate and register a new event data.
//----------------------------------------------------------------------------

ts::Reactor::EventData* ts::Reactor::Guts::newEventData(EventType type, SysSocketType sock)
{
    reactor.trace(u"newEventData(%X)", type);

    EventData* evd = nullptr;

#if defined(TS_USE_EPOLL) || defined(TS_USE_KQUEUE)

    // Check if the file descriptor already has an EventData.
    if (sock >= 0) {
        const auto it = file_descs_map.find(FileDescIndex(type, sock));
        if (it != file_descs_map.end()) {
            EventType actual = EVT_NONE;
            if (EventData::Error(it->second, EVT_NONE, actual) == nullptr) {
                // Found a valid EventData for the same file descriptor. Add the event.
                reactor.trace(u"reuse EventData %X", uintptr_t(it->second));
                assert(it->second->fd == sock);
                return it->second;
            }
            else {
                // Cleanup an old EventData. Bug?
                reactor.trace(u"remove obsolete EventData %X for file descriptor %d", uintptr_t(it->second), sock);
                file_descs_map.erase(it);
            }
        }
    }

    // Allocate and register a new EventData.
    evd = new EventData;
    reactor.trace(u"new EventData: %X", uintptr_t(evd));
    if (sock >= 0) {
        evd->fd = sock;
        file_descs_map.insert(std::make_pair(FileDescIndex(type, sock), evd));
    }

#elif defined(TS_USE_IOCP)

    // Always allocate a new EventData.
    evd = new EventData;
    reactor.trace(u"new EventData: %X", uintptr_t(evd));
    evd->reactor = &reactor;

#endif

    events.insert(evd);
    return evd;
}


//----------------------------------------------------------------------------
// Deregister and deallocate an EventData.
//----------------------------------------------------------------------------

void ts::Reactor::Guts::deleteEventData(EventData* evd, EventType type)
{
    reactor.trace(u"deleteEventData(%X, %X)", uintptr_t(evd), type);

    // Filter invalid EventData.
    if (evd == nullptr) {
        return;
    }

#if defined(TS_USE_EPOLL)

    // Use the same EventData for read and write on the same file descriptor.
    // Clear only the specified event type.
    evd->type = EventType(evd->type & ~type);
    if (evd->type != 0) {
        // Another event type remains, don't delete the EventData, don't remove the association with the file descriptor.
        reactor.trace(u"keep EventData: %X, remaining type %X", uintptr_t(evd), evd->type);
        return;
    }

    // Erase the file descriptor entry if it was used by one type.
    if (evd->fd >= 0) {
        file_descs_map.erase(FileDescIndex(type, evd->fd));
    }

#elif defined(TS_USE_KQUEUE)

    // Always erase the file descriptor entry.
    if (evd->fd >= 0) {
        file_descs_map.erase(FileDescIndex(type, evd->fd));
    }

#endif

    // Delete the EventData.
    events.erase(evd);
    reactor.trace(u"delete EventData: %X", uintptr_t(evd));
    delete evd;
}


//----------------------------------------------------------------------------
// Check that an EventData pointer is valid.
//----------------------------------------------------------------------------

bool ts::Reactor::Guts::validateEventData(EventData* evd, EventType type, bool silent)
{
    EventType evtype = EVT_NONE;
    const UChar* cause = EventData::Error(evd, type, evtype);
    if (cause == nullptr && !events.contains(evd)) {
        cause = u"EventData no longer in use in Reactor";
    }
    if (cause != nullptr) {
        static const UChar prefix[] = u"reactor internal error: invalid EventData pointer";
        if (type == EVT_NONE) {
            report.log(LogLevel(silent), u"%s, %s", prefix, cause);
        }
        else {
            report.log(LogLevel(silent), u"%s, %s, type: %s, expected: %s", prefix, cause, EventTypeNames().bitMaskNames(evtype), EventTypeNames().name(type));
        }
    }
    return cause == nullptr;
}


//----------------------------------------------------------------------------
// Open and initialize the Reactor.
//----------------------------------------------------------------------------

bool ts::Reactor::open()
{
    if (_is_open) {
        _report.error(u"reactor already open");
        return false;
    }
    else {
        return _is_open = _guts->open();
    }
}

bool ts::Reactor::Guts::open()
{
    bool success = true;

#if defined(TS_USE_EPOLL)

    // Initialize the epoll event queue.
    assert(epoll_fd < 0);
    if ((epoll_fd = ::epoll_create1(EPOLL_CLOEXEC)) < 0) {
        report.error(u"error creating epoll queue: %s", SysErrorCodeMessage());
        success = false;
    }
    // Unlike kqueue, the epoll file descriptor is inherited by a child created with fork().
    // Register it to be closed in child process after fork().
    AddCloseOnForkExec(epoll_fd, false);

#elif defined(TS_USE_KQUEUE)

    // Initialize the kernel queue.
    // Unlike epoll, the kqueue file descriptor is not inherited by a child created with fork().
    assert(kqueue_fd < 0);
    if ((kqueue_fd = ::kqueue()) < 0) {
        report.error(u"error creating kqueue(): %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    assert(!WinHandleValid(iocp_handle));
    iocp_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    // CreateIoCompletionPort() is documented to return NULL on error but let's be conservative.
    if (!WinHandleValid(iocp_handle)) {
        report.error(u"error in CreateIoCompletionPort(): %s", SysErrorCodeMessage());
        iocp_handle = nullptr;
        success = false;
    }

#endif

    return success;
}


//----------------------------------------------------------------------------
// Verify that the reactor is initialized.
//----------------------------------------------------------------------------

bool ts::Reactor::checkOpen(bool silent)
{
    if (!_is_open) {
        _report.log(LogLevel(silent), u"reactor is not initialized");
    }
    return _is_open;
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

bool ts::Reactor::Guts::close(bool silent)
{
    bool success = true;

    // Force system close of pending events, if there are any.
    for (auto evd : events) {
        if (validateEventData(evd, EVT_NONE, silent)) {
            if (evd->type == EVT_EVENT) {
                sysDeleteEvent(evd, silent);
            }
            else if (evd->type == EVT_TIMER) {
                sysDeleteTimer(evd, silent);
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
    events.clear();

#if defined(TS_USE_EPOLL)

    // Clear map from file descriptors to EventData.
    file_descs_map.clear();

    // Cleanup epoll queue.
    assert(epoll_fd >= 0);
    RemoveCloseOnForkExec(epoll_fd);
    ::close(epoll_fd);
    epoll_fd = -1;

#elif defined(TS_USE_KQUEUE)

    // Cleanup kqueue().
    assert(kqueue_fd >= 0);
    ::close(kqueue_fd);
    kqueue_fd = -1;

#elif defined(TS_USE_IOCP)

    // Close the I/O completion port.
    assert(WinHandleValid(iocp_handle));
    ::CloseHandle(iocp_handle);
    iocp_handle = nullptr;

#endif

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

void ts::Reactor::Guts::processEventLoop()
{
    // List of signalled events. The type SysEventStructure depends on the operating system.
    std::vector<Guts::SysEventStructure> sysevents(MIN_WAIT_EVENTS);

    // Main event loop.
    while (!reactor._exit_requested) {

        // Adjust the size of the system structures describing events to wait for.
        if (sysevents.size() < events.size() || sysevents.size() > events.size() + 10 * MIN_WAIT_EVENTS) {
            // Current vector is too small or significantly too large.
            sysevents.resize(events.size() + MIN_WAIT_EVENTS);
        }

        // Number of valid returned events.
        size_t evcount = 0;

        // Wait for events. No timeouts, use timers to timeout.
        #if defined(TS_USE_EPOLL)

            const int status = ::epoll_wait(epoll_fd, &sysevents[0], int(sysevents.size()), -1);
            if (status >= 0) {
                evcount = std::min(sysevents.size(), size_t(status));
            }
            else if (errno == EINTR) {
                // Interrupted by a signal, retry event loop.
                continue;
            }
            else {
                report.error(u"error while waiting for epoll: %s", SysErrorCodeMessage());
                reactor._exit_requested = true;
                reactor._exit_success = false;
                break;
            }

        #elif defined(TS_USE_KQUEUE)

            const int status = ::kevent(kqueue_fd, nullptr, 0, &sysevents[0], int(sysevents.size()), nullptr);
            if (status >= 0) {
                evcount = std::min(sysevents.size(), size_t(status));
            }
            else if (errno == EINTR) {
                // Interrupted by a signal, retry event loop.
                continue;
            }
            else {
                report.error(u"error while waiting for kevent: %s", SysErrorCodeMessage());
                reactor._exit_requested = true;
                reactor._exit_success = false;
                break;
            }

        #elif defined(TS_USE_IOCP)

            // Important: specify to wait in alertable state (last parameter) to allow timer APC.
            ::ULONG retcount = 0;
            if (::GetQueuedCompletionStatusEx(iocp_handle, &sysevents[0], ::ULONG(sysevents.size()), &retcount, INFINITE, true)) {
                evcount = std::min(sysevents.size(), size_t(retcount));
            }
            else if (::GetLastError() == STATUS_USER_APC) {
                // The alertable state was interrupted by a user APC (here probably a timer completion), retry event loop.
                continue;
            }
            else {
                // Actual error.
                report.error(u"error while waiting on I/O completion port: %s", SysErrorCodeMessage());
                reactor._exit_requested = true;
                reactor._exit_success = false;
                break;
            }

        #endif

        reactor.trace(u"processEventLoop: got %d events", evcount);

        // Process all returned events.
        for (size_t i = 0; i < evcount && !reactor._exit_requested; i++) {

            // Get and check the associated EventData block.
            auto& sysev(sysevents[i]);
            EventData* sysevd = GetEventData(sysev);
            if (!validateEventData(sysevd, EVT_NONE, false)) {
                continue;
            }
            const EventId id(sysevd);

            // Warning: events can be invalidated if the user-handler updates the reactor. So, keep a copy of it.
            const EventData evd(*sysevd);

            // Process the event, based on a copy of the EventData block.
            bool success = true;
            if (evd.type == EVT_EVENT) {
                #if defined(TS_USE_EPOLL)
                    // Try to read from the event file descriptor to verify that the event is actually pending. Loop on EAGAIN.
                    uint64_t data = 0;
                    success = ::read(evd.fd, &data, sizeof(data)) > 0;
                    if (!success && errno != EAGAIN) {
                        // Actual error, delete the event.
                        report.error(u"error reading user event: %s", SysErrorCodeMessage());
                        sysDeleteEvent(sysevd, false);
                        deleteEventData(sysevd, EVT_EVENT);
                    }
                #endif
                if (success) {
                    // Call the user event callback.
                    ReactorEventHandlerInterface* handler = dynamic_cast<ReactorEventHandlerInterface*>(evd.gen_handler);
                    if (handler != nullptr) {
                        handler->handleUserEvent(reactor, id);
                    }
                }
            }
            else if (evd.type == EVT_TIMER) {
                #if defined(TS_USE_EPOLL)
                    // Try to read the number of expirations.
                    uint64_t data = 0;
                    success = ::read(evd.fd, &data, sizeof(data)) > 0;
                    if (!success && errno != EAGAIN) {
                        // Actual error, delete the timer.
                        report.error(u"error reading timer: %s", SysErrorCodeMessage());
                        sysDeleteTimer(sysevd, false);
                        deleteEventData(sysevd, EVT_TIMER);
                    }
                #endif
                if (success) {
                    if (!evd.repeat) {
                        // In case of one-shot event, the timer must be explicitly closed.
                        // Except with kqueue() where one-shot timers are automatically disarmed.
                        #if !defined(TS_USE_KQUEUE)
                            sysDeleteTimer(sysevd, true);
                        #endif
                        // Manually remove the one-shot timer before calling the handler.
                        deleteEventData(sysevd, EVT_TIMER);
                    }
                    // Call the timer callback.
                    ReactorTimerHandlerInterface* handler = dynamic_cast<ReactorTimerHandlerInterface*>(evd.gen_handler);
                    if (handler != nullptr) {
                        handler->handleTimer(reactor, id);
                    }
                }
            }
            else {
                // Read and write can be on the same EventData.
                bool event_is_valid = true;
                if ((evd.type & EVT_READ) != 0) {
                    size_t io_size = NPOS;
                    int error_code = 0;
                    bool call_handler = true;

                    #if defined(TS_USE_EPOLL)
                        if ((sysev.events & EPOLLIN) == 0) {
                            // No read event, this can be an error or a write notification.
                            if ((sysev.events & EPOLLERR) != 0) {
                                error_code = -1; // unknown error
                            }
                            else {
                                call_handler = false; // must be a write event only.
                            }
                        }
                    #elif defined(TS_USE_KQUEUE)
                        // The data field contains either the pending I/O size or error code.
                        if (sysev.flags & EV_ERROR) {
                            error_code = int(sysev.data);
                        }
                        else {
                            io_size = size_t(sysev.data);
                        }
                    #elif defined(TS_USE_IOCP)
                        //@@@
                    #endif

                    // Call the I/O callback. Caution: use read_handler, not gen_handler.
                    ReactorReadHandlerInterface* handler = dynamic_cast<ReactorReadHandlerInterface*>(evd.read_handler);
                    if (call_handler && handler != nullptr) {
                        handler->handleReadEvent(reactor, id, io_size, error_code);
                    }

                    // Check if the read handler has removed the event. In that case, don't try to call the write handler.
                    // Also check if the same address has been reallocated to another EventData.
                    event_is_valid = events.contains(sysevd) &&
                        #if defined(TS_USE_EPOLL) || defined(TS_USE_KQUEUE)
                            sysevd->fd == evd.fd;
                        #elif defined(TS_USE_IOCP)
                            sysevd->handle == evd.handle;
                        #endif
                }
                if (event_is_valid && (evd.type & EVT_WRITE) != 0) {
                    size_t io_size = NPOS;
                    int error_code = 0;
                    bool call_handler = true;

                    #if defined(TS_USE_EPOLL)
                        if ((sysev.events & EPOLLOUT) == 0) {
                            // No write event, this can be an error or a read notification.
                            if ((sysev.events & EPOLLERR) != 0) {
                                error_code = -1; // unknown error
                            }
                            else {
                                call_handler = false; // must be a read event only.
                            }
                        }
                    #elif defined(TS_USE_KQUEUE)
                        // The data field contains either the remaining output buffer size or error code.
                        // We cannot really use the output buffer size, so ignore it.
                        if (sysev.flags & EV_ERROR) {
                            error_code = int(sysev.data);
                        }
                    #elif defined(TS_USE_IOCP)
                        //@@@
                    #endif

                    // Call the I/O callback.
                    ReactorWriteHandlerInterface* handler = dynamic_cast<ReactorWriteHandlerInterface*>(evd.gen_handler);
                    if (call_handler && handler != nullptr) {
                        handler->handleWriteEvent(reactor, id, io_size, error_code);
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Add a timer.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newTimerImpl(ReactorTimerHandlerInterface* handler, cn::milliseconds duration, bool repeat)
{
    EventId id;

    // Check parameters.
    if (!checkOpen(false)) {
        return id;
    }
    if (duration <= cn::milliseconds::zero()) {
        _report.error(u"invalid reactor timer value");
        return id;
    }

    // Create the EventData
    EventData* evd = _guts->newEventData(EVT_TIMER, SYS_SOCKET_INVALID);

#if defined(TS_USE_EPOLL)

    // On Linux, timers are file descriptors which can be monitored.
    evd->fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (evd->fd < 0) {
        _report.error(u"error in timerfd_create: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
    }

    // Need to register the file descriptor we set in EventData.
    _guts->file_descs_map.insert(std::make_pair(Guts::FileDescIndex(EVT_TIMER, evd->fd), evd));

    // Register time in epoll queue.
    ::epoll_event event;
    TS_ZERO(event);
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = evd;
    if (::epoll_ctl(_guts->epoll_fd, EPOLL_CTL_ADD, evd->fd, &event) < 0) {
        _report.error(u"error adding  timer to epoll: %s", SysErrorCodeMessage());
        ::close(evd->fd);
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
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
        _report.error(u"error in timerfd_settime: %s", SysErrorCodeMessage());
        ::epoll_ctl(_guts->epoll_fd, EPOLL_CTL_DEL, evd->fd, nullptr);
        ::close(evd->fd);
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
    }

    // Make sure the file descriptor is closed on fork().
    AddCloseOnForkExec(evd->fd, false);

#elif defined(TS_USE_KQUEUE)

    // With kqueue(), the timers ids are arbitrary values. Register the event in the kqueue.
    // In kevent, the default value for timer is in milliseconds.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd), EVFILT_TIMER, EV_ADD | EV_ENABLE | (repeat ? 0 : EV_ONESHOT), 0, kevent_data_t(duration.count()), evd);
    if (::kevent(_guts->kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _report.error(u"error registering timer in kqueue: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
    }

#elif defined(TS_USE_IOCP)

    // Create a waitable timer.
    evd->handle = ::CreateWaitableTimerW(nullptr, true, nullptr);
    if (!WinHandleValid(evd->handle)) {
        _report.error(u"error creating waitable timer: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
    }

    // Build the timer expiration time: Use 100 nanosecond intervals.
    // - Positive values indicate UTC absolute time in the format described by FILETIME.
    // - Negative values indicate relative time.
    ::LARGE_INTEGER expiration;
    expiration.QuadPart = -(duration.count() * 10'000); // 10,000 100-ns units = 1 ms

    // Set the timer expîration.
    if (!::SetWaitableTimer(evd->handle, &expiration, ::ULONG(repeat ? duration.count() : 0), Reactor::Guts::WinTimerCompletion, evd, false)) {
        _report.error(u"error setting waitable timer: %s", SysErrorCodeMessage());
        ::CloseHandle(evd->handle);
        _guts->deleteEventData(evd, EVT_TIMER);
        return id;
    }

#endif

    // Timer is finally valid.
    evd->type = EventType(evd->type | EVT_TIMER);
    evd->gen_handler = handler;
    evd->repeat = repeat;
    id._ptr = evd;
    return id;
}


//----------------------------------------------------------------------------
// Cancel a timer.
//----------------------------------------------------------------------------

bool ts::Reactor::cancelTimer(EventId id, bool silent)
{
    // Check parameters.
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    if (!checkOpen(silent) || !_guts->validateEventData(evd, EVT_TIMER, silent)) {
        return false;
    }

    // Cancel and delete the timer.
    bool success = _guts->sysDeleteTimer(evd, silent);
    _guts->deleteEventData(evd, EVT_TIMER);
    return success;
}

bool ts::Reactor::Guts::sysDeleteTimer(EventData* evd, bool silent)
{
    bool success = true;

#if defined(TS_USE_EPOLL)

    // Remove timer from epoll queue.
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evd->fd, nullptr) < 0) {
        report.log(LogLevel(silent), u"error removing timer from epoll: %s", SysErrorCodeMessage());
        success = false;
    }
    RemoveCloseOnForkExec(evd->fd);
    ::close(evd->fd);

#elif defined(TS_USE_KQUEUE)

    // Delete the timer from the kqueue.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd), EVFILT_TIMER, EV_DELETE | EV_DISABLE, 0, 0, evd);
    if (::kevent(kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        report.log(LogLevel(silent), u"error deleting user event in kqueue: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    // Cancel the timer if pending. Ignore errors (timer may not be pending).
    // Note that CancelWaitableTimer cancels outstanding APCs.
    // Therefore, WinTimerCompletion won't be called on this one, even is already pending.
    ::CancelWaitableTimer(evd->handle);
    ::CloseHandle(evd->handle);
    evd->handle = nullptr;

#endif

    return success;
}


//----------------------------------------------------------------------------
// Windows-specific: Asynchronous timer completion routine.
//----------------------------------------------------------------------------

#if defined(TS_USE_IOCP)

void ts::Reactor::Guts::WinTimerCompletion(::LPVOID arg, ::DWORD timer_low, ::DWORD time_high)
{
    // We are in asynchronous context, don't access the Reactor structures, don't display errors.
    EventData* evd = reinterpret_cast<EventData*>(arg);
    EventType type = EVT_NONE;
    if (EventData::Error(evd, EVT_TIMER, type) == nullptr) {
        // Push a notification using the EventData.
        ::PostQueuedCompletionStatus(evd->reactor->_guts->iocp_handle, 0, ::ULONG_PTR(evd), nullptr);
    }
}

#endif

//----------------------------------------------------------------------------
// Add a user event in the reactor.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newEvent(ReactorEventHandlerInterface* handler)
{
    EventId id;

    // Check parameters.
    if (!checkOpen(false)) {
        return id;
    }

    // Create the EventData
    EventData* evd = _guts->newEventData(EVT_EVENT, SYS_SOCKET_INVALID);

#if defined(TS_USE_EPOLL)

    // Open a event file descriptor.
    evd->fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evd->fd < 0) {
        _report.error(u"error creating user event with eventfd(): %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_EVENT);
        return id;
    }

    // Need to register the file descriptor we set in EventData.
    _guts->file_descs_map.insert(std::make_pair(Guts::FileDescIndex(EVT_EVENT, evd->fd), evd));

    // Register event in epoll queue.
    ::epoll_event event;
    TS_ZERO(event);
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = evd;
    if (::epoll_ctl(_guts->epoll_fd, EPOLL_CTL_ADD, evd->fd, &event) < 0) {
        _report.error(u"error adding user event to epoll: %s", SysErrorCodeMessage());
        ::close(evd->fd);
        _guts->deleteEventData(evd, EVT_EVENT);
        return id;
    }

    // Make sure the file descriptor is closed on fork().
    AddCloseOnForkExec(evd->fd, false);

#elif defined(TS_USE_KQUEUE)

    // With kqueue(), the user-defined events are arbitrary values. Register the event in the kqueue.
    // Note: user data is nullptr here, it is non-null on trigger only.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd), EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(_guts->kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _report.error(u"error registering user event in kqueue: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_EVENT);
        return id;
    }

#elif defined(TS_USE_IOCP)

    // An event has no specific system handle. The event is explicitly pushed in the I/O completion queue.

#endif

    // Event is finally valid.
    evd->type = EventType(evd->type | EVT_EVENT);
    evd->gen_handler = handler;
    id._ptr = evd;
    return id;
}


//----------------------------------------------------------------------------
// Signal a user event in the reactor.
// Important: this method can be invoked from any thread.
//----------------------------------------------------------------------------

bool ts::Reactor::signalEvent(EventId id)
{
    bool success = true;

    // Check parameters. However, do not try to access the _event structure because
    // we can be invoked from another thread. So, don't try to check that the Id
    // is actually a user event.
    if (!id.isValid()) {
        _report.error(u"internal error: invalid id in Reactor::signalEvent");
        return false;
    }

#if defined(TS_USE_EPOLL)

    // Write the 64-bit value 1 in the event file descriptor.
    uint64_t value = 1;
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    if (::write(evd->fd, &value, sizeof(value)) < 0) {
        _report.error(u"error triggering user event in epoll: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_KQUEUE)

    // Simple trigger, only use the kqueue file descriptor, don't do anything else.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(id._ptr), EVFILT_USER, 0, NOTE_TRIGGER, 0, id._ptr);
    if (::kevent(_guts->kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _report.error(u"error triggering user event in kqueue: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    // Push a notification using the EventData.
    if (!::PostQueuedCompletionStatus(_guts->iocp_handle, 0, ::ULONG_PTR(id._ptr), nullptr)) {
        _report.error(u"error triggering user event in PostQueuedCompletionStatus: %s", SysErrorCodeMessage());
        success = false;
    }

#endif

    return success;
}


//----------------------------------------------------------------------------
// Delete a user event.
//----------------------------------------------------------------------------

bool ts::Reactor::deleteEvent(EventId id, bool silent)
{
    // Check parameters.
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    if (!checkOpen(silent) || !_guts->validateEventData(evd, EVT_EVENT, silent)) {
        return false;
    }

    // Delete the event.
    bool success = _guts->sysDeleteEvent(evd, silent);
    _guts->deleteEventData(evd, EVT_EVENT);
    return success;
}

bool ts::Reactor::Guts::sysDeleteEvent(EventData* evd, bool silent)
{
    bool success = true;

#if defined(TS_USE_EPOLL)

    // Remove event from epoll queue.
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evd->fd, nullptr) < 0) {
        report.log(LogLevel(silent), u"error removing user event from epoll: %s", SysErrorCodeMessage());
        success = false;
    }
    RemoveCloseOnForkExec(evd->fd);
    ::close(evd->fd);

#elif defined(TS_USE_KQUEUE)

    // Delete the event from the kqueue.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd), EVFILT_USER, EV_DELETE | EV_DISABLE, 0, 0, nullptr);
    if (::kevent(kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        report.log(LogLevel(silent), u"error deleting user event in kqueue: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    // Nothing to close at system level.

#endif

    return success;
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newReadNotify(ReactorReadHandlerInterface* handler, SysSocketType sock)
{
    EventId id;

    // Check parameters.
    if (!checkOpen(false)) {
        return id;
    }

    // Create the EventData
    EventData* evd = _guts->newEventData(EVT_READ, sock);

#if defined(TS_USE_EPOLL)

    // Add or update event in epoll queue.
    if ((evd->type & EVT_READ) == 0) {
        // The file descriptor was not already monitored for read.
        const bool also_write = (evd->type & EVT_WRITE) != 0;
        const int op = also_write ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        ::epoll_event event;
        TS_ZERO(event);
        event.events = EPOLLIN | EPOLLET;
        if (also_write) {
            event.events |= EPOLLOUT;
        }
        event.data.ptr = evd;
        if (::epoll_ctl(_guts->epoll_fd, op, evd->fd, &event) < 0) {
            _report.error(u"error adding read notification to epoll: %s", SysErrorCodeMessage());
            if (!also_write) {
                // The EventData was just created, remove it.
                _guts->deleteEventData(evd, EVT_READ);
            }
            return id;
        }
    }

#elif defined(TS_USE_KQUEUE)

    // If repeat is false, we must not use EV_ONESHOT. This would delete the filter from the kqueue.
    // Instead, we will use EV_DISABLE when the event is fired.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(sock), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, evd);
    if (::kevent(_guts->kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _report.error(u"error registering read event in kqueue: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_READ);
        return id;
    }

#elif defined(TS_USE_IOCP)

    //@@@

#endif

    // Event is finally valid.
    evd->type = EventType(evd->type | EVT_READ);
    evd->read_handler = handler;
    id._ptr = evd;
    return id;
}


//----------------------------------------------------------------------------
// Delete a notification of read-ready or read-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::deleteReadNotify(EventId id, bool silent)
{
    // Check parameters.
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    if (!checkOpen(silent) || !_guts->validateEventData(evd, EVT_READ, silent)) {
        return false;
    }

    // Delete the event.
    bool success = _guts->sysDeleteRead(evd, silent);
    _guts->deleteEventData(evd, EVT_READ);
    return success;
}

bool ts::Reactor::Guts::sysDeleteRead(EventData* evd, bool silent)
{
    bool success = true;

#if defined(TS_USE_EPOLL)

    // Delete or update event in epoll queue.
    const bool also_write = (evd->type & EVT_WRITE) != 0;
    const int op = also_write ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ::epoll_event event;
    TS_ZERO(event);
    if (also_write) {
        event.events = EPOLLOUT | EPOLLET;
        event.data.ptr = evd;
    }
    if (::epoll_ctl(epoll_fd, op, evd->fd, &event) < 0) {
        report.log(LogLevel(silent), u"error removing read notification from epoll: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_KQUEUE)

    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd->fd), EVFILT_READ, EV_DELETE | EV_DISABLE, 0, 0, nullptr);
    if (::kevent(kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        report.log(LogLevel(silent), u"error deleting read event in kqueue: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    //@@@@

#endif

    return success;
}


//----------------------------------------------------------------------------
// Add in the reactor a notification of write-ready or read-completion.
//----------------------------------------------------------------------------

ts::EventId ts::Reactor::newWriteNotify(ReactorWriteHandlerInterface* handler, SysSocketType sock)
{
    EventId id;

    // Check parameters.
    if (!checkOpen(false)) {
        return id;
    }

    // Create the EventData.
    EventData* evd = _guts->newEventData(EVT_WRITE, sock);

#if defined(TS_USE_EPOLL)

    // Add or update event in epoll queue.
    if ((evd->type & EVT_WRITE) == 0) {
        // The file descriptor was not already monitored for write.
        const bool also_read = (evd->type & EVT_READ) != 0;
        const int op = also_read ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        ::epoll_event event;
        TS_ZERO(event);
        event.events = EPOLLOUT | EPOLLET;
        if (also_read) {
            event.events |= EPOLLIN;
        }
        event.data.ptr = evd;
        if (::epoll_ctl(_guts->epoll_fd, op, evd->fd, &event) < 0) {
            _report.error(u"error adding write notification to epoll: %s", SysErrorCodeMessage());
            if (!also_read) {
                // The EventData was just created, remove it.
                _guts->deleteEventData(evd, EVT_WRITE);
            }
            return id;
        }
    }

#elif defined(TS_USE_KQUEUE)

    // If repeat is false, we must not use EV_ONESHOT. This would delete the filter from the kqueue.
    // Instead, we will use EV_DISABLE when the event is fired.
    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(sock), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, evd);
    if (::kevent(_guts->kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        _report.error(u"error registering write event in kqueue: %s", SysErrorCodeMessage());
        _guts->deleteEventData(evd, EVT_WRITE);
        return id;
    }

#elif defined(TS_USE_IOCP)

    //@@@

#endif

    // Event is finally valid.
    evd->type = EventType(evd->type | EVT_WRITE);
    evd->gen_handler = handler;
    id._ptr = evd;
    return id;
}


//----------------------------------------------------------------------------
// Delete a notification of write-ready or write-completion.
//----------------------------------------------------------------------------

bool ts::Reactor::deleteWriteNotify(EventId id, bool silent)
{
    // Check parameters.
    EventData* evd = reinterpret_cast<EventData*>(id._ptr);
    if (!checkOpen(silent) || !_guts->validateEventData(evd, EVT_WRITE, silent)) {
        return false;
    }

    // Delete the event.
    bool success = _guts->sysDeleteWrite(evd, silent);
    _guts->deleteEventData(evd, EVT_WRITE);
    return success;
}

bool ts::Reactor::Guts::sysDeleteWrite(EventData* evd, bool silent)
{
    bool success = true;

#if defined(TS_USE_EPOLL)

    // Delete or update event in epoll queue.
    const bool also_read = (evd->type & EVT_READ) != 0;
    const int op = also_read ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ::epoll_event event;
    TS_ZERO(event);
    if (also_read) {
        event.events = EPOLLIN | EPOLLET;
        event.data.ptr = evd;
    }
    if (::epoll_ctl(epoll_fd, op, evd->fd, &event) < 0) {
        report.log(LogLevel(silent), u"error removing write notification from epoll: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_KQUEUE)

    kevent_t ev;
    EV_SET(&ev, kevent_ident_t(evd->fd), EVFILT_WRITE, EV_DELETE | EV_DISABLE, 0, 0, nullptr);
    if (::kevent(kqueue_fd, &ev, 1, nullptr, 0, nullptr) < 0) {
        report.log(LogLevel(silent), u"error deleting write event in kqueue: %s", SysErrorCodeMessage());
        success = false;
    }

#elif defined(TS_USE_IOCP)

    //@@@@

#endif

    return success;
}
