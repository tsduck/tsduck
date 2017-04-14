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
//
//  Abstract interface for event reporting and monitoring
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    // Message severity. Positive values are debug levels.
    // The "struct" is here to add a naming level
    struct TSDUCKDLL Severity {

        static const int None    = -6;
        static const int Fatal   = -5;
        static const int Severe  = -4;
        static const int Error   = -3;
        static const int Warning = -2;
        static const int Info    = -1;
        static const int Verbose = 0;
        static const int Debug   = 1;

        // Formatted line prefix header for a severity
        static std::string Header (int severity);
    };

    class TSDUCKDLL ReportInterface
    {
    public:

        // Constructor
        ReportInterface(bool verbose = false, int debug_level = 0) :
            _max_severity(debug_level > 0 ? debug_level : (verbose ? Severity::Verbose : Severity::Info))
        {
        }

        // Destructor
        virtual ~ReportInterface() {}

        // Set/Get maximum debug level.

        virtual void setDebugLevel(int level);
        virtual int debugLevel() const {return _max_severity;}
        virtual bool debug() const {return _max_severity >= Severity::Debug;}
        virtual bool verbose() const {return _max_severity >= Severity::Verbose;}

        // Message reporting methods.

        // String interfaces.
        virtual void log(int severity, const std::string& msg);

        virtual void fatal   (const std::string& msg) {log(Severity::Fatal,   msg);}
        virtual void severe  (const std::string& msg) {log(Severity::Severe,  msg);}
        virtual void error   (const std::string& msg) {log(Severity::Error,   msg);}
        virtual void warning (const std::string& msg) {log(Severity::Warning, msg);}
        virtual void info    (const std::string& msg) {log(Severity::Info,    msg);}
        virtual void verbose (const std::string& msg) {log(Severity::Verbose, msg);}
        virtual void debug   (const std::string& msg) {log(Severity::Debug,   msg);}

        // Printf-like interfaces.
        virtual void log(int severity, const char* format, ...) TS_PRINTF_FORMAT(3, 4);

        virtual void fatal   (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void severe  (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void error   (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void warning (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void info    (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void verbose (const char* format, ...) TS_PRINTF_FORMAT(2, 3);
        virtual void debug   (const char* format, ...) TS_PRINTF_FORMAT(2, 3);

    protected:
        // Debug level is accessible to subclasses
        volatile int _max_severity;

        // Must be implemented in actual classes
        virtual void writeLog(int severity, const std::string& msg) = 0;
    };
}
