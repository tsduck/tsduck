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
//  Allocate POSIX real-time signal numbers. Linux-specific.
//
//-----------------------------------------------------------------------------

#include "tsSignalAllocator.h"

#include "tsBeforeStandardHeaders.h"
#include <signal.h>
#include "tsAfterStandardHeaders.h"

// Define singleton instance
TS_DEFINE_SINGLETON (ts::SignalAllocator);


//-----------------------------------------------------------------------------
// Default constructor
//-----------------------------------------------------------------------------

ts::SignalAllocator::SignalAllocator() :
    _signal_min(SIGRTMIN),
    _signal_max(SIGRTMAX),
    _signal_count(size_t(std::max<int>(0, _signal_max - _signal_min + 1))),
    _mutex(),
    _signals(_signal_count)
{
    for (size_t n = 0; n < _signal_count; n++) {
        _signals[n] = false;
    }
}


//-----------------------------------------------------------------------------
// Allocate a new signal number. Return -1 if none available.
//-----------------------------------------------------------------------------

int ts::SignalAllocator::allocate()
{
    GuardMutex lock (_mutex);
    for (size_t n = 0; n < _signal_count; ++n) {
        if (!_signals[n]) {
            _signals[n] = true;
            return _signal_min + int (n);
        }
    }
    return -1;
}


//-----------------------------------------------------------------------------
// Release a signal number.
//-----------------------------------------------------------------------------

void ts::SignalAllocator::release (int sig)
{
    if (sig >= _signal_min && sig <= _signal_max) {
        GuardMutex lock (_mutex);
        _signals[size_t (sig - _signal_min)] = false;
    }
}
