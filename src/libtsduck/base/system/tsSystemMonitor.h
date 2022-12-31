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
//!
//!  @file
//!  Monitoring thread for system resources used by the application.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
#include "tsMutex.h"
#include "tsCondition.h"
#include "tsTime.h"
#include "tsReport.h"
#include "tsxml.h"

namespace ts {
    //!
    //! Monitoring thread for system resources used by the application.
    //! @ingroup system
    //!
    //! This class starts an internal thread which periodically wakes up
    //! and reports the usage of system resources by the current process
    //! (virtual memory usage, CPU load). In addition to precise usage
    //! values, it also displays an analysis of the virtual memory usage
    //! (such as stable or leaking).
    //!
    //! The reporting interval is changing over time, very fast at the
    //! start of the application, then slower and slower:
    //!
    //! - Up to start + 2 mn, log every 10 seconds
    //! - Up to start + 10 mn, log every minute
    //! - Up to start + 20 mn, log every 2 minutes
    //! - Up to start + 1 hour, log every 5 minutes
    //! - After start + 1 hour, log every 30 minutes
    //!
    //! This class derives from Thread. The methods start() and waitForTermination() are
    //! inherited. The destructor stops the thread and synchronously waits for its termination.
    //! The method stop() can be used to stop the thread.
    //!
    class TSDUCKDLL SystemMonitor: public Thread
    {
        TS_NOBUILD_NOCOPY(SystemMonitor);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report log data.
        //! @param [in] config Name of the monitoring configuration file, if different from the default.
        //!
        SystemMonitor(Report& report, const UString& config = UString());

        //!
        //! Destructor.
        //! The monitor thread is stopped.
        //!
        virtual ~SystemMonitor() override;

        //!
        //! Stop the monitor thread.
        //! The monitor thread is requested to stop. This method returns immediately,
        //! use waitForTermination() to synchronously wait for its termination.
        //!
        void stop();

    private:
        // Description of a monitoring configuration, during one period.
        class Config
        {
        public:
            Config();               // Constructor.
            bool    log_messages;   // Log monitoring messages.
            bool    stable_memory;  // If true, raise an alarm when the virtual memory increases.
            int     max_cpu;        // Maximum allowed CPU percentage.
            UString alarm_command;  // Shell command to run on alarm.
        };

        // Description of a monitoring period.
        class Period: public Config
        {
        public:
            Period();               // Constructor.
            MilliSecond duration;   // Period duration in milliseconds.
            MilliSecond interval;   // Monitoring interval in that period.
        };
        typedef std::list<Period> PeriodList;

        // Private members
        Report&    _report;
        UString    _config_file;  // XML configuration file name.
        PeriodList _periods;      // List of monitoring periods.
        Mutex      _mutex;
        Condition  _wake_up;      // Accessed under mutex.
        bool       _terminate;    // Accessed under mutex.

        // Inherited from Thread
        virtual void main() override;

        // Prefix strings for all monitor messages
        static UString MonPrefix(const ts::Time& date);

        // Laad the monitoring configuration file.
        bool loadConfigurationFile(const UString& config);
        bool loadConfig(Config&, const xml::Element*, const Config*);
    };
}
