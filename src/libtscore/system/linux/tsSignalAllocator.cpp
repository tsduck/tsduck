//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//
//  Allocate POSIX real-time signal numbers. Linux-specific.
//
//-----------------------------------------------------------------------------

#include "tsSignalAllocator.h"

// Define singleton instance
TS_DEFINE_SINGLETON(ts::SignalAllocator);


//-----------------------------------------------------------------------------
// Default constructor
//-----------------------------------------------------------------------------

ts::SignalAllocator::SignalAllocator() :
    _signals(std::max<size_t>(0, _signal_max - _signal_min + 1), false)
{
}


//-----------------------------------------------------------------------------
// Allocate a new signal number. Return -1 if none available.
//-----------------------------------------------------------------------------

int ts::SignalAllocator::allocate()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t n = 0; n < _signals.size(); ++n) {
        if (!_signals[n]) {
            _signals[n] = true;
            return _signal_min + int(n);
        }
    }
    return -1;
}


//-----------------------------------------------------------------------------
// Release a signal number.
//-----------------------------------------------------------------------------

void ts::SignalAllocator::release(int sig)
{
    if (sig >= _signal_min && sig <= _signal_max) {
        std::lock_guard<std::mutex> lock(_mutex);
        _signals[size_t(sig - _signal_min)] = false;
    }
}
