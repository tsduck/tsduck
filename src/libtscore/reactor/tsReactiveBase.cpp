//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveBase.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveBase::ReactiveBase(Reactor& reactor, Socket& socket, Object* owner) :
    OwnedObject(owner),
    _reactor(reactor),
    _generic_socket(socket)
{
    // The socket must be set in non-blocking mode, a reactor can never block.
    _generic_socket.setNonBlocking(true);

    // Redirect all socket errors to the reactor.
    _generic_socket.setReport(&_reactor);
}

ts::ReactiveBase::~ReactiveBase()
{
    if (_generic_socket.isOpen()) {
        _reactor.trace(u"warning: reactive socket is destroyed while the underlying socket is still open");
    }

    // Delete all registered ids in the reactor.
    deactivateAll(true);
}


//----------------------------------------------------------------------------
// Search and remove a shared_ptr to IOSB, based on an IOSB address.
///----------------------------------------------------------------------------

std::shared_ptr<ts::ReactiveBase::IOSB> ts::ReactiveBase::removeFromQueue(IOQueue& queue, IOSB* iosb)
{
    std::shared_ptr<IOSB> req;
    // Use reverse iterator since the completed I/O is likely on the front.
    for (auto it = queue.rbegin(); it != queue.rend(); ++it) {
        if (it->get() == iosb) {
            req = *it;
            // C++ trick to erase using a reverse iterator.
            queue.erase(std::next(it).base());
            break;
        }
    }
    return req;
}


//----------------------------------------------------------------------------
// Delete and invalidate all registrations.
//----------------------------------------------------------------------------

void ts::ReactiveBase::deactivateAll(bool silent)
{
    deactivateReadReady(silent);
    deactivateWriteReady(silent);
    deactivateAsynchronousIO(silent);
    deactivateCompletedIO(silent);
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveBase::activateReadReady()
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (!_read_ready_id.isValid()) {
            _read_ready_id = _reactor.newReadNotify(this, _generic_socket.getSocket());
            return _read_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveBase::deactivateReadReady(bool silent)
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (_read_ready_id.isValid()) {
            _reactor.deleteReadNotify(_read_ready_id, silent);
            _read_ready_id.invalidate();
        }
    }
}

bool ts::ReactiveBase::activateWriteReady()
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (!_write_ready_id.isValid()) {
            _write_ready_id = _reactor.newWriteNotify(this, _generic_socket.getSocket());
            return _write_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveBase::deactivateWriteReady(bool silent)
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (_write_ready_id.isValid()) {
            _reactor.deleteWriteNotify(_write_ready_id, silent);
            _write_ready_id.invalidate();
        }
    }
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveBase::activateAsynchronousIO()
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (!_async_io_id.isValid()) {
            _async_io_id = _reactor.newAsynchronousIO(this, _generic_socket.getSocket());
            return _async_io_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveBase::deactivateAsynchronousIO(bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            _reactor.deleteAsynchronousIO(_async_io_id, silent);
            _async_io_id.invalidate();
        }
    }
}

void ts::ReactiveBase::cancelAsynchronousIO(bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            _reactor.cancelAsynchronousIO(_async_io_id, silent);
        }
    }
}

bool ts::ReactiveBase::cancelAndWaitAsynchronousIO(NonBlockingDevice::IOSB& iosb, bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            _reactor.cancelAndWaitAsynchronousIO(_async_io_id, iosb, silent);
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Signal the completed event, so that processCompletedIO() is called.
//----------------------------------------------------------------------------

// The completion event is created the first time it is used.
bool ts::ReactiveBase::signalCompletedIO()
{
    if (!_completed_io_id.isValid()) {
        _completed_io_id = _reactor.newEvent(this);
        if (!_completed_io_id.isValid()) {
            return false;
        }
    }
    return _reactor.signalEvent(_completed_io_id);
}

// Deactivate and delete the user event for processCompletedIO().
void ts::ReactiveBase::deactivateCompletedIO(bool silent)
{
    if (_completed_io_id.isValid()) {
        _reactor.deleteEvent(_completed_io_id, silent);
        _completed_io_id.invalidate();
    }
}

// Called when an internal user-defined event is specified.
void ts::ReactiveBase::handleUserEvent(Reactor& reactor, EventId id)
{
    if (id == _completed_io_id) {
        processCompletedIO();
    }
}

// The default implementation does nothing.
void ts::ReactiveBase::processCompletedIO()
{
}
