//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveSocketBase.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveSocketBase::ReactiveSocketBase(Reactor& reactor, Socket& socket, Object* owner) :
    ReactiveBase(reactor, owner),
    _generic_socket(socket)
{
    // The socket must be set in non-blocking mode, a reactor can never block.
    _generic_socket.setNonBlocking(true);

    // Redirect all socket errors to the reactor.
    _generic_socket.setReport(&ReactiveBase::reactor());
}

ts::ReactiveSocketBase::~ReactiveSocketBase()
{
    if (_generic_socket.isOpen()) {
        reactor().trace(u"warning: reactive socket is destroyed while the underlying socket is still open");
    }

    // Delete all registered ids in the reactor.
    deactivateAll(true);
}


//----------------------------------------------------------------------------
// Search and remove a shared_ptr to IOSB, based on an IOSB address.
///----------------------------------------------------------------------------

std::shared_ptr<ts::ReactiveSocketBase::IOSB> ts::ReactiveSocketBase::removeFromQueue(IOQueue& queue, IOSB* iosb)
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

void ts::ReactiveSocketBase::deactivateAll(bool silent)
{
    deactivateReadReady(silent);
    deactivateWriteReady(silent);
    deactivateAsynchronousIO(silent);
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveSocketBase::activateReadReady()
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (!_read_ready_id.isValid()) {
            _read_ready_id = reactor().newReadNotify(this, _generic_socket.getSocket());
            return _read_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveSocketBase::deactivateReadReady(bool silent)
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (_read_ready_id.isValid()) {
            reactor().deleteReadNotify(_read_ready_id, silent);
            _read_ready_id.invalidate();
        }
    }
}

bool ts::ReactiveSocketBase::activateWriteReady()
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (!_write_ready_id.isValid()) {
            _write_ready_id = reactor().newWriteNotify(this, _generic_socket.getSocket());
            return _write_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveSocketBase::deactivateWriteReady(bool silent)
{
    if constexpr (Reactor::UseNonBlockingIO()) {
        if (_write_ready_id.isValid()) {
            reactor().deleteWriteNotify(_write_ready_id, silent);
            _write_ready_id.invalidate();
        }
    }
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveSocketBase::activateAsynchronousIO()
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (!_async_io_id.isValid()) {
            _async_io_id = reactor().newAsynchronousIO(this, _generic_socket.getSocket());
            return _async_io_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveSocketBase::deactivateAsynchronousIO(bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().deleteAsynchronousIO(_async_io_id, silent);
            _async_io_id.invalidate();
        }
    }
}

void ts::ReactiveSocketBase::cancelAsynchronousIO(bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().cancelAsynchronousIO(_async_io_id, silent);
        }
    }
}

bool ts::ReactiveSocketBase::cancelAndWaitAsynchronousIO(NonBlockingDevice::IOSB& iosb, bool silent)
{
    if constexpr (Reactor::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().cancelAndWaitAsynchronousIO(_async_io_id, iosb, silent);
        }
    }
    return true;
}
