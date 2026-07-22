//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNonBlockingDevice.h"
#include "tsIPUtils.h"


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::NonBlockingDevice::~NonBlockingDevice()
{
}


//----------------------------------------------------------------------------
// Get the underlying file descriptor or device handle.
// The default implementation return the error values.
//----------------------------------------------------------------------------

ts::SysHandleType ts::NonBlockingDevice::getHandle() const
{
    return SYS_HANDLE_INVALID;
}

ts::SysSocketType ts::NonBlockingDevice::getSocket() const
{
    return SYS_SOCKET_INVALID;
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
        // Reset the low-level indicators but not react_data or async_data.
        iosb->pending = false;
        iosb->sent_size = 0;
#if defined(TS_WINDOWS)
        TS_ZERO(iosb->overlap);
#endif
    }
    return checkNonBlocking(iosb != nullptr, opname);
}


//----------------------------------------------------------------------------
// Set a system file or spcket descriptor in non-blocking mode.
//----------------------------------------------------------------------------

bool ts::NonBlockingDevice::setSystemNonBlocking(bool non_blocking)
{
#if defined(TS_UNIX)

    const SysHandleType hfd = getHandle();
    int flags = ::fcntl(hfd, F_GETFL, 0);
    if (flags == -1) {
        report().error(u"error getting file or socket flags: %s", SysErrorCodeMessage());
        return false;
    }
    if (non_blocking) {
        flags |= O_NONBLOCK;
    }
    else {
        flags &= ~O_NONBLOCK;
    }
    if (::fcntl(hfd, F_SETFL, flags) == -1) {
        report().error(u"error setting file or socket non-blocking mode: %s", SysErrorCodeMessage());
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
    error_code = SYS_SUCCESS;
    sent_size = 0;
    app_data.reset();
    react_data.reset();
#if defined(TS_WINDOWS)
    TS_ZERO(overlap);
    async_data.reset();
#endif
}


//----------------------------------------------------------------------------
// Generic system write operation.
//----------------------------------------------------------------------------

int ts::NonBlockingDevice::genericSystemWrite(const void* addr, size_t size, size_t& written_size, NonBlockingDevice::IOSB* iosb, uint64_t position)
{
    written_size = 0;
    int err_code = SYS_SUCCESS;
    const SysHandleType hfd = getHandle();

    if (!checkNonBlocking(iosb, u"system write")) {
        // Not the right blocking mode.
        return SYS_ERROR;
    }
    if (size == 0) {
        // Trivial case of zero-write.
        return SYS_SUCCESS;
    }

#if defined(TS_WINDOWS)

    if (isNonBlocking()) {
        // On Windows with asynchronous I/O, use overlapped I/O.
        assert(iosb != nullptr);
        // The absolute position of the write must be specified in the OVERLAPPED.
        iosb->overlap.Offset = ::DWORD(position & 0xFFFFFFFF);
        iosb->overlap.OffsetHigh = ::DWORD(position >> 32);
        // Start the write operation.
        if (!::WriteFile(hfd, addr, ::DWORD(size), nullptr, &iosb->overlap)) {
            err_code = TranslateError(::GetLastError());
        }
        if (SysSuccess(err_code) || err_code == SYS_PENDING_IO) {
            iosb->pending = true;
            err_code = SYS_SUCCESS;
        }
    }
    else {
        // Blocking I/O: loop until everything is sent.
        const char* data = reinterpret_cast<const char*>(addr);
        ::DWORD remain = ::DWORD(size);
        ::DWORD outsize = 0;
        while (remain > 0) {
            if (::WriteFile(hfd, data, remain, &outsize, nullptr) != 0) {
                // Normal case, some data were written
                assert(outsize <= remain);
                data += outsize;
                remain -= std::max(remain, outsize);
                written_size += size_t(outsize);
            }
            else {
                // Write error
                err_code = TranslateError(::GetLastError());
                break;
            }
        }
    }

#else  // UNIX

    const char* data = reinterpret_cast<const char*>(addr);
    size_t remain = size;
    while (remain > 0) {
        ssize_t outsize = ::write(hfd, data, remain);
        err_code = TranslateError(errno);
        if (outsize > 0) {
            // Normal case, some data were written
            assert(size_t(outsize) <= remain);
            data += outsize;
            remain -= std::max(remain, size_t(outsize));
            written_size += size_t(outsize);
            err_code = SYS_SUCCESS;
        }
        else if (isNonBlocking() && err_code == SYS_PENDING_IO) {
            // Non-blocking file with no enough available outgoing buffer space.
            assert(iosb != nullptr);
            iosb->pending = true;
            iosb->sent_size = written_size;
            err_code = SYS_SUCCESS;
            break;
        }
        else if (err_code != EINTR) {
            // Actual error (not an interrupt)
            break;
        }
    }

#endif

    if (!SysSuccess(err_code) && err_code != SYS_EOF) {
        report().error(u"write error: %s", SysErrorCodeMessage(err_code));
    }
    return err_code;
}


//----------------------------------------------------------------------------
// Generic system read operation.
//----------------------------------------------------------------------------

int ts::NonBlockingDevice::genericSystemRead(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort, NonBlockingDevice::IOSB* iosb, uint64_t position)
{
    ret_size = 0;
    int err_code = SYS_SUCCESS;
    const SysHandleType hfd = getHandle();

    if (!checkNonBlocking(iosb, u"system read")) {
        // Not the right blocking mode.
        return SYS_ERROR;
    }
    if (max_size == 0) {
        // Trivial case of zero-read.
        return SYS_SUCCESS;
    }

#if defined(TS_WINDOWS)

    if (isNonBlocking()) {
        // On Windows with asynchronous I/O, use overlapped I/O.
        assert(iosb != nullptr);
        // The absolute position of the read must be specified in the OVERLAPPED.
        iosb->overlap.Offset = ::DWORD(position & 0xFFFFFFFF);
        iosb->overlap.OffsetHigh = ::DWORD(position >> 32);
        // Start the read operation.
        if (!::ReadFile(hfd, addr, ::DWORD(max_size), nullptr, &iosb->overlap)) {
            err_code = TranslateError(::GetLastError());
        }
        if (SysSuccess(err_code) || err_code == SYS_PENDING_IO) {
            iosb->pending = true;
            return SYS_SUCCESS;
        }
        else {
            report().error(u"read error: %s", SysErrorCodeMessage(err_code));
            return err_code;
        }
    }
    else {
        // Blocking I/O.
        ::DWORD insize = 0;
        if (::ReadFile(hfd, addr, ::DWORD(max_size), &insize, nullptr) != 0) {
            // Normal case, some data were read.
            assert(insize <= ::DWORD(max_size));
            if (insize <= 0) {
                return SYS_EOF;
            }
            else {
                ret_size = size_t(insize);
                return SYS_SUCCESS;
            }
        }
        err_code = TranslateError(::GetLastError());
        if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return SYS_CANCELED;
        }
        if (err_code != SYS_EOF) {
            // End of file is not a real "error", no error message.
            report().error(u"error reading from pipe: %s", SysErrorCodeMessage(err_code));
        }
        return err_code;
    }

#else  // UNIX

    for (;;) {
        const ssize_t insize = ::read(hfd, addr, max_size);
        err_code = TranslateError(errno);
        if (insize == 0) {
            // End of file.
            return SYS_EOF;
        }
        else if (insize > 0) {
            // Normal case, some data were read.
            assert(size_t(insize) <= max_size);
            ret_size = size_t(insize);
            return SYS_SUCCESS;
        }
        else if (abort != nullptr && abort->aborting()) {
            // User-interrupt, end of processing but no error message
            return SYS_CANCELED;
        }
        else if (isNonBlocking() && err_code == SYS_PENDING_IO) {
            // Non-blocking file with no immediately available data to read.
            assert(iosb != nullptr);
            iosb->pending = true;
            return SYS_SUCCESS;
        }
        else if (err_code != EINTR) {
            // Actual error (not an interrupt)
            report().error(u"error reading from pipe: %s", SysErrorCodeMessage(err_code));
            return err_code;
        }
    }

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
    iosb_marker = 0x42534F49; // "BSOI"
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
        const uint32_t marker = *reinterpret_cast<uint32_t*>(ov - overlap_offset + marker_offset);
        if (marker == iosb_marker_value) {
            // Found the expected value for an IOSB marker, we probably are in an IOSB.
            return reinterpret_cast<IOSB*>(ov - overlap_offset);
        }
        else {
            CERR.debug(u"invalid IOSB, marker: 0x%X", marker);
            return nullptr;
        }
    }
    catch (...) {
        // Probably an invalid memory access.
        return nullptr;
    }
}

#endif
