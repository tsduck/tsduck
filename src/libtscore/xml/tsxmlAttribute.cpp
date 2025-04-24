//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlAttribute.h"
#include "tsEnvironment.h"

// A non-thread-safe allocator for sequence numbers.
std::atomic_size_t ts::xml::Attribute::_allocator(0);


//----------------------------------------------------------------------------
// A constant static invalid instance.
//----------------------------------------------------------------------------

const ts::xml::Attribute& ts::xml::Attribute::INVALID()
{
    // Thread-safe init-safe static data pattern:
    static const Attribute invalid;
    return invalid;
}


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::xml::Attribute::Attribute() :
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
// Expand all environment variables in the attribute value.
//----------------------------------------------------------------------------

void ts::xml::Attribute::expandEnvironment()
{
    static const UString intro(u"${");
    if (_value.contains(intro)) {
        _value = ExpandEnvironment(_value, ExpandOptions::BRACES);
    }
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

void ts::xml::Attribute::setDateTime(const Time& value)
{
    setString(DateTimeToString(value));
}

void ts::xml::Attribute::setDate(const Time& value)
{
    setString(DateToString(value));
}


//----------------------------------------------------------------------------
// Static date/time conversions.
//----------------------------------------------------------------------------

ts::UString ts::xml::Attribute::DateTimeToString(const Time& value)
{
    const Time::Fields f(value);
    return UString::Format(u"%04d-%02d-%02d %02d:%02d:%02d", f.year, f.month, f.day, f.hour, f.minute, f.second);
}

ts::UString ts::xml::Attribute::DateToString(const Time& value)
{
    const Time::Fields f(value);
    return UString::Format(u"%04d-%02d-%02d", f.year, f.month, f.day);
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
