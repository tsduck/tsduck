//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Monitoring thread for system resources used by the application.
//
//----------------------------------------------------------------------------

#include "tsSystemMonitor.h"
#include "tsGuardCondition.h"
#include "tsIntegerUtils.h"
#include "tsSysUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsTime.h"
TSDUCK_SOURCE;

// Stack size for the monitor thread
#define MONITOR_STACK_SIZE (64 * 1024)


//----------------------------------------------------------------------------
// Monitoring time profile: fast at the beginning, then slower and slower
//----------------------------------------------------------------------------

#define MN (60 * ts::MilliSecPerSec) // 1 minute in milli-seconds

namespace {

    struct TimeProfile
    {
        ts::MilliSecond up_to;      // up to this time after start ...
        ts::MilliSecond interval;   // ... log every interval
    };

    const TimeProfile monitor_time_profile[] = {
        { 2 * MN,  MN / 6},  // up to start + 2 mn, log every 10 seconds
        {10 * MN,  1 * MN},  // up to start + 10 mn, log every minute
        {20 * MN,  2 * MN},  // up to start + 20 mn, log every 2 minutes
        {60 * MN,  5 * MN},  // up to start + 1 hour, log every 5 minutes
        {      0, 30 * MN},  // after start + 1 hour, log every 30 minutes
    };
}

#undef MN


//----------------------------------------------------------------------------
// Prefix strings for all monitor messages (for filtering purpose)
//----------------------------------------------------------------------------

namespace {
    std::string MonPrefix(const ts::Time& date)
    {
        //@@@
        return "[MON] " + date.format(ts::Time::DATE | ts::Time::HOUR | ts::Time::MINUTE).toUTF8() + ", ";
    }
}


//----------------------------------------------------------------------------
//  Format a size using MB, kB or B as appropriate.
//----------------------------------------------------------------------------

namespace {
    std::string SizeString (ptrdiff_t value, bool force_sign)
    {
        if (value < 8 * 1024) { // less than 8 kB => use bytes
            return ts::Decimal (value, 0, true, ",", force_sign) + " B";
        }
        else if (value < 8 * 1024 * 1024) { // between 8 kB and 8 MB => use kB
            return ts::Decimal (value / 1024, 0, true, ",", force_sign) + " kB";
        }
        else { // more than 8 MB => use MB
            return ts::Decimal (value / (1024 * 1024), 0, true, ",", force_sign) + " MB";
        }
    }
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SystemMonitor::SystemMonitor (Report* report) :
    Thread (ThreadAttributes().setPriority(ThreadAttributes::GetMinimumPriority()).setStackSize(MONITOR_STACK_SIZE)),
    _report (report),
    _mutex (),
    _wake_up (),
    _terminate (false)
{
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::SystemMonitor::~SystemMonitor ()
{
    // Signal that the thread shall terminate

    {
        GuardCondition lock (_mutex, _wake_up);
        _terminate = true;
        lock.signal();
    }
    waitForTermination();
}


//----------------------------------------------------------------------------
// Thread main code. Inherited from Thread
//----------------------------------------------------------------------------

void ts::SystemMonitor::main()
{
    const TimeProfile* time_profile = monitor_time_profile;
    const Time start_time (Time::CurrentLocalTime());
    ProcessMetrics start_metrics;                // Initial system metrics
    GetProcessMetrics (start_metrics);           // Get initial system metrics
    ProcessMetrics last_metrics (start_metrics); // Last system metrics
    Time last_time (start_time);                 // Last report time
    Time vsize_uptime (start_time);              // Time of last vsize increase
    size_t vsize_max (start_metrics.vmem_size);  // Maximum vsize

    _report->log (Severity::Info, MonPrefix (last_time) + "resource monitoring started");

    // Loop on monitoring intervals

    for (;;) {

        // Compute next time profile

        while (time_profile->up_to != 0 && last_time > start_time + time_profile->up_to) {
            time_profile++;
        }

        // Wait until due time or termination request

        {
            GuardCondition lock (_mutex, _wake_up);
            if (!_terminate) {
                lock.waitCondition (time_profile->interval);
            }
            if (_terminate) {
                break;
            }
        }

        // Get current process metrics

        Time current_time (Time::CurrentLocalTime());
        ProcessMetrics metrics;
        GetProcessMetrics (metrics);

        // Format virtual memory size status

        std::string message (MonPrefix (current_time) + "VM:" + SizeString (metrics.vmem_size, false));

        if (metrics.vmem_size != last_metrics.vmem_size) {
            // Virtual memory has changed
            message += " (" + SizeString (ptrdiff_t (metrics.vmem_size) - ptrdiff_t (last_metrics.vmem_size), true) + ")";
        }
        else {
            // VM stable since last time. Check if temporarily stable or
            // safely stable. If no increase during last 95% of the running
            // time, then we are stable.
            message += (current_time - vsize_uptime) > (95 * (current_time - start_time)) / 100 ? " (stable)" : " (leaking)";
        }

        if (metrics.vmem_size > vsize_max) {
            // Virtual memory has increased
            vsize_max = metrics.vmem_size;
            vsize_uptime = current_time;
        }

        // Format CPU load.

        message += ", CPU:";
        message += PercentageString(metrics.cpu_time - last_metrics.cpu_time, current_time - last_time);
        message += " (average:";
        message += PercentageString(metrics.cpu_time - start_metrics.cpu_time, current_time - start_time);
        message += ")";

        // Display monitoring status

        _report->log (Severity::Info, message);

        last_time = current_time;
        last_metrics = metrics;
    }

    _report->log (Severity::Info, MonPrefix (Time::CurrentLocalTime()) + "resource monitoring terminated");
}
