//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISOTime.h"
#include "tsIntegerUtils.h"


//----------------------------------------------------------------------------
// Reset the content of the object to an invalid state.
//----------------------------------------------------------------------------

void ts::ISOTime::clear()
{
    _type = NONE;
    _start.clear();
    _end.clear();
    _duration = cn::milliseconds::zero();
    _recurrences = 0;
}


//----------------------------------------------------------------------------
// Format this object as a ISO 8601 string.
//----------------------------------------------------------------------------

ts::UString ts::ISOTime::toString(TimeType format) const
{
    // Set default format. There is only one possible format for single date & time.
    if (format == NONE || _type == TIME) {
        format = _type;
    }

    // Start with the recurring indicator.
    UString rec;
    if (isRecurring()) {
        if (isUnbounded()) {
            rec.append(u"R/");
        }
        else {
            rec.format(u"R%d/", _recurrences);
        }
    }

    Time st, en;
    static const UString empty;

    switch (format & ~RECURRING) {
        case TIME:
            st = start();
            return st == Time::Epoch ? empty : rec + ToISO(st);
        case DURATION:
            return rec + MillisecondsToIso(duration().count());
        case START_END:
            st = start();
            en = end();
            return st == Time::Epoch || en == Time::Epoch ? empty : rec + ToISO(st) + u"/" + ToISO(en);
        case START_DURATION:
            st = start();
            return st == Time::Epoch ? empty : rec + ToISO(st) + u"/" + MillisecondsToIso(duration().count());
        case DURATION_END:
            en = end();
            return en == Time::Epoch ? empty : rec + MillisecondsToIso(duration().count()) + u"/" + ToISO(en);
        default:
            return empty;
    }
}


//----------------------------------------------------------------------------
// Check if a string starts as a duration.
//----------------------------------------------------------------------------

namespace {
    bool IsDurationString(const ts::UString& str)
    {
        return !str.empty() && (str[0] == u'P' || str[0] == u'p');
    }
}


//----------------------------------------------------------------------------
// Set the value of this object from a string in ISO 8601 format.
//----------------------------------------------------------------------------

bool ts::ISOTime::fromString(const UString& str)
{
    // Clear all fields with invalid values.
    clear();

    // Split fields.
    UStringVector fields;
    str.split(fields, u'/', true, true);

    // Look for a recurrence count and point to first index after it.
    const size_t first = !fields.empty() && fields[0].starts_with(u'R') ? 1 : 0;
    if (first > 0) {
        // The first field is a number of recurrences.
        if (fields[0].length() == 1) {
            _recurrences = UNBOUNDED_RECURRENCES;
        }
        else if (!fields[0].substr(1).toInteger(_recurrences)) {
            return false; // invalid recurrence count
        }
    }

    // Decode, depending on the number of fields.
    if (fields.size() == first + 1) {
        // Only one field.
        if (IsDurationString(fields[first])) {
            // This is a duration.
            if ((_duration = DurationFromISO(fields[first])) < cn::milliseconds::zero()) {
                // Invalid duration
                clear();
            }
            else {
                _type = DURATION;
            }
        }
        else {
            // This must be a date & time. No recurrence allowed.
            if (_recurrences > 0 || (_start = TimeFromISO(fields[first])) == Time::Epoch) {
                // Invalid time.
                clear();
            }
            else {
                _type = TIME;
            }
        }
    }
    else if (fields.size() == first + 2) {
        // There are two fields. At most one can be a duration.
        const bool duration0 = IsDurationString(fields[first]);
        const bool duration1 = IsDurationString(fields[first+1]);
        if (!duration0 && !duration1) {
            // This is a start and end.
            if ((_start = TimeFromISO(fields[first])) == Time::Epoch || (_end = TimeFromISO(fields[first+1])) == Time::Epoch) {
                // Invalid time interval.
                clear();
            }
            else {
                _type = START_END;
            }
        }
        else if (!duration0 && duration1) {
            // This is a start and duration.
            if ((_start = TimeFromISO(fields[first])) == Time::Epoch || (_duration = DurationFromISO(fields[first+1])) < cn::milliseconds::zero()) {
                // Invalid time interval.
                clear();
            }
            else {
                _type = START_DURATION;
            }
        }
        else if (duration0 && !duration1) {
            // This is a duration and end.
            if ((_duration = DurationFromISO(fields[first])) < cn::milliseconds::zero() || (_end = TimeFromISO(fields[first+1])) == Time::Epoch) {
                // Invalid time interval.
                clear();
            }
            else {
                _type = DURATION_END;
            }
        }
        else {
            // Both fields are duration, this is invalid.
            clear();
        }
    }
    else {
        // Not the right number of fields.
        clear();
    }

    // Add the recurring flag when necessary.
    if (_recurrences > 0) {
        _type = TimeType(_type | RECURRING);
    }

    return isValid();
}


