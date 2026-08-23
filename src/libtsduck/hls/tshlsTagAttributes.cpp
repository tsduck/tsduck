//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsTagAttributes.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::TagAttributes::TagAttributes(const UString& params)
{
    reload(params);
}


//----------------------------------------------------------------------------
// Reload the contents of the attributes.
//----------------------------------------------------------------------------

void ts::hls::TagAttributes::reload(const UString& params)
{
    _map.clear();

    // Parse the line field by field. We can't just split on commas because
    // a value can be a quoted string containing a comma.
    size_t pos = 0;
    const size_t end = params.size();

    // Loop on all attributes.
    while (pos < end) {

        // Locate the name.
        const size_t name_start = pos;
        while (pos < end && params[pos] != u'=' && params[pos] != u',') {
            ++pos;
        }
        const size_t name_size = pos - name_start;

        // Locate the value.
        size_t value_start = pos;
        size_t value_size = 0;
        if (pos < end && params[pos] == u'=') {
            // There is a value. Skip '='.
            ++value_start;
            ++pos;
            // Check if the value is a quoted string.
            const bool quoted = pos < end && params[pos] == u'"';
            if (quoted) {
                // Skip '"'
                ++value_start;
                ++pos;
            }
            // Locate end of value.
            while (pos < end && ((quoted && params[pos] != u'"') || (!quoted && params[pos] != u','))) {
                ++pos;
            }
            value_size = pos - value_start;
            // Skip closing sequence.
            if (pos < end && quoted && params[pos] == u'"') {
                ++pos;
            }
            while (pos < end && params[pos] != u',') {
                ++pos;
            }
            while (pos < end && params[pos] == u',') {
                ++pos;
            }
        }

        // Register the attribute.
        if (name_size > 0) {
            _map[params.substr(name_start, name_size)] = params.substr(value_start, value_size);
        }
    }
}


//----------------------------------------------------------------------------
// Get attribute.
//----------------------------------------------------------------------------

bool ts::hls::TagAttributes::present(const ts::UString& name) const
{
    return _map.contains(name);
}

ts::UString ts::hls::TagAttributes::value(const UString& name, const UString& def_value) const
{
    auto it = _map.find(name);
    return it == _map.end() ? def_value : it->second;
}
