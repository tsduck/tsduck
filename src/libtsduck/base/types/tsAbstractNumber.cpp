//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractNumber.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Virtual destructor.
//----------------------------------------------------------------------------

ts::AbstractNumber::~AbstractNumber()
{
}


//----------------------------------------------------------------------------
// Pointer to an invalid instance of AbstractNumber.
//----------------------------------------------------------------------------

namespace {
    class InvalidNumber: public ts::AbstractNumber
    {
    public:
        InvalidNumber() {}
        virtual ts::UString description() const override;
        virtual bool inRange(int64_t min, int64_t max) const override;
        virtual int64_t toInt64() const override;
        virtual double toDouble() const override;
        virtual ts::UString toString(size_t, bool, ts::UChar, bool, size_t, bool, ts::UChar, ts::UChar) const override;
        virtual bool fromString(const ts::UString&, ts::UChar, ts::UChar) override;
    };
    ts::UString InvalidNumber::description() const
    {
        return ts::UString();
    }
    bool InvalidNumber::inRange(int64_t min, int64_t max) const
    {
        return false;
    }
    int64_t InvalidNumber::toInt64() const
    {
        return 0;
    }
    double InvalidNumber::toDouble() const
    {
        return 0.0;
    }
    ts::UString InvalidNumber::toString(size_t, bool, ts::UChar, bool, size_t, bool, ts::UChar, ts::UChar) const
    {
        return ts::UString();
    }
    bool InvalidNumber::fromString(const ts::UString&, ts::UChar, ts::UChar)
    {
        return false;
    }
    const InvalidNumber _invalid;
}

const ts::AbstractNumber* const ts::AbstractNumber::INVALID = &_invalid;


//----------------------------------------------------------------------------
// Static common utility to format a string containing a number.
//----------------------------------------------------------------------------

void ts::AbstractNumber::Format(UString& str,
                                size_t min_width,
                                bool right_justified,
                                UChar separator,
                                bool add_plus_sign,
                                size_t decimals,
                                bool force_decimals,
                                UChar decimal_dot,
                                UChar pad)
{
    // Apply decimal dot translation.
    size_t decimal_pos = str.find(u'.');
    if (decimal_dot != u'.' && decimal_pos != NPOS) {
        str[decimal_pos] = decimal_dot;
    }

    // Apply decimal part formatting.
    if (force_decimals) {
        if (decimals == 0) {
            // Remove the decimal part.
            if (decimal_pos != NPOS) {
                str.resize(decimal_pos);
                decimal_pos = NPOS;
            }
        }
        else {
            // Force a trailing decimal digit.
            if (decimal_pos == NPOS) {
                decimal_pos = str.length();
                str.append(decimal_dot);
            }
            // Pad or truncate decimal part.
            const size_t str_len = str.length();
            const size_t decimal_len = str_len - decimal_pos - 1;
            if (decimal_len < decimals) {
                str.insert(str_len, decimals - decimal_len, u'0');
            }
            else if (decimal_len > decimals) {
                str.resize(str_len - (decimal_len - decimals));
            }
        }
    }
    else if (decimal_pos != NPOS) {
        // Decimals are not forced and there is a decimal part, remove trailing decimal zeroes.
        while (str.length() > 0 && str.back() == u'0') {
            str.pop_back();
        }
        if (str.length() == decimal_pos + 1) {
            // There is nothing after the decimal dot, remove it.
            str.pop_back();
            decimal_pos = NPOS;
        }
    }

    // Apply thousands separators.
    if (separator != CHAR_NULL) {
        size_t end = str.length();
        // Apply separators on decimal part.
        if (decimal_pos != NPOS) {
            end = decimal_pos;
            for (size_t i = decimal_pos + 4; i < str.length(); i += 4) {
                str.insert(i, 1, separator);
            }
        }
        // Apply separators on integer part. Take care that first character can be '-'.
        const size_t first = str.empty() || IsDigit(str.front()) ? 3 : 4;
        for (size_t i = end; i > first; i -= 3) {
            str.insert(i - 3, 1, separator);
        }
    }

    // Apply forced sign.
    if (add_plus_sign) {
        str.insert(0, 1, u'+');
    }

    // Pad to minimum width.
    const size_t str_len = str.length();
    if (str_len < min_width) {
        str.insert(right_justified ? 0 : str_len, min_width - str_len, pad);
    }
}


//----------------------------------------------------------------------------
// Static common utility to deformat a string containing a number.
//----------------------------------------------------------------------------

void ts::AbstractNumber::Deformat(UString& str, UChar separator, UChar decimal_dot)
{
    str.trim();
    str.remove(separator);
    str.substitute(decimal_dot, u'.');
}
