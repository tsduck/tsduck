//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReport.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::Report::Report(int max_severity, const UString& prefix, Report* report) :
    _max_severity(max_severity),
    _prefix(prefix)
{
    if (report != nullptr && report != this) {
        std::lock_guard<std::mutex> lock(report->_mutex);
        report->_delegated.insert(this);
        _delegate = report;
    }
}

ts::Report::~Report()
{
    // Unlink from delegate, if there is one. _delegate is volatile, use a copy.
    Report* const del = _delegate;
    if (del != nullptr) {
        std::lock_guard<std::mutex> lock(del->_mutex);
        del->_delegated.erase(this);
        _delegate = nullptr;
    }

    // Unlink other reports which delegated to this.
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto child : _delegated) {
        child->_delegate = nullptr;
    }
    _delegated.clear();
}


//----------------------------------------------------------------------------
// Delegate message logging to another report object.
//----------------------------------------------------------------------------

ts::Report* ts::Report::delegateReport(Report* report)
{
    Report* const previous = _delegate;

    // Avoid looping.
    if (report == this) {
        report = nullptr;
    }

    // Unlink from previous. _delegate is volatile, use a copy.
    Report* const del = _delegate;
    if (del != nullptr) {
        std::lock_guard<std::mutex> lock(del->_mutex);
        del->_delegated.erase(this);
        _delegate = nullptr;
    }

    // Link to new.
    if (report != nullptr) {
        std::lock_guard<std::mutex> lock(report->_mutex);
        report->_delegated.insert(this);
        _delegate = report;
    }

    return previous;
}


//----------------------------------------------------------------------------
// Set maximum debug level.
//----------------------------------------------------------------------------

void ts::Report::setMaxSeverity(int level, bool delegated)
{
    _max_severity = level;
    if (delegated) {
        for (Report* del = _delegate; del != nullptr; del = del->_delegate) {
            del->_max_severity = level;
        }
    }
    if (level >= Severity::Debug) {
        log(level, u"debug level set to %d", level);
    }
}

void ts::Report::raiseMaxSeverity(int level, bool delegated)
{
    if (_max_severity < level) {
        _max_severity = level;
    }
    if (delegated) {
        for (Report* del = _delegate; del != nullptr; del = del->_delegate) {
            if (del->_max_severity < level) {
                del->_max_severity = level;
            }
        }
    }
    if (level >= Severity::Debug) {
        log(level, u"debug level set to %d", level);
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
        // _delegate is volatile, use a copy.
        Report* const del = _delegate;
        if (del != nullptr) {
            del->log(severity, _prefix.empty() ? msg : _prefix + msg);
        }
        else {
            writeLog(severity, _prefix.empty() ? msg : _prefix + msg);
        }
    }
}


//----------------------------------------------------------------------------
// Actual message reporting method. By default, does nothing.
//----------------------------------------------------------------------------

void ts::Report::writeLog(int severity, const UString& msg)
{
}
