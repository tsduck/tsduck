//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Monitoring thread for system resources used by the application.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
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
    class TSCOREDLL SystemMonitor: public Thread
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
            Config() = default;
            bool    log_messages = false;   // Log monitoring messages.
            bool    stable_memory = false;  // If true, raise an alarm when the virtual memory increases.
            int     max_cpu = 0;            // Maximum allowed CPU percentage.
            UString alarm_command {};       // Shell command to run on alarm.
        };

        // Description of a monitoring period.
        class Period: public Config
        {
        public:
            Period() = default;
            cn::milliseconds duration {};   // Period duration in milliseconds.
            cn::milliseconds interval {};   // Monitoring interval in that period.
        };
        using PeriodList = std::list<Period>;

        // Private members
        Report&    _report;
        UString    _config_file {};          // XML configuration file name.
        PeriodList _periods {};              // List of monitoring periods.
        std::mutex _mutex {};                // Protect subsequent fields.
        std::condition_variable _wake_up {}; // Accessed under mutex.
        bool       _terminate = false;       // Accessed under mutex.

        // Inherited from Thread
        virtual void main() override;

        // Prefix strings for all monitor messages
        static UString MonPrefix(const ts::Time& date);

        // Laad the monitoring configuration file.
        bool loadConfigurationFile(const UString& config);
        bool loadConfig(Config&, const xml::Element*, const Config*);
    };
}
