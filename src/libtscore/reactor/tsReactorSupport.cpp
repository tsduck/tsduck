//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactorSupport.h"

#include "tsBeforeStandardHeaders.h"
#include <sys/stat.h>
#include <sys/epoll.h>
#include "tsAfterStandardHeaders.h"


//----------------------------------------------------------------------------
// Check if a given file descriptor or handle is supported by the reactor.
//----------------------------------------------------------------------------

bool ts::ReactorSupport::SupportedDevice(SysSocketType sock)
{
    if (sock == SYS_SOCKET_INVALID) {
        return false;
    }

#if defined(TS_USE_EPOLL)

    // On Linux, epoll only supports what poll() supports. Get file type.
    struct stat st;
    TS_ZERO(st);
    if (::fstat(sock, &st) < 0) {
        return false; // cannot use that file descriptor for anything
    }

    // Sockets, pipes and FIFO (named pipes) are supported.
    if (S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode)) {
        return true;
    }

    // Regular files and directories are not supported.
    if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
        return false;
    }

    // For other file types, we need to check using a dummy epoll call.
    // We try to "delete" the file descriptor from a temporary epoll instance.
    // If the file type is not supported, EPERM is returned. Otherwise, we expect ENOENT (not registered).
    // This may sound heavy but most cases were already quickly addressed above (sockets, pipes, files).
    const int epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        return false;  // epoll fails, it cannot support anything
    }
    errno = 0;
    const bool supported = ::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, nullptr) == 0 || errno != EPERM;
    ::close(epoll_fd);
    return supported;

#else

    // By default, when not addressed above, consider that all devices are supported in a reactor.
    return true;

#endif
}
