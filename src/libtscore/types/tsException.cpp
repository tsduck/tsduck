//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsException.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"

ts::Exception::Exception(const UString& w) :
    _what(w.toUTF8())
{
    CERR.log(Severity::Debug, u"Exception: %s", w);
}

ts::Exception::Exception(const UString& w, int error) :
    Exception(UString::Format(u"%s, system error %n, %s", w, error, SysErrorCodeMessage(error)))
{
}

ts::Exception::~Exception() noexcept
{
}

const char* ts::Exception::what() const noexcept
{
    return _what.c_str();
}
