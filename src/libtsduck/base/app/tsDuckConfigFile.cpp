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

#include "tsDuckConfigFile.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"

// Define singleton instance
TS_DEFINE_SINGLETON(ts::DuckConfigFile);


//----------------------------------------------------------------------------
// Default constructor.
// On Windows, we use the legacy file name (same as Unix) as fallback.
// Do not load the configuration if TSDUCK_NO_USER_CONFIG is defined.
//----------------------------------------------------------------------------

ts::DuckConfigFile::DuckConfigFile() :
    ConfigFile(UserConfigurationFileName(u".tsduck", u"tsduck.ini"), NULLREP, u"TSDUCK_NO_USER_CONFIG"),
    _appName(PathPrefix(BaseName(ExecutableFile())).toLower()),
    _appSection(section(_appName)),
    _mainSection(section(u""))
{
}


//----------------------------------------------------------------------------
// Get the value of an entry.
//----------------------------------------------------------------------------

ts::UString ts::DuckConfigFile::value(const ts::UString& entry, const ts::UString& defvalue) const
{
    return _appSection.valueCount(entry) > 0 ? _appSection.value(entry) : _mainSection.value(entry, 0, defvalue);
}


//----------------------------------------------------------------------------
// Get all values of an entry.
//----------------------------------------------------------------------------

void ts::DuckConfigFile::getValues(const ts::UString& entry, ts::UStringVector& values) const
{
    values.clear();
    size_t count = 0;

    if ((count = _appSection.valueCount(entry)) > 0) {
        for (size_t i = 0; i < count; ++i) {
            values.push_back(_appSection.value(entry, i));
        }
    }
    else if ((count = _mainSection.valueCount(entry)) > 0) {
        for (size_t i = 0; i < count; ++i) {
            values.push_back(_mainSection.value(entry, i));
        }
    }
}
