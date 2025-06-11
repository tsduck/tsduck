//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton implementing Report on std::cerr without synchronization
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsLibTSCoreVersion.h"

namespace ts {
    //!
    //! A singleton implementing Report on std::cerr without synchronization.
    //! @ingroup libtscore log
    //!
    //! If the environment variable TS_CERR_DEBUG_LEVEL is set to some integer
    //! value, it is used as initial maximum severity level for this object.
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    class TSCOREDLL CerrReport : public Report
    {
        TS_SINGLETON(CerrReport);
    public:
        //!
        //! A class with constructors which log messages.
        //! Useful to log debug message on standard error during initialization.
        //! @see TS_CERR_DEBUG_LEVEL
        //! @see TS_CERR_DEBUG
        //!
        class ReportConstructor
        {
        public:
            //!
            //! Report a debug message with a printf-like interface.
            //! @param [in] fmt Format string with embedded '\%' sequences.
            //! @param [in] args List of arguments to substitute in the format string.
            //! @see UString::format()
            //!
            template <class... Args>
            ReportConstructor(const UChar* fmt, Args&&... args)
            {
                CerrReport::Instance().log(Severity::Debug, fmt, std::forward<ArgMixIn>(args)...);
            }

            //!
            //! Report a message with an explicit severity and a printf-like interface.
            //! @param [in] severity Message severity.
            //! @param [in] fmt Format string with embedded '\%' sequences.
            //! @param [in] args List of arguments to substitute in the format string.
            //! @see UString::format()
            //!
            template <class... Args>
            ReportConstructor(int severity, const UChar* fmt, Args&&... args)
            {
                CerrReport::Instance().log(severity, UString::Format(fmt, std::forward<ArgMixIn>(args)...));
            }
        };
    protected:
        // Report implementation
        virtual void writeLog(int severity, const UString& msg) override;
    };
}

//!
//! Macro for fast access to the ts::CerrReport singleton.
//!
#define CERR (ts::CerrReport::Instance())

//!
//! Macro which logs debug messages on standard error during initialization.
//! Same parameters as ts::Report::debug() or ts::Report::log().
//! @hideinitializer
//!
#define TS_CERR_DEBUG(...) \
    TS_LIBTSCORE_CHECK(); \
    static ts::CerrReport::ReportConstructor TS_UNIQUE_NAME(_Registrar)(__VA_ARGS__)
