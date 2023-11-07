//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFatal.h"


//----------------------------------------------------------------------------
// Handle a fatal error.
// An emergency message is output and the application is terminated.
//----------------------------------------------------------------------------

void ts::FatalError(const char* message, size_t length)
{
#if defined(TS_WINDOWS)
    ::DWORD written;
    ::WriteFile(::GetStdHandle(STD_ERROR_HANDLE), message, ::DWORD(length), &written, 0);
#else
    TS_UNUSED ssize_t n = ::write(STDERR_FILENO, message, length);
#endif
    std::exit(EXIT_FAILURE);
}


//----------------------------------------------------------------------------
// Out of virtual memory, very dangerous situation, really can't
// recover from that, need to abort immediately. We can't even
// try to use some sophisticated C++ library since it may require
// memory allocation.
//----------------------------------------------------------------------------

void ts::FatalMemoryAllocation()
{
    static const char err[] = "\n\n*** Fatal virtual memory allocation failure, aborting...\n\n";
    static constexpr size_t err_size = sizeof(err) - 1;
    FatalError(err, err_size);
}
