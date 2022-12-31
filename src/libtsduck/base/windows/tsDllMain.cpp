//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//
//  TSDuck DLL entry point on Windows.
//
//-----------------------------------------------------------------------------

#include "tsThreadLocalObjects.h"

// This code makes sense only when TSDuck is not compiled as a static library.
#if !defined(TSDUCK_STATIC_LIBRARY)

::BOOL WINAPI DllMain(::HINSTANCE hdll, ::DWORD reason, ::LPVOID reserved)
{
    // Perform actions based on the reason for calling.
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            // Initialization of the DLL in the process, executed once.
            break;

        case DLL_THREAD_ATTACH:
            // Thread initialization, executed for each new thread.
            break;

        case DLL_THREAD_DETACH:
            // Thread termination, executed for each terminating thread.
            // Delete all local objects in the thread. Already done for ts::Thread objects.
            // Added as a precaution if TSDuck code is called in the context of other threads.
            ts::ThreadLocalObjects::Instance()->deleteLocalObjects();
            break;

        case DLL_PROCESS_DETACH:
            // Termination of the DLL in the process, executed once.
            break;
    }
    return true;
}

#endif
