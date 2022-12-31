//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
    ::exit(EXIT_FAILURE);
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
    static const size_t err_size = sizeof(err) - 1;
    FatalError(err, err_size);
}
