//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUID.h"
#include "tsTime.h"

#if defined(TS_WINDOWS)
    #define PROCESS_ID ::GetCurrentProcessId()
#else
    #define PROCESS_ID ::getpid()
#endif


//----------------------------------------------------------------------------
// Generate a new 64-bit UID, unique integer.
//----------------------------------------------------------------------------

uint64_t ts::UID()
{
    // Implementation:
    // To ensure a reasonable level of uniqueness, a UID is composed of:
    //   - 24 bits: LSB of process id
    //   - 24 bits: LSB of initial UTC time (milliseconds)
    //   - 16 bits: sequential index
    // The UID is incremented each time a new value is requested.
    // The index does not wrap, it overflows on time field.

    // Thread-safe init-safe static data pattern:
    static std::atomic<uint64_t> next_uid = (uint64_t(PROCESS_ID) << 40) | ((uint64_t((Time::CurrentUTC() - Time::Epoch).count()) & 0x00FFFFFF) << 16);

    // Atomic operation on each call.
    return next_uid++;
}
