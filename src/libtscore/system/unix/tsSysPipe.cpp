//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSysPipe.h"


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

bool ts::SysPipe::create()
{
    if (_fd[PIPE_READFD] >= 0 || _fd[PIPE_WRITEFD] >= 0) {
        report().error(u"pipe is already open");
        return false;
    }
    else if (::pipe(_fd) < 0) {
        report().error(u"error creating pipe: %s", SysErrorCodeMessage());
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Close the pipe file descriptors which are not "fetched".
//----------------------------------------------------------------------------

bool ts::SysPipe::close(bool silent)
{
    bool success = true;
    if (_fd[PIPE_READFD] >= 0 && ::close(_fd[PIPE_READFD]) < 0) {
        report().log(SilentLevel(silent), u"error closing read end of the pipe");
        success = false;
    }
    if (_fd[PIPE_WRITEFD] >= 0 && ::close(_fd[PIPE_WRITEFD]) < 0) {
        report().log(SilentLevel(silent), u"error closing write end of the pipe");
        success = false;
    }
    _fd[PIPE_READFD] = _fd[PIPE_WRITEFD] = SYS_HANDLE_INVALID;
    return success;
}


//----------------------------------------------------------------------------
// "Fetch" the read file descriptor of the pipe.
//----------------------------------------------------------------------------

ts::SysHandleType ts::SysPipe::fetchRead()
{
    SysHandleType fd = _fd[PIPE_READFD];
    if (fd < 0) {
        report().error(u"error fetching read end of the pipe, not defined");
        fd = SYS_HANDLE_INVALID;
    }
    _fd[PIPE_READFD] = SYS_HANDLE_INVALID;
    return fd;
}


//----------------------------------------------------------------------------
// "Fetch" the write file descriptor of the pipe.
//----------------------------------------------------------------------------

ts::SysHandleType ts::SysPipe::fetchWrite()
{
    SysHandleType fd = _fd[PIPE_WRITEFD];
    if (fd < 0) {
        report().error(u"error fetching write end of the pipe, not defined");
        fd = SYS_HANDLE_INVALID;
    }
    _fd[PIPE_WRITEFD] = SYS_HANDLE_INVALID;
    return fd;
}
