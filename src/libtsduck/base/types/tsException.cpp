//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsException.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"

ts::Exception::Exception(const UString& w) :
    _what(w),
    _utf8()
{
    CERR.log(Severity::Debug, u"Exception: " + _what);
}

ts::Exception::Exception(const UString& w, int error) :
    Exception(UString::Format(u"%s, system error %d (0x%X), %s", {w, error, error, SysErrorCodeMessage(error)}))
{
}

ts::Exception::~Exception() noexcept
{
}

const char* ts::Exception::what() const noexcept
{
    if (_utf8.empty() && !_what.empty()) {
        _utf8 = _what.toUTF8();
    }
    return _utf8.c_str();
}
