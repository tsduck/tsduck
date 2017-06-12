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

#include "tsReportInterface.h"
#include "tsFormat.h"

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const int ts::Severity::Fatal;
const int ts::Severity::Severe;
const int ts::Severity::Error;
const int ts::Severity::Warning;
const int ts::Severity::Info;
const int ts::Severity::Verbose;
const int ts::Severity::Debug;
#endif


//----------------------------------------------------------------------------
// Set maximum debug level.
//----------------------------------------------------------------------------

void ts::ReportInterface::setDebugLevel(int level)
{
    _max_severity = level;
    if (level >= Severity::Debug) {
        log(level, "debug level set to %d", level);
    }
}


//----------------------------------------------------------------------------
// Formatted line prefix header for a severity
//----------------------------------------------------------------------------

std::string ts::Severity::Header(int severity)
{
    if (severity < Fatal) {
        return Format("[%d] ", severity);
    }
    else if (severity > Debug) {
        return Format("Debug[%d]: ", severity);
    }
    else {
        switch (severity) {
            case Fatal:
                return "FATAL ERROR: ";
            case Severe:
                return "SEVERE ERROR: ";
            case Error:
                return "Error: ";
            case Warning:
                return "Warning: ";
            case Debug:
                return "Debug: ";
            default: // Including Info and Verbose
                return "";
        }
    }

    // should not get there
    assert(false);
    return "";
}


//----------------------------------------------------------------------------
// Message logging.
//----------------------------------------------------------------------------

void ts::ReportInterface::log(int severity, const std::string& msg)
{
    if (severity <= _max_severity) {
        writeLog(severity, msg);
    }
}

void ts::ReportInterface::log(int severity, const char* format, ...)
{
    if (severity <= _max_severity) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(severity, result);
    }
}

void ts::ReportInterface::fatal(const char* format, ...)
{
    if (_max_severity >= Severity::Fatal) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Fatal, result);
    }
}

void ts::ReportInterface::severe(const char* format, ...)
{
    if (_max_severity >= Severity::Severe) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Severe, result);
    }
}

void ts::ReportInterface::error(const char* format, ...)
{
    if (_max_severity >= Severity::Error) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Error, result);
    }
}

void ts::ReportInterface::warning(const char* format, ...)
{
    if (_max_severity >= Severity::Warning) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Warning, result);
    }
}

void ts::ReportInterface::info(const char* format, ...)
{
    if (_max_severity >= Severity::Info) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Info, result);
    }
}

void ts::ReportInterface::verbose(const char* format, ...)
{
    if (_max_severity >= Severity::Verbose) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Verbose, result);
    }
}

void ts::ReportInterface::debug(const char* format, ...)
{
    if (_max_severity >= Severity::Debug) {
        std::string result;
        TS_FORMAT_STRING(result, format);
        writeLog(Severity::Debug, result);
    }
}
