//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReport.h"
#include "tsEnumeration.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::Report::~Report()
{
}


//----------------------------------------------------------------------------
// Set maximum debug level.
//----------------------------------------------------------------------------

void ts::Report::setMaxSeverity(int level)
{
    _max_severity = level;
    if (level >= Severity::Debug) {
        log(level, u"debug level set to %d", {level});
    }
}

void ts::Report::raiseMaxSeverity(int level)
{
    if (_max_severity < level) {
        setMaxSeverity(level);
    }
}


//----------------------------------------------------------------------------
// Enumeration to use severity values on the command line for instance.
//----------------------------------------------------------------------------

const ts::Enumeration ts::Severity::Enums({
    {u"fatal",   ts::Severity::Fatal},
    {u"severe",  ts::Severity::Severe},
    {u"error",   ts::Severity::Error},
    {u"warning", ts::Severity::Warning},
    {u"info",    ts::Severity::Info},
    {u"verbose", ts::Severity::Verbose},
    {u"debug",   ts::Severity::Debug},
});


//----------------------------------------------------------------------------
// Formatted line prefix header for a severity
//----------------------------------------------------------------------------

ts::UString ts::Severity::Header(int severity)
{
    if (severity < Fatal) {
        // Invalid / undefined severity.
        return UString::Format(u"[%d] ", {severity});
    }
    else if (severity > Debug) {
        return UString::Format(u"Debug[%d]: ", {severity});
    }
    else {
        switch (severity) {
            case Fatal:
                return u"FATAL ERROR: ";
            case Severe:
                return u"SEVERE ERROR: ";
            case Error:
                return u"Error: ";
            case Warning:
                return u"Warning: ";
            case Debug:
                return u"Debug: ";
            default: // Including Info and Verbose
                return UString();
        }
    }
}


//----------------------------------------------------------------------------
// Message logging.
//----------------------------------------------------------------------------

void ts::Report::log(int severity, const UString& msg)
{
    if (severity <= Severity::Error) {
        _got_errors = true;
    }
    if (severity <= _max_severity) {
        writeLog(severity, msg);
    }
}

void ts::Report::log(int severity, const UChar* fmt, std::initializer_list<ArgMixIn> args)
{
    if (severity <= _max_severity) {
        log(severity, UString::Format(fmt, args));
    }
}

void ts::Report::log(int severity, const UString& fmt, std::initializer_list<ArgMixIn> args)
{
    if (severity <= _max_severity) {
        log(severity, UString::Format(fmt, args));
    }
}
