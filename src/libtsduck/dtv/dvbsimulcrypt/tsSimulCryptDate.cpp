//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSimulCryptDate.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimulCryptDate::SimulCryptDate(const Time& t)
{
    Time::Fields f(t);
    setYear(f.year);
    setMonth(f.month);
    setDay(f.day);
    setHour(f.hour);
    setMinute(f.minute);
    setSecond(f.second);
    setHundredth(f.millisecond / 10);
}

ts::SimulCryptDate::SimulCryptDate(int year, int month, int day, int hour, int minute, int second, int hundredth)
{
    setYear(year);
    setMonth(month);
    setDay(day);
    setHour(hour);
    setMinute(minute);
    setSecond(second);
    setHundredth(hundredth);
}


//----------------------------------------------------------------------------
// Get from DVB SimulCrypt TLV messages
//----------------------------------------------------------------------------

void ts::SimulCryptDate::get(const tlv::MessageFactory& mf, tlv::TAG tag)
{
    // Get address and size of parameter. May raise exception if no such parameter
    tlv::MessageFactory::Parameter p;
    mf.get(tag, p);

    // Check parameter size. Raise exception on error.
    if (p.length != sizeof(_data)) {
        throw tlv::DeserializationInternalError(
            UString::Format(u"Invalid DVB time size in parameter 0x%X, expected %d bytes, got %d", {tag, sizeof(_data), p.length}));
    }

    // Now get binary content
    std::memcpy(_data, p.addr, sizeof(_data));  // Flawfinder: ignore: memcpy()
}


//----------------------------------------------------------------------------
// Convert to a Time object
//----------------------------------------------------------------------------

ts::SimulCryptDate::operator ts::Time() const
{
    return Time(Time::Fields(year(), month(), day(), hour(), minute(), second(), 10 * hundredth()));
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::SimulCryptDate::operator ts::UString() const
{
    return UString::Format(u"%04d/%02d/%02d-%02d:%02d:%02d.%02d", {year(), month(), day(), hour(), minute(), second(), hundredth()});
}
