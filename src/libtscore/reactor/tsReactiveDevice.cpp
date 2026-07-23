//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveDevice.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::ReactiveDevice::ReactiveDevice(Reactor& reactor, NonBlockingDevice& device) :
    ReactiveBase(reactor),
    _device(device)
{
    // The device must be set in non-blocking mode, a reactor can never block.
    _device.setNonBlocking(true);

    // Redirect all device errors to the reactor.
    _device.setReport(&ReactiveBase::reactor());
}

ts::ReactiveDevice::~ReactiveDevice()
{
    if (_device.getSocket() != SYS_SOCKET_INVALID) {
        reactor().trace(u"warning: reactive device is destroyed while the underlying device is still open");
    }

    // Delete all registered ids in the reactor.
    deactivateAll(true);
}


//----------------------------------------------------------------------------
// Search and remove a shared_ptr to IOSB, based on an IOSB address.
///----------------------------------------------------------------------------

std::shared_ptr<ts::ReactiveDevice::IOSB> ts::ReactiveDevice::removeFromQueue(IOQueue& queue, IOSB* iosb)
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

void ts::ReactiveDevice::deactivateAll(bool silent)
{
    deactivateReadReady(silent);
    deactivateWriteReady(silent);
    deactivateAsynchronousIO(silent);
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveDevice::activateReadReady()
{
    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        if (!device().isSupportedByReactor()) {
            return true;
        }
        if (!_read_ready_id.isValid()) {
            _read_ready_id = reactor().newReadNotify(this, _device.getSocket());
            return _read_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveDevice::deactivateReadReady(bool silent)
{
    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        if (_read_ready_id.isValid()) {
            reactor().deleteReadNotify(_read_ready_id, silent);
            _read_ready_id.invalidate();
        }
    }
}

bool ts::ReactiveDevice::activateWriteReady()
{
    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        if (!device().isSupportedByReactor()) {
            return true;
        }
        if (!_write_ready_id.isValid()) {
            _write_ready_id = reactor().newWriteNotify(this, _device.getSocket());
            return _write_ready_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveDevice::deactivateWriteReady(bool silent)
{
    if constexpr (ReactorSupport::UseNonBlockingIO()) {
        if (_write_ready_id.isValid()) {
            reactor().deleteWriteNotify(_write_ready_id, silent);
            _write_ready_id.invalidate();
        }
    }
}


//----------------------------------------------------------------------------
// Activate / delete reactor events for non-blocking I/O.
//----------------------------------------------------------------------------

bool ts::ReactiveDevice::activateAsynchronousIO()
{
    if constexpr (ReactorSupport::UseAsynchronousIO()) {
        if (!device().isSupportedByReactor()) {
            return true;
        }
        if (!_async_io_id.isValid()) {
            _async_io_id = reactor().newAsynchronousIO(this, _device.getSocket());
            return _async_io_id.isValid();
        }
    }
    return true;
}

void ts::ReactiveDevice::deactivateAsynchronousIO(bool silent)
{
    if constexpr (ReactorSupport::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().deleteAsynchronousIO(_async_io_id, silent);
            _async_io_id.invalidate();
        }
    }
}

void ts::ReactiveDevice::cancelAsynchronousIO(bool silent)
{
    if constexpr (ReactorSupport::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().cancelAsynchronousIO(_async_io_id, silent);
        }
    }
}

bool ts::ReactiveDevice::cancelAndWaitAsynchronousIO(NonBlockingDevice::IOSB& iosb, bool silent)
{
    if constexpr (ReactorSupport::UseAsynchronousIO()) {
        if (_async_io_id.isValid()) {
            reactor().cancelAndWaitAsynchronousIO(_async_io_id, iosb, silent);
        }
    }
    return true;
}
