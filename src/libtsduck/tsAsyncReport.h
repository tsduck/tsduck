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
//!
//!  @file
//!  Asynchronous message report.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReportInterface.h"
#include "tsReportHandler.h"
#include "tsMessageQueue.h"
#include "tsNullMutex.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Asynchronous message report.
    //!
    //! This class logs messages asynchronously. Each time a message is logged,
    //! the message is queued into an internal buffer and control returns immediately
    //! to the caller without waiting. The messages are logged later in one single
    //! low-priority thread.
    //!
    //! In case of a huge amount of errors, there is no avalanche effect. If a caller
    //! cannot immediately enqueue a message or if the internal queue of messages is
    //! full, the message is dropped. In other words, reporting messages is guaranteed
    //! to never block, slow down or crash the application. Messages are dropped when
    //! necessary to avoid that kind of problem.
    //!
    //! Messages are displayed on the standard error device by default.
    //!
    class TSDUCKDLL AsyncReport : public ReportInterface, private Thread
    {
    public:
        //!
        //! Constructor.
        //! The default initial report level is Info.
        //! @param [in] verbose If true, set initial report level to Verbose.
        //! @param [in] debug_level If greater than zero, set initial report to that level and ignore @a verbose.
        //! @param [in] time_stamp If true, time stamps are added to all messages.
        //!
        AsyncReport(bool verbose = false, int debug_level = 0, bool time_stamp = false);

        //!
        //! Destructor.
        //!
        virtual ~AsyncReport();

        //!
        //! Set a new ReportHandler.
        //! @param [in] handler Address of a new report handler. When zero, revert to the default
        //! handler (log to standard error).
        //!
        void setMessageHandler(ReportHandler* handler);

        //!
        //! Activate or deactivate time stamps in log messages
        //! @param [in] on If true, time stamps are added to all messages.
        //!
        void setTimeStamp(bool on) { _time_stamp = on; }

        //!
        //! Check if time stamps are added in log messages.
        //! @return True if time stamps are added in log messages.
        //!
        bool getTimeStamp() const { return _time_stamp; }

        //!
        //! Synchronously terminate the report thread.
        //! Automatically performed in destructor.
        //!
        void terminate();

    protected:
        // ReportInterface implementation.
        virtual void writeLog(int severity, const std::string& msg) override;

    private:
        AsyncReport(const AsyncReport&) = delete;
        AsyncReport& operator=(const AsyncReport&) = delete;

        // This hook is invoked in the context of the logging thread.
        virtual void main() override;

        // The application threads send that type of message to the logging thread
        struct LogMessage
        {
            // Members
            bool terminate;  // ask the logging thread to terminate
            int severity;
            std::string message;

            // Constructor:
            LogMessage (bool t, int s, const std::string& m) : terminate (t), severity (s), message (m) {}
        };
        typedef SafePtr <LogMessage, NullMutex> LogMessagePtr;
        typedef MessageQueue <LogMessage, NullMutex> LogMessageQueue;

        // Maximum number of messages in the queue.
        // Must be limited since the logging thread has a low priority.
        // If a high priority thread loops on report, it would exhaust the memory.
        static const size_t MAX_LOG_MESSAGES = 512;

        // Default report handler:
        class DefaultHandler : public ReportHandler
        {
        private:
            const AsyncReport& _report;
        public:
            DefaultHandler(const AsyncReport& report) : _report(report) {}
            virtual void handleMessage(int, const std::string&);
        };

        // Private members:
        LogMessageQueue _log_queue;
        DefaultHandler _default_handler;
        ReportHandler* volatile _handler;
        volatile bool _time_stamp;
        volatile bool _terminated;
    };
}
