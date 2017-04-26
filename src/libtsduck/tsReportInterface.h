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
//!  Abstract interface for event reporting and monitoring
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //!
    //! Message severity.
    //! Positive values are debug levels. The typical default reporting level is @c Info.
    //! All messages with a higher level (@c Verbose and all debug levels) are not reported by default.
    //! The @c struct is here just to add a naming level
    //!
    struct TSDUCKDLL Severity {

        static const int None    = -6;  //!< No message is reported at this level or below.
        static const int Fatal   = -5;  //!< Fatal error, typically aborts the application.
        static const int Severe  = -4;  //!< Severe errror.
        static const int Error   = -3;  //!< Regular error.
        static const int Warning = -2;  //!< Warning message.
        static const int Info    = -1;  //!< Information message.
        static const int Verbose = 0;   //!< Verbose information.
        static const int Debug   = 1;   //!< First debug level.

        //!
        //! Formatted line prefix header for a severity
        //! @param [in] severity Severity value.
        //! @return A string to prepend to messages. Empty for Info and Verbose levels.
        //!
        static std::string Header(int severity);
    };

    //!
    //! Abstract interface for event reporting and monitoring.
    //!
    class TSDUCKDLL ReportInterface
    {
    public:

        //!
        //! Constructor.
        //! The default initial report level is Info.
        //! @param [in] verbose If true, set initial report level to Verbose.
        //! @param [in] debug_level If greater than zero, set initial report to that level and ignore @a verbose.
        //!
        ReportInterface(bool verbose = false, int debug_level = 0) :
            _max_severity(debug_level > 0 ? debug_level : (verbose ? Severity::Verbose : Severity::Info))
        {
        }

        //!
        //! Destructor.
        //!
        virtual ~ReportInterface() {}

        //!
        //! Set maximum debug level.
        //! @param [in] level Set report to that level.
        //!
        virtual void setDebugLevel(int level);

        //!
        //! Get maximum debug level.
        //! @return Current maximum debug level.
        //!
        virtual int debugLevel() const {return _max_severity;}

        //!
        //! Check if debugging is active.
        //! @return True if current reporting level is Debug or higher.
        //!
        virtual bool debug() const {return _max_severity >= Severity::Debug;}

        //!
        //! Check if verbose reporting is active.
        //! @return True if current reporting level is Verbose or higher.
        //!
        virtual bool verbose() const {return _max_severity >= Severity::Verbose;}

        //!
        //! Report a message with an explicit severity.
        //! @param [in] severity Message severity.
        //! @param [in] msg Message text.
        //!
        virtual void log(int severity, const std::string& msg);

        //!
        //! Report a fatal error message.
        //! @param [in] msg Message text.
        //!
        virtual void fatal(const std::string& msg)
        {
            log(Severity::Fatal, msg);
        }

        //!
        //! Report a severe error message.
        //! @param [in] msg Message text.
        //!
        virtual void severe(const std::string& msg)
        {
            log(Severity::Severe, msg);
        }

        //!
        //! Report an error message.
        //! @param [in] msg Message text.
        //!
        virtual void error(const std::string& msg)
        {
            log(Severity::Error, msg);
        }

        //!
        //! Report a warning message.
        //! @param [in] msg Message text.
        //!
        virtual void warning(const std::string& msg)
        {
            log(Severity::Warning, msg);
        }

        //!
        //! Report an informational message.
        //! @param [in] msg Message text.
        //!
        virtual void info(const std::string& msg)
        {
            log(Severity::Info, msg);
        }

        //!
        //! Report a verbose message.
        //! @param [in] msg Message text.
        //!
        virtual void verbose(const std::string& msg)
        {
            log(Severity::Verbose, msg);
        }

        //!
        //! Report a debug message.
        //! @param [in] msg Message text.
        //!
        virtual void debug(const std::string& msg)
        {
            log(Severity::Debug, msg);
        }

        //!
        //! Report a message with an explicit severity and a printf-like interface.
        //! @param [in] severity Message severity.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void log(int severity, const char* format, ...) TS_PRINTF_FORMAT(3, 4);

        //!
        //! Report a fatal error message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void fatal(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report a severe error message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void severe(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report an error message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void error(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report a warning message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void warning (const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report an informational message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void info(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report a verbose message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void verbose(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

        //!
        //! Report a debug message with a printf-like interface.
        //! @param [in] format Printf-like format string. Followed by variable-length list of arguments.
        //!
        virtual void debug(const char* format, ...) TS_PRINTF_FORMAT(2, 3);

    protected:
        //!
        //! Debug level is accessible to subclasses
        //!
        volatile int _max_severity;

        //!
        //! Actual message reporting method.
        //! Must be implemented in actual classes
        //! @param [in] severity Message severity.
        //! @param [in] msg Message text.
        //!
        virtual void writeLog(int severity, const std::string& msg) = 0;
    };
}
