//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsAsyncReport.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::AsyncReport::MAX_LOG_MESSAGES;
#endif


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::AsyncReport::AsyncReport(int max_severity, bool time_stamp, size_t max_messages, bool synchronous) :
    Report(max_severity),
    Thread(ThreadAttributes().setPriority(ThreadAttributes::GetMinimumPriority())),
    _log_queue(max_messages),
    _default_handler(*this),
    _handler(&_default_handler),
    _time_stamp(time_stamp),
    _synchronous(synchronous),
    _terminated(false)
{
    // Start the logging thread
    start ();
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------

ts::AsyncReport::~AsyncReport()
{
    terminate();
}


//----------------------------------------------------------------------------
// Synchronously terminate the report thread.
//----------------------------------------------------------------------------

void ts::AsyncReport::terminate()
{
    if (!_terminated) {
        // Insert an "end of report" message in the queue.
        // This message will tell the logging thread to terminate.
        _log_queue.forceEnqueue(new LogMessage(true, 0, UString()));

        // Wait for termination of the logging thread
        waitForTermination();
        _terminated = true;
    }
}


//----------------------------------------------------------------------------
// Message logging method.
//----------------------------------------------------------------------------

void ts::AsyncReport::writeLog(int severity, const UString &msg)
{
    if (!_terminated) {
        // Enqueue the message immediately (timeout = 0), drop message on overflow.
        // On the contrary, in synchronous mode, wait infinitely until the message is queued.
        _log_queue.enqueue(new LogMessage(false, severity, msg), _synchronous ? Infinite : 0);
    }
}


//----------------------------------------------------------------------------
// This hook is invoked in the context of the logging thread.
//----------------------------------------------------------------------------

void ts::AsyncReport::main()
{
    LogMessagePtr msg;

    while (_log_queue.dequeue(msg) && !msg->terminate) {

        // Invoke the report handler
        _handler->handleMessage(msg->severity, msg->message);

        // Abort application on fatal error
        if (msg->severity == Severity::Fatal) {
            ::exit(EXIT_FAILURE);
        }
    }

    if (_max_severity >= Severity::Debug) {
        _handler->handleMessage(Severity::Debug, u"Report logging thread terminated");
    }
}


//----------------------------------------------------------------------------
// Set a new ReportHandler
//----------------------------------------------------------------------------

void ts::AsyncReport::setMessageHandler(ReportHandler* h)
{
    _handler = h != nullptr ? h : &_default_handler;
}


//----------------------------------------------------------------------------
// Default report handler
//----------------------------------------------------------------------------

void ts::AsyncReport::DefaultHandler::handleMessage(int severity, const UString& msg)
{
    std::cerr << "* ";
    if (_report._time_stamp) {
        std::cerr << ts::Time::CurrentLocalTime().format(ts::Time::DATE | ts::Time::TIME) << " - ";
    }
    std::cerr << Severity::Header(severity) << msg << std::endl;
}
