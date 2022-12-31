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

#include "tsFileNameGenerator.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::FileNameGenerator::FileNameGenerator() :
    _name_prefix(),
    _name_suffix(),
    _counter_mode(true),
    _counter_value(0),
    _counter_width(6),
    _time_fields(Time::DATETIME),
    _last_time()
{
}


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
// Fix name prefix, make sure it ends with '-' or any punctuation.
//----------------------------------------------------------------------------

void ts::FileNameGenerator::fixNamePrefix()
{
    if (!_name_prefix.empty()) {
        const UChar c = _name_prefix.back();
        if (c != u'-' && c != u'_' && c != u'.') {
            _name_prefix.append(u'-');
        }
    }
}


//----------------------------------------------------------------------------
// Reinitialize the file name generator in counter mode.
//----------------------------------------------------------------------------

void ts::FileNameGenerator::initCounter(const UString& name_template, size_t initial_counter, size_t counter_width)
{
    _counter_mode = true;

    // Analyze the file name template to isolate segments.
    _name_prefix = PathPrefix(name_template);
    _name_suffix = PathSuffix(name_template);

    // Compute number of existing digits at end of template head.
    _counter_width = TrailingDigits(_name_prefix);
    if (_counter_width == 0) {
        // No pre-existing integer field at end of file name. Use user-defined values.
        _counter_value = initial_counter;
        _counter_width = std::max<size_t>(1, counter_width);
        fixNamePrefix();
    }
    else {
        // Use existing integer field as initial value.
        const size_t len = _name_prefix.length();
        _name_prefix.substr(len - _counter_width).toInteger(_counter_value);
        _name_prefix.resize(len - _counter_width);
    }
}


//----------------------------------------------------------------------------
// Reinitialize the file name generator in date and time mode.
//----------------------------------------------------------------------------

void ts::FileNameGenerator::initDateTime(const UString& name_template, int fields)
{
    _counter_mode = false;
    _last_time.clear();

    // Analyze the file name template to isolate segments.
    _name_prefix = PathPrefix(name_template);
    _name_suffix = PathSuffix(name_template);

    // Attempt to locate date-time at end of prefix.
    size_t date_len = 0;
    size_t time_len = TrailingDigits(_name_prefix);
    if (time_len == 0) {
        // No pre-existing date-time at end of file name. Use user-defined values.
        _time_fields = fields == 0 ? Time::DATETIME : fields;
        fixNamePrefix();
    }
    else {
        // Check if there is another field.
        const size_t len = _name_prefix.length();
        size_t field_len = time_len;
        if (time_len < len && _name_prefix[len - time_len - 1] == u'-') {
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

ts::UString ts::FileNameGenerator::newFileName()
{
    if (_counter_mode) {
        return UString::Format(u"%s%0*d%s", {_name_prefix, _counter_width, _counter_value++, _name_suffix});
    }
    else {
        return newFileName(Time::CurrentLocalTime());
    }
}


//----------------------------------------------------------------------------
// Generate a new file name with a specific date and time.
//----------------------------------------------------------------------------

ts::UString ts::FileNameGenerator::newFileName(const Time& time)
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

    return _name_prefix + str + _name_suffix;
}
