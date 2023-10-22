//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsTagAttributes.h"
#include "tsAlgorithm.h"


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
        const size_t nameStart = pos;
        while (pos < end && params[pos] != u'=' && params[pos] != u',') {
            ++pos;
        }
        const size_t nameSize = pos - nameStart;

        // Locate the value.
        size_t valueStart = pos;
        size_t valueSize = 0;
        if (pos < end && params[pos] == u'=') {
            // There is a value. Skip '='.
            ++valueStart;
            ++pos;
            // Check if the value is a quoted string.
            const bool quoted = pos < end && params[pos] == u'"';
            if (quoted) {
                // Skip '"'
                ++valueStart;
                ++pos;
            }
            // Locate end of value.
            while (pos < end && ((quoted && params[pos] != u'"') || (!quoted && params[pos] != u','))) {
                ++pos;
            }
            valueSize = pos - valueStart;
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
        if (nameSize > 0) {
            _map[params.substr(nameStart, nameSize)] = params.substr(valueStart, valueSize);
        }
    }
}


//----------------------------------------------------------------------------
// Get attribute.
//----------------------------------------------------------------------------

bool ts::hls::TagAttributes::present(const ts::UString& name) const
{
    return Contains(_map, name);
}

ts::UString ts::hls::TagAttributes::value(const UString& name, const UString& defValue) const
{
    auto it = _map.find(name);
    return it == _map.end() ? defValue : it->second;
}
