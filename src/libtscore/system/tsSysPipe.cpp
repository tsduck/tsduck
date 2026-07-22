//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSysPipe.h"
#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
#endif


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::SysPipe::~SysPipe()
{
    close(true);
}


//----------------------------------------------------------------------------
// Create the pipe and open the two file descriptors.
//----------------------------------------------------------------------------

bool ts::SysPipe::create(Flags flags, size_t buffer_size)
{
    if (_read_hfd != SYS_HANDLE_INVALID || _write_hfd != SYS_HANDLE_INVALID) {
        report().error(u"pipe is already open");
        return false;
    }

#if defined(TS_WINDOWS)

    ::DWORD bufsize = buffer_size == 0 ? 0 : ::DWORD(std::max<size_t>(32768, buffer_size));
    ::SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = true; // inherited in child process

    if ((flags & (READ_ASYNC | WRITE_ASYNC)) != 0) {
        // At least one end of the pipe uses overlapped I/O. This is allowed on named pipes only.
        // Create a "unique" pipe name. Note that named pipes disappear when the last handle is closed.
        // Therefore, we don't need to care about deleting the named pipe.
        static std::atomic_uint64_t sequence {0};
        UString pipe_name;
        pipe_name.format(u"\\\\.\\pipe\\ts.%08X.%08X", ::GetCurrentProcessId(), ++sequence);

        // The read end of the pipe is used to create the named pipe. The write end is opened on the file name.
        const ::DWORD read_flags = PIPE_ACCESS_INBOUND | FILE_FLAG_FIRST_PIPE_INSTANCE | ((flags & READ_ASYNC) != 0 ? FILE_FLAG_OVERLAPPED : 0);
        const ::DWORD write_flags = FILE_ATTRIBUTE_NORMAL | ((flags & WRITE_ASYNC) != 0 ? FILE_FLAG_OVERLAPPED : 0);

        // Create the named pipe. Get the read handle.
        _read_hfd = ::CreateNamedPipeW(pipe_name.wc_str(), read_flags, PIPE_TYPE_BYTE | PIPE_WAIT, 1, bufsize, bufsize, 0, &sa);
        if (!WinHandleValid(_read_hfd)) {
            report().error(u"error creating named pipe %s: %s", pipe_name, SysErrorCodeMessage());
            return false;
        }

        // Get a write handle to this named pipe.
        _write_hfd = ::CreateFileW(pipe_name.wc_str(), GENERIC_WRITE, 0, &sa, OPEN_EXISTING, write_flags, nullptr);
        if (!WinHandleValid(_write_hfd)) {
            report().error(u"error opening named pipe %s: %s", pipe_name, SysErrorCodeMessage());
            close(true);
            return false;
        }
    }
    else {
        // No asynchronous I/O required, use an anonymous pipe.
        if (::CreatePipe(&_read_hfd, &_write_hfd, &sa, bufsize) == 0) {
            report().error(u"error creating pipe: %s", SysErrorCodeMessage());
            return false;
        }
    }

    // CreatePipe can only inherit none or both handles. So, we used "inherit" for both and now we close uninherited ends.
    // Named pipes do not have the same limitation but we use the same method for consistency.
    if ((flags & READ_INHERIT) == 0) {
        ::SetHandleInformation(_read_hfd, HANDLE_FLAG_INHERIT, 0);
    }
    if ((flags & WRITE_INHERIT) == 0) {
        ::SetHandleInformation(_write_hfd, HANDLE_FLAG_INHERIT, 0);
    }

#else

    // Create the pipe.
    int hfd[PIPE_COUNT];
    if (::pipe(hfd) < 0) {
        report().error(u"error creating pipe: %s", SysErrorCodeMessage());
        return false;
    }
    _read_hfd = hfd[PIPE_READFD];
    _write_hfd = hfd[PIPE_WRITEFD];

    // Set asynchronous and close-on-exec flags.
    // Warning: F_SETFL and F_SETFD are distinct fcntl commands.
    bool success = true;
    if ((flags & READ_ASYNC) != 0) {
        success = ::fcntl(_read_hfd, F_SETFL, O_NONBLOCK) != -1;
    }
    if (success && (flags & WRITE_ASYNC) != 0) {
        success = ::fcntl(_write_hfd, F_SETFL, O_NONBLOCK) != -1;
    }
    if (success && (flags & READ_CLOEXEC) != 0) {
        success = ::fcntl(_read_hfd, F_SETFD, FD_CLOEXEC) != -1;
    }
    if (success && (flags & WRITE_CLOEXEC) != 0) {
        success = ::fcntl(_write_hfd, F_SETFD, FD_CLOEXEC) != -1;
    }
    if (!success) {
        report().error(u"error setting pipe flags: %s", SysErrorCodeMessage());
        close(true);
        return false;
    }

#endif

    return true;
}


//----------------------------------------------------------------------------
// Close the pipe file descriptors which are not "fetched".
//----------------------------------------------------------------------------

bool ts::SysPipe::close(bool silent)
{
    bool success = true;
    int err = SYS_SUCCESS;
    if (_read_hfd != SYS_HANDLE_INVALID && !SysSuccess(err = SysCloseHandle(_read_hfd))) {
        report().log(SilentLevel(silent), u"error closing read end of the pipe: %s", SysErrorCodeMessage(err));
        success = false;
    }
    if (_write_hfd != SYS_HANDLE_INVALID && !SysSuccess(err = SysCloseHandle(_write_hfd))) {
        report().log(SilentLevel(silent), u"error closing write end of the pipe: %s", SysErrorCodeMessage(err));
        success = false;
    }
    _read_hfd = _write_hfd = SYS_HANDLE_INVALID;
    return success;
}


//----------------------------------------------------------------------------
// "Fetch" the read file descriptor of the pipe.
//----------------------------------------------------------------------------

ts::SysHandleType ts::SysPipe::fetchRead()
{
    const SysHandleType fd = _read_hfd;
    if (fd == SYS_HANDLE_INVALID) {
        report().error(u"error fetching read end of the pipe, not defined");
    }
    else {
        _read_hfd = SYS_HANDLE_INVALID;
    }
    return fd;
}


//----------------------------------------------------------------------------
// "Fetch" the write file descriptor of the pipe.
//----------------------------------------------------------------------------

ts::SysHandleType ts::SysPipe::fetchWrite()
{
    const SysHandleType fd = _write_hfd;
    if (fd == SYS_HANDLE_INVALID) {
        report().error(u"error fetching write end of the pipe, not defined");
    }
    else {
        _write_hfd = SYS_HANDLE_INVALID;
    }
    return fd;
}
