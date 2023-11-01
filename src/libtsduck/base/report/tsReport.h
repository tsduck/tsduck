//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface for event reporting and monitoring
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUChar.h"
#include "tsArgMix.h"

namespace ts {

    class Enumeration;
    class UString;

    //!
    //! Message severity.
    //! @ingroup log
    //!
    //! Positive values are debug levels. The typical default reporting level is @c Info.
    //! All messages with a higher level (@c Verbose and all debug levels) are not reported by default.
    //! The @c struct is here just to add a naming level.
    //!
    struct TSDUCKDLL Severity {

        static constexpr int Fatal   = -5;  //!< Fatal error, typically aborts the application.
        static constexpr int Severe  = -4;  //!< Severe errror.
        static constexpr int Error   = -3;  //!< Regular error.
        static constexpr int Warning = -2;  //!< Warning message.
        static constexpr int Info    = -1;  //!< Information message.
        static constexpr int Verbose = 0;   //!< Verbose information.
        static constexpr int Debug   = 1;   //!< First debug level.

        //!
        //! Formatted line prefix header for a severity
        //! @param [in] severity Severity value.
        //! @return A string to prepend to messages. Empty for Info and Verbose levels.
        //!
        static UString Header(int severity);

        //!
        //! An enumeration to use severity values on the command line for instance.
        //!
        static const Enumeration Enums;
    };

    //!
    //! Abstract interface for event reporting and monitoring.
    //! @ingroup log
    //!
    class TSDUCKDLL Report
    {
    public:
        //!
        //! Constructor.
        //! The default initial report level is Info.
        //! @param [in] max_severity Initial maximum severity of reported messages.
        //!
        Report(int max_severity = Severity::Info) : _max_severity(max_severity) {}

        //!
        //! Destructor.
        //!
        virtual ~Report();

        //!
        //! Set maximum severity level.
        //! Messages with higher severities are not reported.
        //! @param [in] level Set report to that level.
        //!
        virtual void setMaxSeverity(int level);

        //!
        //! Raise maximum severity level.
        //! @param [in] level Set report at least to that level.
        //!
        virtual void raiseMaxSeverity(int level);

        //!
        //! Get maximum severity level.
        //! @return Current maximum debug level.
        //!
        int maxSeverity() const { return _max_severity; }

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

        //!
        //! Report a message with an explicit severity.
        //!
        //! This method is the central reporting point. If filters the severity
        //! and drops the message if @a severity is higher than maxSeverity().
        //!
        //! Subclasses should override writeLog() to implement a specific reporting
        //! device. It is not necessary to override log() unless the subclass needs
        //! to implement a different severity filtering policy.
        //!
        //! @param [in] severity Message severity.
        //! @param [in] msg Message text.
        //!
        virtual void log(int severity, const UString& msg);

        //!
        //! Report a message with an explicit severity and a printf-like interface.
        //! @param [in] severity Message severity.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        virtual void log(int severity, const UChar* fmt, std::initializer_list<ArgMixIn> args);

        //!
        //! Report a message with an explicit severity and a printf-like interface.
        //! @param [in] severity Message severity.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        virtual void log(int severity, const UString& fmt, std::initializer_list<ArgMixIn> args);

        //!
        //! Report a fatal error message.
        //! @param [in] msg Message text.
        //!
        void fatal(const UString& msg) { log(Severity::Fatal, msg); }

        //!
        //! Report a fatal error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void fatal(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Fatal, fmt, args); }

        //!
        //! Report a fatal error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void fatal(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Fatal, fmt, args); }

        //!
        //! Report a severe error message.
        //! @param [in] msg Message text.
        //!
        void severe(const UString& msg) { log(Severity::Severe, msg); }

        //!
        //! Report a severe error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void severe(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Severe, fmt, args); }

        //!
        //! Report a severe error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void severe(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Severe, fmt, args); }

        //!
        //! Report an error message.
        //! @param [in] msg Message text.
        //!
        void error(const UString& msg) { log(Severity::Error, msg); }

        //!
        //! Report an error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void error(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Error, fmt, args); }

        //!
        //! Report an error message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void error(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Error, fmt, args); }

        //!
        //! Report a warning message.
        //! @param [in] msg Message text.
        //!
        void warning(const UString& msg) { log(Severity::Warning, msg); }

        //!
        //! Report a warning message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void warning(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Warning, fmt, args); }

        //!
        //! Report a warning message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void warning(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Warning, fmt, args); }

        //!
        //! Report an informational message.
        //! @param [in] msg Message text.
        //!
        void info(const UString& msg) { log(Severity::Info, msg); }

        //!
        //! Report an informational message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void info(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Info, fmt, args); }

        //!
        //! Report an informational message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void info(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Info, fmt, args); }

        //!
        //! Report a verbose message.
        //! @param [in] msg Message text.
        //!
        void verbose(const UString& msg) { log(Severity::Verbose, msg); }

        //!
        //! Report a verbose message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void verbose(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Verbose, fmt, args); }

        //!
        //! Report a verbose message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void verbose(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Verbose, fmt, args); }

        //!
        //! Report a debug message.
        //! @param [in] msg Message text.
        //!
        void debug(const UString& msg) { log(Severity::Debug, msg); }

        //!
        //! Report a debug message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void debug(const UChar* fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Debug, fmt, args); }

        //!
        //! Report a debug message with a printf-like interface.
        //! @param [in] fmt Format string with embedded '\%' sequences.
        //! @param [in] args List of arguments to substitute in the format string.
        //! @see UString::format()
        //!
        void debug(const UString& fmt, std::initializer_list<ArgMixIn> args) { log(Severity::Debug, fmt, args); }

        //!
        //! Check if errors (or worse) were reported through this object.
        //! @return True if errors (or worse) were reported through this object.
        //!
        bool gotErrors() const { return _got_errors; }

        //!
        //! Reset the error indicator.
        //! @see gotErrors()
        //!
        void resetErrors() { _got_errors = false; }

    protected:
        //!
        //! Debug level is accessible to subclasses
        //!
        volatile int _max_severity = Severity::Info;

        //!
        //! Actual message reporting method.
        //!
        //! Must be implemented in actual classes.
        //! The method is called only when a message passed the severity filter.
        //! It is not necessary to recheck @a severity inside the method.
        //!
        //! @param [in] severity Message severity.
        //! @param [in] msg Message text.
        //!
        virtual void writeLog(int severity, const UString& msg) = 0;

    private:
        volatile bool _got_errors = false;
    };
}
