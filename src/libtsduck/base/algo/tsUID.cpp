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
//
//  Implementation:
//  To ensure a reasonable level of uniqueness, a UID is composed of:
//    - 24 bits: LSB of process id
//    - 24 bits: LSB of initial UTC time (milliseconds)
//    - 16 bits: sequential index
//  The UID is incremented each time a new value is requested.
//  The index does not wrap, it overflows on time field.
//
//----------------------------------------------------------------------------

#include "tsUID.h"
#include "tsGuardMutex.h"
#include "tsSysUtils.h"
#include "tsTime.h"

// Define singleton instance
TS_DEFINE_SINGLETON (ts::UID);


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::UID::UID() :
    _mutex(),
    _next_uid((uint64_t(CurrentProcessId()) << 40) | ((uint64_t(Time::CurrentUTC() - Time::Epoch) & 0x00FFFFFF) << 16))
{
}


//----------------------------------------------------------------------------
// Generate a new UID
//----------------------------------------------------------------------------

uint64_t ts::UID::newUID()
{
    GuardMutex lock(_mutex);
    return _next_uid++;
}
