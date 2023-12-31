//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
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