//----------------------------------------------------------------------------
// Simple values extraction.
//----------------------------------------------------------------------------

ts::Time ts::ISOTime::start() const
{
    switch (_type & ~RECURRING) {
        case TIME:
        case START_END:
        case START_DURATION:
            return _start;
        case DURATION_END:
            return _end - _duration;
        default:
            return Time::Epoch;
    }
}

ts::Time ts::ISOTime::end() const
{
    switch (_type & ~RECURRING) {
        case TIME:
            return _start;
        case START_END:
        case DURATION_END:
            return _end;
        case START_DURATION:
            return _start + _duration;
        default:
            return Time::Epoch;
    }
}

cn::milliseconds ts::ISOTime::duration() const
{
    switch (_type & ~RECURRING) {
        case START_END:
            return _end - _start;
        case DURATION:
        case START_DURATION:
        case DURATION_END:
            return _duration;
        default:
            return cn::milliseconds::zero();
    }
}


//----------------------------------------------------------------------------
// Find an integer in a string at given 'pos' (update pos).
// - Skip non digits.
// - Stop on 't' or 'T'.
// - Consume at most 'char_count' digits and return the result in 'value'.
// - Return true if ok, false if no value found.
// - Also update the 'width' of the integer field.
//----------------------------------------------------------------------------

namespace {
    bool IntFromString(const ts::UString& str, size_t& pos, const ts::UString& delimiters, size_t char_count, int& value, size_t& width)
    {
        static const ts::UString punct(u" -+:/");
        while (pos < str.size() && punct.contains(str[pos])) {
            if (delimiters.contains(str[pos])) {
                return false;  // reached end of allowed string
            }
            pos++;
        }
        if (pos < str.size() && delimiters.contains(str[pos])) {
            return false;  // reached end of allowed string
        }
        const size_t start = pos;
        while (pos < str.size() && (pos - start) < char_count && ts::IsDigit(str[pos])) {
            pos++;
        }
        width = pos - start;
        return str.substr(start, width).toInteger(value);
    }
}


//----------------------------------------------------------------------------
// Decode a time from an ISO 8601 representation.
//----------------------------------------------------------------------------

ts::Time ts::ISOTime::TimeFromISO(const UString& str)
{
    // Elements to collect.
    Time::Fields fields;

    // Collect date fields.
    static const UString end_date(u"tT");
    size_t count = 0;
    size_t pos = 0;
    size_t width = 0;
    if (IntFromString(str, pos, end_date, 4, fields.year, width)) {
        count++;
        if (IntFromString(str, pos, end_date, 2, fields.month, width)) {
            count++;
            if (IntFromString(str, pos, end_date, 2, fields.day, width)) {
                count++;
            }
        }
    }

    // Fill missing date fields with current UTC time.
    if (count < 3) {
        // Fetch current UTC *only* if some fields are missing.
        const Time::Fields now(Time::CurrentUTC());
        fields.day = now.day;
        if (count < 2) {
            fields.month = now.month;
            if (count < 1) {
                fields.year = now.year;
            }
        }
    }

    // Analyze the time part, it there is one.
    if (pos < str.length() && end_date.contains(str[pos])) {
        pos++; // skip the 'T' delimiter.
        static const UString end_time(u".,+-" + UString::Range('A', 'Z') + UString::Range('a', 'z'));
        if (IntFromString(str, pos, end_time, 2, fields.hour, width) && IntFromString(str, pos, end_time, 2, fields.minute, width)) {
            IntFromString(str, pos, end_time, 2, fields.second, width);
        }
        if (pos < str.length() && (str[pos] == '.' || str[pos] == ',')) {
            pos++;
            if (IntFromString(str, pos, end_time, 3, fields.millisecond, width)) {
                while (width++ < 3) {
                    fields.millisecond *= 10;
                }
            }
        }
    }

    // Analyze the UTC offset.
    cn::minutes utc_offset = cn::minutes::zero();
    if (pos < str.length()) {
        if (str[pos] == 'z' || str[pos] == 'Z') {
            pos++; // skip the Zule timezone indicator
        }
        else if (str[pos] == '+' || str[pos] == '-') {
            int sign = str[pos++] == '-' ? -1 : 1;
            int off_hour = 0;
            int off_minute = 0;
            static const UString empty;
            IntFromString(str, pos, empty, 2, off_hour, width);
            IntFromString(str, pos, empty, 2, off_minute, width);
            utc_offset = cn::minutes(sign * ((60 * off_hour) + off_minute));
        }
    }

    // Check that the string is fully consumed and all provided fields are correct.
    if (pos < str.length() || !fields.isValid()) {
        return Time::Epoch;
    }

    // Build the time value.
    try {
        Time result(fields);
        if (result != Time::Epoch) {
            result += utc_offset;
        }
        return result;
    }
    catch (...) {
        return Time::Epoch;
    }
}


