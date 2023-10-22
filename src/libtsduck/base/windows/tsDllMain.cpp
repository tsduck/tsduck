//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
