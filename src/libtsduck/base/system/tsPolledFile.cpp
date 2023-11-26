//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPolledFile.h"


//----------------------------------------------------------------------------
// Enumerations, names for values
//----------------------------------------------------------------------------

const ts::Enumeration ts::PolledFile::StatusEnumeration({
    {u"modified", ts::PolledFile::MODIFIED},
    {u"added",    ts::PolledFile::ADDED},
    {u"deleted",  ts::PolledFile::DELETED},
});


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
