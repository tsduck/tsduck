//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFileNameGenerator.h"


//----------------------------------------------------------------------------
// Get the number of trailing digits in a string.
//----------------------------------------------------------------------------

size_t ts::FileNameGenerator::TrailingDigits(const UString& str)
{
    size_t count = 0;
    const size_t len = str.length();
    while (count < len && IsDigit(str[len - 1 - count])) {
        count++;
    }
    return count;
}


//----------------------------------------------------------------------------
// Initialize the name prefix and suffix.
// Return the number of trailing digits in prefix.
//----------------------------------------------------------------------------

size_t ts::FileNameGenerator::init(const fs::path& name_template)
{
    // Analyze the file name template to isolate segments.
    fs::path prefix(name_template);
    prefix.replace_extension();
    _name_prefix = prefix;
    _name_suffix = name_template.extension();

    // Compute number of existing digits at end of template head.
    const size_t width = TrailingDigits(_name_prefix);

    // If no pre-existing integer field at end of file name, make sure to add a punctuation.
    if (width == 0 && !_name_prefix.empty()) {
        const UChar c = _name_prefix.back();
        if (c != u'-' && c != u'_' && c != u'.' && c != u'/' && c != u'\\') {
            _name_prefix += u"-";
        }
    }

    return width;
}


//----------------------------------------------------------------------------
// Reinitialize the file name generator in counter mode.
//----------------------------------------------------------------------------

void ts::FileNameGenerator::initCounter(const fs::path& name_template, size_t initial_counter, size_t counter_width)
{
    _counter_mode = true;
    _counter_value = initial_counter;
    _counter_width = std::max<size_t>(1, counter_width);

    const size_t width = init(name_template);

    if (width > 0) {
        // Use existing integer field as initial value.
        _counter_width = width;
        const size_t len = _name_prefix.length();
        _name_prefix.substr(len - width).toInteger(_counter_value);
        _name_prefix.resize(len - width);
    }
}


//----------------------------------------------------------------------------
// Reinitialize the file name generator in date and time mode.
//----------------------------------------------------------------------------

void ts::FileNameGenerator::initDateTime(const fs::path& name_template, int fields)
{
    _counter_mode = false;
    _time_fields = fields == 0 ? Time::DATETIME : fields;
    _last_time.clear();

    size_t time_len = init(name_template);

    if (time_len > 0) {
        // Locate [date-]time fields at end of prefix.
        // Check if there is another field.
        const size_t len = _name_prefix.length();
        size_t date_len = 0;
        size_t field_len = time_len;
        if (time_len < len && _name_prefix[len - time_len - 1] == u'-') {
            // The prefix ends in "-time", maybe there is a preceding date.
            date_len = TrailingDigits(_name_prefix.substr(0, len - time_len - 1));
            if (date_len == 0) {
                // Only one field, this is a date field.
                date_len = time_len;
                time_len = 0;
            }
            else {
                // Two fields: date-time.
                field_len = date_len + 1 + time_len;
            }
        }
        // Truncate pre-existing suffix, do not reuse it.
        _name_prefix.resize(len - field_len);
        // Determine the list of date/time fields based on the size, keeping most significant.
        _time_fields = 0;
        if (date_len > 0) {
            _time_fields |= Time::YEAR;
        }
        if (date_len > 4) {
            _time_fields |= Time::MONTH;
        }
        if (date_len > 6) {
            _time_fields |= Time::DAY;
        }
        if (time_len > 0) {
            _time_fields |= Time::HOUR;
        }
        if (time_len > 2) {
            _time_fields |= Time::MINUTE;
        }
        if (time_len > 4) {
            _time_fields |= Time::SECOND;
        }
        if (time_len > 6) {
            _time_fields |= Time::MILLISECOND;
        }
    }
}


//----------------------------------------------------------------------------
// Generate a new file name.
//----------------------------------------------------------------------------

fs::path ts::FileNameGenerator::newFileName()
{
    if (_counter_mode) {
        return fs::path(UString::Format(u"%s%0*d%s", {_name_prefix, _counter_width, _counter_value++, _name_suffix}));
    }
    else {
        return newFileName(Time::CurrentLocalTime());
    }
}


//----------------------------------------------------------------------------
// Generate a new file name with a specific date and time.
//----------------------------------------------------------------------------

fs::path ts::FileNameGenerator::newFileName(const Time& time)
{
    // In counter mode, use the other version.
    if (_counter_mode) {
        return newFileName();
    }

    // Format the date-time fields.
    UString str;
    Time::Fields fields(time);
    if ((_time_fields & Time::YEAR) != 0) {
        str.format(u"%04d", {fields.year});
    }
    if ((_time_fields & Time::MONTH) != 0) {
        str.format(u"%02d", {fields.month});
    }
    if ((_time_fields & Time::DAY) != 0) {
        str.format(u"%02d", {fields.day});
    }
    if ((_time_fields & Time::DATE) != 0 && (_time_fields & (Time::TIME | Time::MILLISECOND)) != 0) {
        str.append(u'-');
    }
    if ((_time_fields & Time::HOUR) != 0) {
        str.format(u"%02d", {fields.hour});
    }
    if ((_time_fields & Time::MINUTE) != 0) {
        str.format(u"%02d", {fields.minute});
    }
    if ((_time_fields & Time::SECOND) != 0) {
        str.format(u"%02d", {fields.second});
    }
    if ((_time_fields & Time::MILLISECOND) != 0) {
        str.format(u"%03d", {fields.millisecond});
    }

    // Avoid duplicates.
    if (str != _last_time) {
        _last_time = str;
        _counter_value = 1;
    }
    else {
        str.format(u"-%d", {_counter_value++});
    }

    return fs::path(_name_prefix + str + _name_suffix);
}
