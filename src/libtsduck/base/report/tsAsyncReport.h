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
//!  Asynchronous message report.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsAsyncReportArgs.h"
#include "tsMessageQueue.h"
#include "tsNullMutex.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Asynchronous message report.
    //! @ingroup log
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
    class TSDUCKDLL AsyncReport : public Report, private Thread
    {
        TS_NOCOPY(AsyncReport);
    public:
        //!
        //! Constructor.
        //! The default initial report level is Info.
        //! @param [in] max_severity Set initial level report to that level.
        //! @param [in] args Initial parameters.
        //! @see AsyncReportArgs
        //!
        AsyncReport(int max_severity = Severity::Info, const AsyncReportArgs& args = AsyncReportArgs());

        //!
        //! Destructor.
        //!
        virtual ~AsyncReport() override;

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
        //! Activate or deactivate the synchronous mode
        //! @param [in] on If true, the delivery of messages is synchronous.
        //! No message is dropped, all messages are delivered.
        //!
        void setSynchronous(bool on) { _synchronous = on; }

        //!
        //! Check if synchronous mode is on.
        //! @return True if the delivery of messages is synchronous.
        //!
        bool getSynchronous() const { return _synchronous; }

        //!
        //! Synchronously terminate the report thread.
        //! Automatically performed in destructor.
        //!
        void terminate();

    protected:
        //!
        //! This method is called in the context of the asynchronous logging thread when it starts.
        //! The default implementation does nothing. Subclasses may override it to get notified.
        //!
        virtual void asyncThreadStarted();

        //!
        //! This method is called in the context of the asynchronous logging thread to log a message.
        //! The default implementation prints the message on the standard error.
        //! @param [in] severity Severity level of the message.
        //! @param [in] message The message line to log.
        //!
        virtual void asyncThreadLog(int severity, const UString& message);

        //!
        //! This method is called in the context of the asynchronous logging thread when it completes.
        //! The default implementation does nothing. Subclasses may override it to get notified.
        //!
        virtual void asyncThreadCompleted();

    private:
        // Report implementation.
        virtual void writeLog(int severity, const UString& msg) override;

        // This hook is invoked in the context of the logging thread.
        virtual void main() override;

        // The application threads send that type of message to the logging thread
        struct LogMessage
        {
            LogMessage(bool t, int s, const UString& m) : terminate(t), severity(s), message(m) {}

            bool    terminate;  // ask the logging thread to terminate
            int     severity;
            UString message;
        };
        typedef SafePtr <LogMessage, NullMutex> LogMessagePtr;
        typedef MessageQueue <LogMessage, NullMutex> LogMessageQueue;

        // Private members:
        LogMessageQueue _log_queue;
        volatile bool   _time_stamp;
        volatile bool   _synchronous;
        volatile bool   _terminated;
    };
}
