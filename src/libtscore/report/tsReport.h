//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    //! Base class for message reporting and monitoring.
    //! @ingroup log
    //!
    //! Maximum severity: Each Report instance has an adjustable "maximum severity". All messages
    //! with a higher severity are dropped without reporting. The initial default severity is
    //! @c Info, meaning that @c Verbose and @c Debug messages are dropped by default.
    //!
    //! Report delegation: A Report can delegate its message reporting to another Report. Each Report
    //! has at most one delegate and several delagators (other Reports which delegate to this object).
    //! Therefore, there is a tree of Reports which ultimately ends to one Report which does the actual
    //! message logging. All Reports in that tree share the same maximum severity. When the maximum
    //! severity is changed in one Report, it is updated in all Reports in the tree.
    //!
    //! Delegation and thread synchronization: We tried to find the right balance between performances
    //! and synchronization.
    //! - We assume that messages are extremely frequently emitted and should be logged without locking.
    //!   Accesses to the current maximum severity and to the delegate are unchecked are done using local
    //!   capies in this object. Problems occur when we log to the delegate at the same time the delegate
    //!   is destructed, because there is no lock during the logging functions. We accept the race condition
    //!   since getting the lock each time we log a message whould be too costly. To avoid this race condition,
    //!   never delegate to a report which may be destructed before this object.
    //! - Setting or removing delegation is rare and needs synchronization. Because modifications
    //!   can be done upward or downward, it is difficult to find a fine-grained hierarchy of locks
    //!   without risking a deadlock. Similarly, it is difficult to locate one mutex per tree of Reports
    //!   because trees can be split (when a delegation is removed) or merged (when a delegation is
    //!   created). Therefore, a global mutex is used for all Reports, when delegations are added or
    //!   removed. This is why this must be a rare operation.
    //! - Adjusting the maximum severity is far less frequent than logging a message. However, since
    //!   we assume that many or most Reports do not have any delegate or delegators, we want to safely
    //!   update their maximum severity without locking the global mutex. We do this using atomic counters.
    //!   For Reports which are part of a delegation tree, we lock the global mutex and update the local
    //!   copy of the maximum severity in each Report in the tree.
    //!
    class TSCOREDLL Report
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
        //! It is unsafe to delete a Report when it has delegators (i.e. when other Reports
        //! delegate to this object). Race conditions exist when a delegator logs a message
        //! at the same time its delegate is destructed.
        //!
        virtual ~Report();

        //!
        //! Set maximum severity level.
        //! Messages with higher severities are not reported.
        //! @param [in] level Set report to that level.
        //!
        void setMaxSeverity(int level);

        //!
        //! Raise maximum severity level.
        //! The severity can only be increased (more verbose, more debug), never decreased.
        //! @param [in] level Set report at least to that level.
        //!
        void raiseMaxSeverity(int level);

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
        bool _got_errors = false;
        UString _prefix {};

        // Current maximum severity which applies to this object and all Reports in the delegation tree.
        volatile int _max_severity = Severity::Info;

        // Last maximum severity which was explicitly set in this report. Can be different from
        // _max_severity because_max_severity can be updated when another Report in the delegation
        // tree changes its maximum severity. The _last_max_severity is used to restore _max_severity
        // when this object no longer delegates its reporting.
        int _last_max_severity = Severity::Info;

        // Number of transactions on this node of the delegation tree. This counter is incremented
        // when the delegate or one of the delegotors is added or removed.
        std::atomic_intmax_t _transactions {0};

        // Delegate Report. When not null, all messages are logged through this other report (recursively
        // if the delegate has a delegate, etc.) Unsynchronized access during message logging, see comment
        // in the header of the class.
        Report* volatile _delegate = nullptr;

        // Indicate if _delegators is not empty.
        // Can be modified only under the global mutex.
        // Can be read without locking the global mutex.
        volatile bool _has_delegators = false;

        // Set of other instances which delegate to this object.
        // Can be modified only under the global mutex.
        std::set<Report*> _delegators {};

        // Set the severity of all its delegators, recursively, with global mutex held.
        void setDelegatorsMaxSeverityLocked(int level, Report* skip, int foolproof);
    };
}