//----------------------------------------------------------------------------
// Format the time in ISO 8601 representation, including offset from UTC time.
//----------------------------------------------------------------------------

ts::UString ts::ISOTime::ToIsoWithMinutes(const Time& time, intmax_t utc_offset)
{
    const Time::Fields f(time);
    UString s;
    s.format(u"%04d-%02d-%02dT%02d:%02d:%02d", f.year, f.month, f.day, f.hour, f.minute, f.second);
    if (f.millisecond > 0) {
        s.format(u".%03d", f.millisecond);
    }
    if (utc_offset == 0) {
        s += u'Z';
    }
    else {
        s += utc_offset < 0 ? u'-' : u'+';
        utc_offset = std::abs(utc_offset) % (24 * 60);
        if (utc_offset % 60 == 0) {
            s.format(u"%02d", utc_offset / 60);
        }
        else {
            s.format(u"%02d:%02d", utc_offset / 60, utc_offset % 60);
        }
    }
    return s;
}


//----------------------------------------------------------------------------
// Format a duration in ISO 8601 representation.
//----------------------------------------------------------------------------

ts::UString ts::ISOTime::MillisecondsToIso(cn::milliseconds::rep ms)
{
    // Not an integral number of months per year -> specific computation of the "days" field.
    const cn::milliseconds::rep years = ms / MS_PER_YEAR;
    const cn::milliseconds::rep months = (ms % MS_PER_YEAR) / MS_PER_MONTH;
    const cn::milliseconds::rep days = ((ms % MS_PER_YEAR) - (months * MS_PER_MONTH)) / MS_PER_DAY;
    const cn::milliseconds::rep seconds = (ms % MS_PER_DAY) / 1000;

    UString s;
    s.format(u"P%dY%dM%dDT%dH%dM%d", years, months, days, seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    if (ms % 1000 != 0) {
        s.format(u".%03d", ms % 1000);
    }
    s.append(u'S');
    return s;
}


//----------------------------------------------------------------------------
// Decode a duration from an ISO 8601 representation.
//----------------------------------------------------------------------------

cn::milliseconds ts::ISOTime::DurationFromISO(const UString& str)
{
    // A duration string must start with a 'P'.
    static const cn::milliseconds invalid(-1);
    if (!IsDurationString(str)) {
        return invalid;
    }

    cn::milliseconds::rep result = 0;
    const size_t end = str.length();
    size_t pos = 1;       // start after initial 'P'
    bool in_time = false; // true after 'T', in hour/minutes/seconds

    // Loop on integ[.dec] fields.
    while (pos < end) {
        cn::milliseconds::rep integ = 0;
        cn::milliseconds::rep dec = 0;
        size_t dec_width = 0;

        // Switch to time part after 'T'.
        if (!in_time && (str[pos] == 't' || str[pos] == 'T')) {
            in_time = true;
            pos++;
        }

        // Read integ. There must be at least one character after it.
        size_t start = pos;
        while (pos < end && IsDigit(str[pos])) {
            pos++;
        }
        if (pos >= end || pos == start || !str.substr(start, pos - start).toInteger(integ)) {
            return invalid;
        }

        // Read [.dec]. ISO 8601 says that a comma can be used as well.
        if (pos < end && (str[pos] == '.' || str[pos] == ',')) {
            start = ++pos;
            while (pos < end && IsDigit(str[pos])) {
                pos++;
            }
            if (pos >= end || pos == start || !str.substr(start, pos - start).toInteger(dec)) {
                return invalid;
            }
            dec_width = pos - start;
        }

        // Get the designator.
        assert(pos < end);
        cn::milliseconds::rep factor = 0;
        switch (ToUpper(str[pos++])) {
            case 'Y':
                factor = MS_PER_YEAR;
                break;
            case 'M':
                if (in_time) {
                    factor = 60'000; // minutes
                }
                else {
                    factor = MS_PER_MONTH;
                }
                break;
            case 'W':
                factor = MS_PER_WEEK;
                break;
            case 'D':
                factor = MS_PER_DAY;
                break;
            case 'H':
                factor = 3'600'000;
                break;
            case 'S':
                factor = 1'000;
                break;
            default:
                return invalid;
        }

        // Apply the designator factor on the integral part.
        result += integ * factor;

        // Process the optional decimal part.
        if (dec > 0) {
            cn::milliseconds::rep dec_factor = cn::milliseconds::rep(Power10(dec_width));
            if (dec_factor > 0 && dec_factor <= factor) {
                result += (dec * factor) / dec_factor;
            }
        }
    }

    return cn::milliseconds(result);
 }
