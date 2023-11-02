//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvLogger.h"
#include "tsNullReport.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::tlv::Logger::Logger(int default_level, Report* default_report) :
    _report(default_report != nullptr ? default_report : &NULLREP),
    _default_level(default_level)
{
}


//----------------------------------------------------------------------------
// Set a new default report object.
//----------------------------------------------------------------------------

void ts::tlv::Logger::setReport(Report* default_report)
{
    _report = default_report != nullptr ? default_report : &NULLREP;
}


//----------------------------------------------------------------------------
// Severity levels.
//----------------------------------------------------------------------------

int ts::tlv::Logger::severity(TAG tag) const
{
    auto it = _levels.find(tag);
    return it == _levels.end() ? _default_level : it->second;
}

void ts::tlv::Logger::resetSeverities(int default_level)
{
    _default_level = default_level;
    _levels.clear();
}


//----------------------------------------------------------------------------
// Report a TLV message.
//----------------------------------------------------------------------------

void ts::tlv::Logger::log(const Message& msg, const UString& comment, Report* report)
{
    Report* rep = report != nullptr ? report : _report;
    const int level = severity(msg.tag());
    if (rep->maxSeverity() >= level) {
        const UString dump(msg.dump(4));
        if (comment.empty()) {
            rep->log(level, dump);
        }
        else {
            rep->log(level, u"%s\n%s", {comment, dump});
        }
    }
}
