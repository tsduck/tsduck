//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    _appName(UString(ExecutableFile().stem()).toLower()),
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
