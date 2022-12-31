//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tstlvLogger.h"


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::tlv::Logger::Logger(int default_level, Report* default_report) :
    _report(default_report != nullptr ? default_report : NullReport::Instance()),
    _default_level(default_level),
    _levels()
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
