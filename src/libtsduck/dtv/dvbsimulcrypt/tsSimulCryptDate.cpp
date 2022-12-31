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

#include "tsSimulCryptDate.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::SimulCryptDate::SIZE;
#endif


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimulCryptDate::SimulCryptDate (const Time& t)
{
    Time::Fields f (t);
    setYear      (f.year);
    setMonth     (f.month);
    setDay       (f.day);
    setHour      (f.hour);
    setMinute    (f.minute);
    setSecond    (f.second);
    setHundredth (f.millisecond / 10);
}

ts::SimulCryptDate::SimulCryptDate (int year, int month, int day, int hour, int minute, int second, int hundredth)
{
    setYear      (year);
    setMonth     (month);
    setDay       (day);
    setHour      (hour);
    setMinute    (minute);
    setSecond    (second);
    setHundredth (hundredth);
}


//----------------------------------------------------------------------------
// Get from DVB SimulCrypt TLV messages
//----------------------------------------------------------------------------

void ts::SimulCryptDate::get (const tlv::MessageFactory& mf, tlv::TAG tag)
{
    // Get address and size of parameter. May raise exception if no such parameter
    tlv::MessageFactory::Parameter p;
    mf.get (tag, p);

    // Check parameter size. Raise exception on error.
    if (p.length != sizeof(_data)) {
        throw tlv::DeserializationInternalError(
            UString::Format(u"Invalid DVB time size in parameter 0x%X, expected %d bytes, got %d", {tag, sizeof(_data), p.length}));
    }

    // Now get binary content
    ::memcpy(_data, p.addr, sizeof(_data));  // Flawfinder: ignore: memcpy()
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
