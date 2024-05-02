//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for event reporting and monitoring
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSeverity.h"
#include "tsUString.h"
#include "tsArgMix.h"

namespace ts {
    //!
    //! Base class for event reporting and monitoring.
    //! @ingroup log
    //!
    //! Maximum severity: Each report instance has an adjustable "maximum severity". All messages
    //! with a higher severity are dropped without reporting. The initial default severity is
    //! @c Info, meaning that @c Verbose and @c Debug messages are dropped by default.
    //!
    class TSDUCKDLL Report
    {
        TS_NOCOPY(Report);
    public:
        //!
        //! Default constructor.
        //! The default initial report level is Info.
        //!
        Report() = default;

        //!
        //! Constructor with initial report level, prefix and delegation.
        //! @param [in] max_severity Initial maximum severity of reported messages.
        //! @param [in] prefix The prefix to prepend to all messages.
        //! @param [in] report New report object to which messages are delegated.
        //!
        Report(int max_severity, const UString& prefix = UString(), Report* report = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~Report();

        //!
        //! Set maximum severity level.
        //! Messages with higher severities are not reported.
        //! @param [in] level Set report to that level.
        //! @param [in] delegated Propagate the severity to delegated reports.
        //!
        void setMaxSeverity(int level, bool delegated = false);

        //!
        //! Raise maximum severity level.
        //! The severity can only be increased (more verbose, more debug), never decreased.
        //! @param [in] level Set report at least to that level.
        //! @param [in] delegated Propagate the severity to delegated reports.
        //!
        void raiseMaxSeverity(int level, bool delegated = false);

        //!
        //! Get maximum severity level.
        //! @return Current maximum debug level.
        //!
        int maxSeverity() const { return _max_severity; }

        //!
        //! Check if errors (or worse) were reported through this object.
        //! Errors which were reported through delegated reports are ignored.
        //! @return True if errors (or worse) were reported through this object.
        //!
        bool gotErrors() const { return _got_errors; }

        //!
        //! Reset the error indicator.
        //! @see gotErrors()
        //!
        void resetErrors() { _got_errors = false; }

        //!
        //! Set the prefix to display before each message.
        //! @param [in] prefix The prefix to prepend to all messages.
        //!
        void setReportPrefix(const UString& prefix) { _prefix = prefix; }

        //!
        //! Get the current prefix to display.
        //! @return The current prefix to display.
        //!
        UString reportPrefix() const { return _prefix; }

        //!
        //! Delegate message logging to another report object.
        //! @param [in] report New report object to which messages are delegated.
        //! Use @a nullptr to remove the delegation and return to normal logging.
        //! @return Previous delegate report, return a null pointer if there was no previous delegate.
        //!
        Report* delegateReport(Report* report);

        //!
        //! Check if debugging is active.
        //! @return True if current reporting level is Debug or higher.
        //!
        bool debug() const { return _max_severity >= Severity::Debug; }

        //!
        //! Check if verbose reporting is active.
        //! @return True if current reporting level is Verbose or higher.
        //!
        bool verbose() const { return _max_severity >= Severity::Verbose; }

        // Legacy functions using initializer lists.
        // The new declarations use variadic templates instead of explicit initializer lists.
        // Calling the new overloaded functions is identical, without the brackets.
        //! @cond nodoxygen
        TS_PUSH_WARNING()
        TS_GCC_NOWARNING(deprecated-declarations)
        TS_MSC_NOWARNING(4996)
        TS_DEPRECATED void log(int severity, const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            if (severity <= _max_severity) {
                log(severity, UString::Format(fmt, args));
            }
        }
        TS_DEPRECATED void log(int severity, const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(severity, fmt.c_str(), args);
        }
        TS_DEPRECATED void fatal(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Fatal, fmt, args);
        }
        TS_DEPRECATED void fatal(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Fatal, fmt, args);
        }
        TS_DEPRECATED void severe(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Severe, fmt, args);
        }
        TS_DEPRECATED void severe(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Severe, fmt, args);
        }
        TS_DEPRECATED void error(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Error, fmt, args);
        }
        TS_DEPRECATED void error(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Error, fmt, args);
        }
        TS_DEPRECATED void warning(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Warning, fmt, args);
        }
        TS_DEPRECATED void warning(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Warning, fmt, args);
        }
        TS_DEPRECATED void info(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Info, fmt, args);
        }
        TS_DEPRECATED void info(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Info, fmt, args);
        }
        TS_DEPRECATED void verbose(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Verbose, fmt, args);
        }
        TS_DEPRECATED void verbose(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Verbose, fmt, args);
        }
        TS_DEPRECATED void debug(const UChar* fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Debug, fmt, args);
        }
        TS_DEPRECATED void debug(const UString& fmt, std::initializer_list<ArgMixIn> args)
        {
            log(Severity::Debug, fmt, args);
        }
        TS_POP_WARNING()
        //! @endcond

        //!
        //! Report a message with an explicit severity.
        //! @param [in] severity Message severity.
        //! @param [in] msg Message line.
        //!
        void log(int severity, const UString& msg);

        //!
        //! Report a message with an explicit severity and a printf-like interface.
        //! @param [in] severity Message severity.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void log(int severity, const UChar* fmt, Args&&... args)
        {
            if (severity <= _max_severity) {
                log(severity, UString::Format(fmt, std::forward<ArgMixIn>(args)...));
            }
        }

        //!
        //! Report a message with an explicit severity and a printf-like interface.
        //! @param [in] severity Message severity.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void log(int severity, const UString& fmt, Args&&... args)
        {
            log(severity, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a fatal error message.
        //! @param [in] msg Message line.
        //!
        void fatal(const UChar* msg)
        {
            if (Severity::Fatal <= _max_severity) {
                log(Severity::Fatal, UString(msg));
            }
        }

        //!
        //! Report a fatal error message.
        //! @param [in] msg Message line.
        //!
        void fatal(const UString& msg)
        {
            log(Severity::Fatal, msg);
        }

        //!
        //! Report a fatal error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void fatal(const UChar* fmt, Args&&... args)
        {
            log(Severity::Fatal, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a fatal error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void fatal(const UString& fmt, Args&&... args)
        {
            log(Severity::Fatal, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a severe error message.
        //! @param [in] msg Message line.
        //!
        void severe(const UChar* msg)
        {
            if (Severity::Severe <= _max_severity) {
                log(Severity::Severe, UString(msg));
            }
        }

        //!
        //! Report a severe error message.
        //! @param [in] msg Message line.
        //!
        void severe(const UString& msg)
        {
            log(Severity::Severe, msg);
        }

        //!
        //! Report a severe error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void severe(const UChar* fmt, Args&&... args)
        {
            log(Severity::Severe, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a severe error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void severe(const UString& fmt, Args&&... args)
        {
            log(Severity::Severe, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report an error message.
        //! @param [in] msg Message line.
        //!
        void error(const UChar* msg)
        {
            if (Severity::Error <= _max_severity) {
                log(Severity::Error, UString(msg));
            }
        }

        //!
        //! Report an error message.
        //! @param [in] msg Message line.
        //!
        void error(const UString& msg)
        {
            log(Severity::Error, msg);
        }

        //!
        //! Report an error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void error(const UChar* fmt, Args&&... args)
        {
            log(Severity::Error, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report an error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void error(const UString& fmt, Args&&... args)
        {
            log(Severity::Error, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a warning error message.
        //! @param [in] msg Message line.
        //!
        void warning(const UChar* msg)
        {
            if (Severity::Warning <= _max_severity) {
                log(Severity::Warning, UString(msg));
            }
        }

        //!
        //! Report a warning error message.
        //! @param [in] msg Message line.
        //!
        void warning(const UString& msg)
        {
            log(Severity::Warning, msg);
        }

        //!
        //! Report a warning message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void warning(const UChar* fmt, Args&&... args)
        {
            log(Severity::Warning, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a warning message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void warning(const UString& fmt, Args&&... args)
        {
            log(Severity::Warning, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report an informational message.
        //! @param [in] msg Message line.
        //!
        void info(const UChar* msg)
        {
            if (Severity::Info <= _max_severity) {
                log(Severity::Info, UString(msg));
            }
        }

        //!
        //! Report an informational message.
        //! @param [in] msg Message line.
        //!
        void info(const UString& msg)
        {
            log(Severity::Info, msg);
        }

        //!
        //! Report an informational message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void info(const UChar* fmt, Args&&... args)
        {
            log(Severity::Info, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report an informational message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void info(const UString& fmt, Args&&... args)
        {
            log(Severity::Info, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a verbose message.
        //! @param [in] msg Message line.
        //!
        void verbose(const UChar* msg)
        {
            if (Severity::Verbose <= _max_severity) {
                log(Severity::Verbose, UString(msg));
            }
        }

        //!
        //! Report a verbose message.
        //! @param [in] msg Message line.
        //!
        void verbose(const UString& msg)
        {
            log(Severity::Verbose, msg);
        }

        //!
        //! Report a verbose message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void verbose(const UChar* fmt, Args&&... args)
        {
            log(Severity::Verbose, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a verbose message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void verbose(const UString& fmt, Args&&... args)
        {
            log(Severity::Verbose, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a debug message.
        //! @param [in] msg Message line.
        //!
        void debug(const UChar* msg)
        {
            if (Severity::Debug <= _max_severity) {
                log(Severity::Debug, UString(msg));
            }
        }

        //!
        //! Report a debug message.
        //! @param [in] msg Message line.
        //!
        void debug(const UString& msg)
        {
            log(Severity::Debug, msg);
        }

        //!
        //! Report a debug message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void debug(const UChar* fmt, Args&&... args)
        {
            log(Severity::Debug, fmt, std::forward<ArgMixIn>(args)...);
        }

        //!
        //! Report a debug message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        template <class... Args>
        void debug(const UString& fmt, Args&&... args)
        {
            log(Severity::Debug, fmt.c_str(), std::forward<ArgMixIn>(args)...);
        }

    protected:
        //!
        //! Actual message reporting method.
        //! The method is called only when a message passed the severity filter.
        //! It is not necessary to recheck the maximum severity inside the method.
        //! By default, does nothing.
        //!
        //! @param [in] severity Message severity.
        //! @param [in] msg Message text.
        //!
        virtual void writeLog(int severity, const UString& msg);

    private:
        bool     volatile _got_errors = false;
        int      volatile _max_severity = Severity::Info;
        UString           _prefix {};
        // Delegation and thread synchronization:
        // - Establishing a delegation: _mutex shall be held in this object first, then in delegate.
        // - Using the delegate to log a message or set severity: no lock, just read the volatile _delegate
        //   once and use the copy. If unlinked in the meantime, the message will be logged to the previous
        //   delegate, which is not incorrect since logging and delegating simultaneously occured.
        // - Problems occur when we log to the delegate at the same time the delegate is destructed,
        //   because there is no lock during the logging functions. We accept the race condition since
        //   getting the lock each time we log a message whould be too costly. To avoid this race condition,
        //   never delegate to a report which may be destructed before this object.
        std::mutex        _mutex {};
        Report*  volatile _delegate = nullptr;
        std::set<Report*> _delegated {};   // list of other instances which delegate to this
    };
}
