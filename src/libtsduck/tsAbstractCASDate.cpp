//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Base class for representation of a CAS date.
//  This general format is used by several CAS vendors.
//
//----------------------------------------------------------------------------

#include "tsAbstractCASDate.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Compute the 16-bit value.
// Subclass specific, cannot be compared between subclasses
//-----------------------------------------------------------------------------

uint16_t ts::AbstractCASDate::toUInt16(int year, int month, int day) const
{
    if (year >= _year_base && year <= _year_base + 127 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        return (uint16_t(year - _year_base) << 9) | ((uint16_t(month) & 0x000F) << 5) | (uint16_t(day) & 0x001F);
    }
    else {
        return InvalidDate;
    }
}


//-----------------------------------------------------------------------------
// Constructor from Time
//-----------------------------------------------------------------------------

ts::AbstractCASDate::AbstractCASDate(int year_base, const Time& t) :
    _year_base(year_base),
    _value(0)
{
    const Time::Fields f(t);
    _value = toUInt16(f.year, f.month, f.day);
}


//-----------------------------------------------------------------------------
// Assigment
//-----------------------------------------------------------------------------

ts::AbstractCASDate& ts::AbstractCASDate::operator=(const AbstractCASDate& date)
{
    if (_year_base == date._year_base) {
        _value = date._value;
    }
    else {
        _value = toUInt16(date.year(), date.month(), date.day());
    }
    return *this;
}
