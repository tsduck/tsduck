//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAsyncReport.h"


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::AsyncReport::AsyncReport(int max_severity, const AsyncReportArgs& args) :
    Report(max_severity),
    Thread(ThreadAttributes().setPriority(ThreadAttributes::GetMinimumPriority())),
    _log_queue(args.log_msg_count),
    _time_stamp(args.timed_log),
    _synchronous(args.sync_log)
{
    // Start the logging thread
    start();
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
#if defined(TS_WINDOWS) && defined(TS_DEBUG_LOG)
    // On Windows, when TS_DEBUG_LOG is set, also send all messages to the debugger console.
    // If the environment variable TS_DEBUG_LOG is set during compilation, then the macro
    // TS_DEBUG_LOG is automatically defined and the debug logging is active.
    UString msgNewLine(msg);
    msgNewLine += u"\n";
    ::OutputDebugStringW(msgNewLine.wc_str());
#endif

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

    // Notify subclasses (if any) of thread start.
    asyncThreadStarted();

    while (_log_queue.dequeue(msg) && !msg->terminate) {

        asyncThreadLog(msg->severity, msg->message);

        // Abort application on fatal error
        if (msg->severity == Severity::Fatal) {
            ::exit(EXIT_FAILURE);
        }
    }

    if (_max_severity >= Severity::Debug) {
        asyncThreadLog(Severity::Debug, u"Report logging thread terminated");
    }

    // Notify subclasses (if any) of thread completion.
    asyncThreadCompleted();
}


//----------------------------------------------------------------------------
// Asynchronous logging thread interface.
//----------------------------------------------------------------------------

void ts::AsyncReport::asyncThreadStarted()
{
    // The default implementation does nothing.
}

void ts::AsyncReport::asyncThreadLog(int severity, const UString& message)
{
    // The default implementation logs on stderr.
    std::cerr << "* ";
    if (_time_stamp) {
        std::cerr << ts::Time::CurrentLocalTime().format(ts::Time::DATETIME) << " - ";
    }
    std::cerr << Severity::Header(severity) << message << std::endl;
}

void ts::AsyncReport::asyncThreadCompleted()
{
    // The default implementation does nothing.
}
