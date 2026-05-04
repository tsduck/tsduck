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

ts::NonBlockingDevice::AncillaryData::~AncillaryData()
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


//----------------------------------------------------------------------------
// Reset the IOSB for a new I/O operation.
//----------------------------------------------------------------------------

void ts::NonBlockingDevice::IOSB::reset()
{
    pending = false;
    app_data.reset();
#if defined(TS_WINDOWS)
    TS_ZERO(overlap);
    io_data.reset();
#endif
}


//----------------------------------------------------------------------------
// Support for Windows asynchronous I/O.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

// IOSB constructor.
ts::NonBlockingDevice::IOSB::IOSB()
{
    TS_ZERO(overlap);
}

// IOSB destructor.
ts::NonBlockingDevice::IOSB::~IOSB()
{
    // Make sure that the iosb_marker becomes invalid.
    iosb_marker = 0;
}

// Check if an OVERLAPPED structure is part of an IOSB.
ts::NonBlockingDevice::IOSB* ts::NonBlockingDevice::IOSB::ParentIOSB(::OVERLAPPED* overlap)
{
    static constexpr size_t marker_offset = offsetof(IOSB, iosb_marker);
    static constexpr size_t overlap_offset = offsetof(IOSB, overlap);
    static_assert(marker_offset < overlap_offset);

    if (overlap == nullptr || size_t(overlap) < overlap_offset) {
        return nullptr;
    }
    try {
        char* ov = reinterpret_cast<char*>(overlap);
        if (*reinterpret_cast<uint32_t*>(ov - overlap_offset + marker_offset) == iosb_marker_value) {
            // Found the expected value for an IOSB marker, we probably are in an IOSB.
            return reinterpret_cast<IOSB*>(ov - overlap_offset);
        }
        else {
            return nullptr;
        }
    }
    catch (...) {
        // Probably an invalid memory access.
        return nullptr;
    }
}

#endif
