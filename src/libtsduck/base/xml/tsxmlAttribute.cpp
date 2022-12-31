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

#include "tsxmlAttribute.h"

// A constant static invalid instance.
const ts::xml::Attribute ts::xml::Attribute::INVALID;

// A non-thread-safe allocator for sequence numbers.
std::atomic_size_t  ts::xml::Attribute::_allocator(0);

//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Attribute::Attribute() :
    _valid(false),
    _name(),
    _value(),
    _line(0),
    _sequence(++_allocator)
{
}

ts::xml::Attribute::Attribute(const UString& name, const UString& value, size_t line) :
    _valid(true),
    _name(name),
    _value(value),
    _line(line),
    _sequence(++_allocator)
{
}


//----------------------------------------------------------------------------
// Get the formatted attribute value with quotes and escaped characters.
//----------------------------------------------------------------------------

const ts::UString ts::xml::Attribute::formattedValue(const ts::xml::Tweaks& tweaks) const
{
    // Get the quote character to use.
    UChar quote = tweaks.attributeValueQuote();

    // List of characters to escape.
    UString escape;

    if (tweaks.strictAttributeFormatting) {
        // With strict formatting, escape all characters.
        escape = u"<>&'\"";
    }
    else {
        // Without strict formatting, escape required characters only.
        escape = u"&";
        // Try to find a unique quote to avoid escape characters.
        if (_value.find(quote) != NPOS) {
            // The default quote is present, try the other one.
            const UChar otherQuote = tweaks.attributeValueOtherQuote();
            if (_value.find(otherQuote) == NPOS) {
                // The other quote is not present, use it. Nothing to escape.
                quote = otherQuote;
            }
            else {
                // The other quote is present as well. Keep default quote and escape it.
                escape.append(quote);
            }
        }
    }

    // Full formatted value.
    return quote + _value.toHTML(escape) + quote;
}


//----------------------------------------------------------------------------
// Set attribute value.
//----------------------------------------------------------------------------

void ts::xml::Attribute::setString(const UString& value)
{
    _value = value;
    _sequence = ++_allocator;
}

void ts::xml::Attribute::setBool(bool value)
{
    setString(UString::TrueFalse(value));
}

void ts::xml::Attribute::setEnum(const Enumeration& definition, int value)
{
    setString(definition.name(value));
}

void ts::xml::Attribute::setDateTime(const Time& value)
{
    setString(DateTimeToString(value));
}

void ts::xml::Attribute::setDate(const Time& value)
{
    setString(DateToString(value));
}

void ts::xml::Attribute::setTime(Second value)
{
    setString(TimeToString(value));
}


//----------------------------------------------------------------------------
// Static date/time conversions.
//----------------------------------------------------------------------------

ts::UString ts::xml::Attribute::DateTimeToString(const Time& value)
{
    const Time::Fields f(value);
    return UString::Format(u"%04d-%02d-%02d %02d:%02d:%02d", {f.year, f.month, f.day, f.hour, f.minute, f.second});
}

ts::UString ts::xml::Attribute::DateToString(const Time& value)
{
    const Time::Fields f(value);
    return UString::Format(u"%04d-%02d-%02d", {f.year, f.month, f.day});
}

ts::UString ts::xml::Attribute::TimeToString(Second value)
{
    return UString::Format(u"%02d:%02d:%02d", {value / 3600, (value / 60) % 60, value % 60});
}

bool ts::xml::Attribute::DateTimeFromString(Time& value, const UString& str)
{
    // We are tolerant on syntax, decode 6 fields, regardless of separators.
    return value.decode(str, Time::YEAR | Time::MONTH | Time::DAY | Time::HOUR | Time::MINUTE | Time::SECOND);
}

bool ts::xml::Attribute::DateFromString(Time& value, const UString& str)
{
    // We are tolerant on syntax, decode 3 fields, regardless of separators.
    return value.decode(str, Time::YEAR | Time::MONTH | Time::DAY);
}

bool ts::xml::Attribute::TimeFromString(Second& value, const UString& str)
{
    Second hours = 0;
    Second minutes = 0;
    Second seconds = 0;

    const bool ok = str.scan(u"%d:%d:%d", {&hours, &minutes, &seconds}) &&
        hours   >= 0 && hours   <= 23 &&
        minutes >= 0 && minutes <= 59 &&
        seconds >= 0 && seconds <= 59;

    if (ok) {
        value = (hours * 3600) + (minutes * 60) + seconds;
    }

    return ok;
}
