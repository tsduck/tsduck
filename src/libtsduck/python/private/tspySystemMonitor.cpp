//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: encapsulates SystemMonitor objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsSystemMonitor.h"
#include "tsCerrReport.h"


//-----------------------------------------------------------------------------
// Interface to SystemMonitor.
//-----------------------------------------------------------------------------

TSDUCKPY void* tspyNewSystemMonitor(void* report, const uint8_t* config, size_t config_size)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    return new ts::SystemMonitor(rep == nullptr ? CERR : *rep, ts::py::ToString(config, config_size));
}

TSDUCKPY void tspyDeleteSystemMonitor(void* pymon)
{
    delete reinterpret_cast<ts::SystemMonitor*>(pymon);
}

TSDUCKPY void tspyStartSystemMonitor(void* pymon)
{
    ts::SystemMonitor* mon = reinterpret_cast<ts::SystemMonitor*>(pymon);
    if (mon != nullptr) {
        mon->start();
    }
}

TSDUCKPY void tspyStopSystemMonitor(void* pymon)
{
    ts::SystemMonitor* mon = reinterpret_cast<ts::SystemMonitor*>(pymon);
    if (mon != nullptr) {
        mon->stop();
    }
}

TSDUCKPY void tspyWaitSystemMonitor(void* pymon)
{
    ts::SystemMonitor* mon = reinterpret_cast<ts::SystemMonitor*>(pymon);
    if (mon != nullptr) {
        mon->waitForTermination();
    }
}
