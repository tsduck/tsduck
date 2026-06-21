//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    ::DWORD written = 0;
    ::WriteFile(::GetStdHandle(STD_ERROR_HANDLE), message, ::DWORD(length), &written, nullptr);
#else
    [[maybe_unused]] ssize_t n = ::write(STDERR_FILENO, message, length);
#endif
    std::exit(EXIT_FAILURE);
}
