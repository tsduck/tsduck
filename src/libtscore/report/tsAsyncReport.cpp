//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAsyncReport.h"
#include "tsTime.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

// The logging thread os created with the lowest priority.
ts::AsyncReport::LoggingThread::LoggingThread(AsyncReport& parent) :
    Thread(ThreadAttributes().setPriority(ThreadAttributes::GetMinimumPriority())),
    _parent(parent)
{
}

ts::AsyncReport::AsyncReport(int max_severity, const AsyncReportArgs& args) :
    Report(max_severity),
    _log_queue(args.log_msg_count),
    _time_stamp(args.timed_log),
    _synchronous(args.sync_log)
{
    // Start the logging thread
    _thread.start();
}

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
        auto ptr = std::make_shared<LogMessage>(true, 0, UString());
        _log_queue.forceEnqueue(ptr);

        // Wait for termination of the logging thread
        _thread.waitForTermination();
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
        auto ptr = std::make_shared<LogMessage>(false, severity, msg);
        if (_synchronous) {
            // Synchronous mode, wait infinitely until the message is queued.
            _log_queue.enqueue(ptr);
        }
        else {
            // Enqueue the message immediately (timeout = 0), drop message on overflow.
            _log_queue.enqueue(ptr, cn::milliseconds::zero());
        }
    }
}


//----------------------------------------------------------------------------
// This hook is invoked in the context of the logging thread.
//----------------------------------------------------------------------------

void ts::AsyncReport::LoggingThread::main()
{
    LogMessagePtr msg;

    // There is a trick here to avoid a race condition on the vtable of subclasses.
    // The thread is started during AsyncReport constructor. Therefore, the thread may start before
    // the execution of the body of the subclass constructor, meaning before the vtable of the subclass
    // is updated. Calling the virtual method asyncThreadStarted() right now is non-deterministic: it
    // could either call AsyncReport::asyncThreadStarted() or the subclass asyncThreadStarted(). To avoid
    // this race condition, we delay the call when the first message is received. The idea is to guarantee
    // that the subclass asyncThreadStarted() is called before its first asyncThreadLog().
    bool first_time = true;

    for (;;) {
        // Dequeue next message (infinite wait).
        _parent._log_queue.dequeue(msg);

        // Notify subclasses (if any) of thread start, after first message (see above).
        // Do that even if the first message is a termination message to make sure that
        // asyncThreadStarted() was called before the final asyncThreadCompleted().
        if (first_time) {
            first_time = false;
            _parent.asyncThreadStarted();
        }

        // Exit when received a termination message.
        if (msg->terminate) {
            break;
        }

        // Notify subclass of message (or log it on standard error).
        _parent.asyncThreadLog(msg->severity, msg->message);

        // Abort application on fatal error
        if (msg->severity == Severity::Fatal) {
            std::exit(EXIT_FAILURE);
        }
    }

    if (_parent.maxSeverity() >= Severity::Debug) {
        _parent.asyncThreadLog(Severity::Debug, u"Report logging thread terminated");
    }

    // Notify subclasses (if any) of thread completion.
    _parent.asyncThreadCompleted();
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
    std::cerr << Severity::AddHeader(severity, message) << std::endl;
}

void ts::AsyncReport::asyncThreadCompleted()
{
    // The default implementation does nothing.
}
