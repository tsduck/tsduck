//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReport.h"
#include "tsFatal.h"

// Default foolproof counter. Used to detect internal error and infinite loops
// in the trees of delegations. Shouldn't be necessary but I am paranoid.
#define FOOLPROOF 1000

// Global mutex. Used when delegations are modified. See comment in header file.
TS_STATIC_MUTEX(std::recursive_mutex, GlobalReportMutex);


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::Report::Report(int max_severity, const UString& prefix, Report* report) :
    _prefix(prefix),
    _max_severity(max_severity),
    _last_max_severity(max_severity)
{
    if (report != nullptr) {
        delegateReport(report);
    }
}

ts::Report::~Report()
{
    // Try to avoid locking when possible.
    if (_has_delegators || _delegate != nullptr) {

        // There is something global to do, use the global mutex.
        std::lock_guard<std::recursive_mutex> lock(GlobalReportMutex());

        // Unlink other reports which delegated to this. Possible race condition if delegators exist.
        if (!_delegators.empty()) {
            debug(u"internal error, possible race condition, destructing Report 0x%X with %d delegators", size_t(this), _delegators.size());
            for (auto child : _delegators) {
                child->_delegate = nullptr;
                child->_transactions += 1;
                if (child->_max_severity != child->_last_max_severity) {
                    child->_max_severity = child->_last_max_severity;
                    child->setDelegatorsMaxSeverityLocked(child->_last_max_severity, nullptr, FOOLPROOF);
                }
            }
            _delegators.clear();
        }
        _has_delegators = false;

        // Finally, remove ourselves from our delegate.
        if (_delegate != nullptr) {
            if (_delegate->_delegators.erase(this) == 0) {
                _delegate->error(u"internal error, destructing Report 0x%X, unknown in its delegate 0x%X", size_t(this), size_t(_delegate));
            }
            _delegate->_transactions += 1;
            _delegate->_has_delegators = !_delegate->_delegators.empty();
            _delegate = nullptr;
        }
    }
}


//----------------------------------------------------------------------------
// Set or raise the max severity of all delegators, with global mutex held.
//----------------------------------------------------------------------------

void ts::Report::setDelegatorsMaxSeverityLocked(int level, Report* skip, int foolproof)
{
    if (foolproof <= 0) {
        TS_FATAL("fatal internal error, infinite loop in Report delegation");
    }
    for (auto child : _delegators) {
        if (child != nullptr && child != skip) {
            child->_max_severity = level;
            child->setDelegatorsMaxSeverityLocked(level, nullptr, foolproof - 1);
        }
    }
}


//----------------------------------------------------------------------------
// Set or raise the maximum debug level.
//----------------------------------------------------------------------------

void ts::Report::raiseMaxSeverity(int level)
{
    if (_last_max_severity < level) {
        _last_max_severity = level;
    }
    if (_max_severity < level) {
        setMaxSeverity(level);
    }
}

void ts::Report::setMaxSeverity(int level)
{
    // Keep track of the last severity which was set here.
    _last_max_severity = level;

    if (_max_severity == level) {
        // No need to perform costly checks if nothing changes.
        return;
    }

    if (level >= Severity::Debug) {
        log(level, u"debug level set to %d", level);
    }

    // Try to update locally, without locking, if we have no delegate and no delegators.
    const intmax_t counter1 = _transactions.load();
    if (!_has_delegators && _delegate == nullptr) {
        // We are alone so far...
        _max_severity = level;
        if (_transactions.load() == counter1) {
            // No modification in the meantime, still alone, completed.
            return;
        }
    }

    // Acquire the global mutex, we have some global stuff to do.
    std::lock_guard<std::recursive_mutex> lock(GlobalReportMutex());

    _max_severity = level;

    // Update all delegators and their delegators.
    for (auto child : _delegators) {
        child->_max_severity = level;
        child->setDelegatorsMaxSeverityLocked(level, nullptr, FOOLPROOF);
    }

    // Update all levels of delegate.
    Report* previous = this;
    for (Report* del = _delegate; del != nullptr; del = del->_delegate) {
        del->_max_severity = level;
        del->setDelegatorsMaxSeverityLocked(level, previous, FOOLPROOF);
        previous = del;
    }
}


//----------------------------------------------------------------------------
// Delegate message logging to another report object.
//----------------------------------------------------------------------------

ts::Report* ts::Report::delegateReport(Report* report)
{
    // Avoid looping.
    if (report == this) {
        report = nullptr;
    }

    // No need to perform costly checks if nothing changes.
    if (_delegate == report) {
        return _delegate;
    }

    // Acquire the global mutex.
    std::lock_guard<std::recursive_mutex> lock(GlobalReportMutex());

    // Detect loops in the tree.
    for (Report* del = report; del != nullptr; del = del->_delegate) {
        if (del == this) {
            _delegate->error(u"internal error, Report 0x%X tries to delegate to 0x%X, would create a loop", size_t(this), size_t(report));
            return _delegate;
        }
    }

    // Unlink from previous.
    Report* const previous = _delegate;
    if (_delegate != nullptr) {
        if (_delegate->_delegators.erase(this) == 0) {
            _delegate->error(u"internal error, Report 0x%X unknown in its delegate 0x%X", size_t(this), size_t(_delegate));
        }
        _delegate->_transactions += 1;
        _delegate->_has_delegators = !_delegate->_delegators.empty();
        _delegate = nullptr;
    }

    // The new max severity to set to us and our delegators.
    // If there is no new delegate, restore the last level which was explicitly set to this report.
    int new_severity = _last_max_severity;

    // Link to new.
    if (report != nullptr) {
        new_severity = report->_max_severity;
        report->_delegators.insert(this);
        report->_transactions += 1;
        report->_has_delegators = true;
        _delegate = report;
    }
    _transactions += 1;

    // Adjust severity above us.
    if (_max_severity != new_severity) {
        _max_severity = new_severity;
        for (auto child : _delegators) {
            child->_max_severity = new_severity;
            child->setDelegatorsMaxSeverityLocked(new_severity, nullptr, FOOLPROOF);
        }
    }

    return previous;
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
