//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvLogger.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::tlv::Logger::Logger(Report* report, int default_level, Object* owner) :
    ReporterBase(report, owner),
    _default_level(default_level)
{
}

ts::tlv::Logger::Logger(ReporterBase* delegate, int default_level, Object* owner) :
    ReporterBase(delegate, owner),
    _default_level(default_level)
{
}

ts::tlv::Logger::~Logger()
{
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

void ts::tlv::Logger::log(const Message& msg, const UString& comment)
{
    const int level = severity(msg.tag());
    if (report().maxSeverity() >= level) {
        const UString dump(msg.dump(4));
        if (comment.empty()) {
            report().log(level, dump);
        }
        else {
            report().log(level, u"%s\n%s", comment, dump);
        }
    }
}
