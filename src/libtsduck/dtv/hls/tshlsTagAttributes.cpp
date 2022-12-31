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

#include "tshlsTagAttributes.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::hls::TagAttributes::TagAttributes(const UString& params) :
    _map()
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
