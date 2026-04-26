//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNonBlockingDevice.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::NonBlockingDevice::~NonBlockingDevice()
{
}


//----------------------------------------------------------------------------
// Set the device in non-blocking mode.
//----------------------------------------------------------------------------

bool ts::NonBlockingDevice::setNonBlocking(bool non_blocking)
{
    if (allowSetNonBlocking()) {
        _is_non_blocking = non_blocking;
        return true;
    }
    else {
        return false;
    }
}

bool ts::NonBlockingDevice::allowSetNonBlocking() const
{
    // The default implementation always allows setting the non-blocking mode.
    return true;
}


//----------------------------------------------------------------------------
// Check the blocking mode of a device.
//----------------------------------------------------------------------------

bool ts::NonBlockingDevice::checkNonBlocking(bool non_blocking, const UChar* opname)
{
#if defined(TS_WINDOWS)
    // To be removed when we fully support asynchronous I/O on Windows.
    if (_is_non_blocking) {
        report().error(u"internal error: asynchronous I/O not yet supported on Windows");
        return false;
    }
#endif

    if (non_blocking == _is_non_blocking) {
        return true;
    }
    else {
        report().error(u"internal error: %s called in %sblocking mode", opname, non_blocking ? u"" : u"non-");
        return false;
    }
}

bool ts::NonBlockingDevice::checkNonBlocking(IOSB* iosb, const UChar* opname)
{
    if (iosb != nullptr) {
        iosb->pending = false;
#if defined(TS_WINDOWS)
        TS_ZERO(iosb->overlap);
#endif
    }
    return checkNonBlocking(iosb != nullptr, opname);
}


//----------------------------------------------------------------------------
// Set a system file descriptor or socket handle in non-blocking mode.
//----------------------------------------------------------------------------

bool ts::NonBlockingDevice::setSystemNonBlocking(SysSocketType fd, bool non_blocking)
{
#if defined(TS_UNIX)

    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        report().error(u"error getting socket flags: %s", SysErrorCodeMessage());
        return false;
    }
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }
    else {
        flags &= ~O_NONBLOCK;
    }
    if (::fcntl(fd, F_SETFL, flags) == -1) {
        report().error(u"error setting socket non-blocking mode: %s", SysErrorCodeMessage());
        return false;
    }

#elif defined(TS_WINDOWS)

    // This works on sockets only. Other devices shall be opened with flag FILE_FLAG_OVERLAPPED.
    ::u_long mode = non_blocking ? 1 : 0;
    if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
        report().error(u"error setting socket non-blocking mode: %s", SysErrorCodeMessage());
        return false;
    }

#endif

    return true;
}
