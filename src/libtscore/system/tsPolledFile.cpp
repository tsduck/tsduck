//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPolledFile.h"


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Names& ts::PolledFile::StatusEnumeration()
{
    static const Names data({
        {u"modified", MODIFIED},
        {u"added",    ADDED},
        {u"deleted",  DELETED},
    });
    return data;
}


//----------------------------------------------------------------------------
// Description of a polled file - Constructor
//----------------------------------------------------------------------------

ts::PolledFile::PolledFile(const UString& name, const int64_t& size, const Time& date, const Time& now) :
    _name(name),
    _status(ADDED),
    _file_size(size),
    _file_date(date),
    _pending(true),
    _found_date(now)
{
}


//----------------------------------------------------------------------------
// Check if file has changed size or date.
//----------------------------------------------------------------------------

void ts::PolledFile::trackChange(const std::uintmax_t& size, const Time& date, const Time& now)
{
    if (_file_size != size || _file_date != date) {
        _status = MODIFIED;
        _file_size = size;
        _file_date = date;
        _pending = true;
        _found_date = now;
    }
}
