//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------------------
// Get the multiple values of a property in the buffer.
//-----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
void ts::DTVProperties::getValuesByCommand(std::set<INT>& values, uint32_t cmd) const
{
    values.clear();
    for (size_t i = 0; i < size_t(_prop_head.num); i++) {
        if (_prop_buffer[i].cmd == cmd) {
            getValuesByIndex(values, i);
            break;
        }
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value || std::is_enum<INT>::value>::type*>
void ts::DTVProperties::getValuesByIndex(std::set<INT>& values, size_t index) const
{
    values.clear();
    if (index < size_t(_prop_head.num)) {
        assert(sizeof(_prop_buffer[index].u.buffer.data[0]) == 1);
        const size_t count = std::min<size_t>(sizeof(_prop_buffer[index].u.buffer.data), _prop_buffer[index].u.buffer.len);
        for (size_t i = 0; i < count; ++i) {
            values.insert(INT(_prop_buffer[index].u.buffer.data[i]));
        }
    }
}
