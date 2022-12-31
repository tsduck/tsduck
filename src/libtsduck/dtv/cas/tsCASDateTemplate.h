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


#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
template <int YEARBASE> constexpr uint16_t ts::CASDate<YEARBASE>::INVALID_DATE;
template <int YEARBASE> constexpr int ts::CASDate<YEARBASE>::MIN_YEAR;
template <int YEARBASE> constexpr int ts::CASDate<YEARBASE>::MAX_YEAR;
#endif

//-----------------------------------------------------------------------------
// Compute the 16-bit value.
//-----------------------------------------------------------------------------

template <int YEARBASE>
uint16_t ts::CASDate<YEARBASE>::toUInt16(int year, int month, int day) const
{
    if (year >= MIN_YEAR && year <= MAX_YEAR && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        return uint16_t((year - YEARBASE) << 9) | uint16_t(month << 5) | uint16_t(day);
    }
    else {
        return INVALID_DATE;
    }
}


//-----------------------------------------------------------------------------
// Constructor from Time
//-----------------------------------------------------------------------------

template <int YEARBASE>
ts::CASDate<YEARBASE>::CASDate(const Time& t) : _value(0)
{
    const Time::Fields f(t);
    _value = toUInt16(f.year, f.month, f.day);
}

//-----------------------------------------------------------------------------
// Convert to a string object.
//-----------------------------------------------------------------------------

template <int YEARBASE>
ts::UString ts::CASDate<YEARBASE>::toString() const
{
    return isValid() ? UString::Format(u"%04d-%02d-%02d", {year(), month(), day()}) : u"?";
}

//-----------------------------------------------------------------------------
// Convert to a Time object.
//-----------------------------------------------------------------------------

template <int YEARBASE>
ts::CASDate<YEARBASE>::operator ts::Time() const
{
    return isValid() ? Time(year(), month(), day(), 0, 0) : Time::Epoch;
}
