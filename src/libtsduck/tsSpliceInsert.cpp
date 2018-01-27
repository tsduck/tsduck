//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsSpliceInsert.h"
#include "tsTablesDisplay.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::SpliceInsert::SpliceInsert() :
    event_id(0),
    canceled(true),
    splice_out(false),
    immediate(false),
    program_splice(false),
    use_duration(false),
    program_pts(~0),
    components_pts(),
    duration_pts(~0),
    auto_return(false),
    program_id(0),
    avail_num(0),
    avails_expected(0)
{
}


//----------------------------------------------------------------------------
// Reset all fields to default initial values.
//----------------------------------------------------------------------------

void ts::SpliceInsert::clear()
{
    event_id = 0;
    canceled = true;
    splice_out = false;
    immediate = false;
    program_splice = false;
    use_duration = false;
    program_pts = ~0;
    components_pts.clear();
    duration_pts = ~0;
    auto_return = false;
    program_id = 0;
    avail_num = 0;
    avails_expected = 0;
}


//----------------------------------------------------------------------------
// Adjust PTS time values using the "PTS adjustment" field from a splice
// information section.
//----------------------------------------------------------------------------

void ts::SpliceInsert::adjustPTS(uint64_t adjustment)
{
    // Ignore null or invalid adjustment. And cancelation or immediate commands have no time.
    if (adjustment == 0 || adjustment > PTS_DTS_MASK || canceled || immediate) {
        return;
    }

    // Adjust program splice time.
    if (program_splice && program_pts <= PTS_DTS_MASK) {
        program_pts = (program_pts + adjustment) & PTS_DTS_MASK;
    }

    // Adjust components splice times.
    if (!program_splice) {
        for (PTSByComponent::iterator it = components_pts.begin(); it != components_pts.end(); ++it) {
            if (it->second <= PTS_DTS_MASK) {
                it->second = (it->second + adjustment) & PTS_DTS_MASK;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Display a SpliceInsert command.
//----------------------------------------------------------------------------

void ts::SpliceInsert::display(TablesDisplay& display, int indent) const
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    strm << margin << UString::Format(u"Splice event id: 0x%X, cancel: %d", {event_id, canceled}) << std::endl;

    if (!canceled) {
        strm << margin
             << "Out of network: " << UString::YesNo(splice_out)
             << ", program splice: " << UString::YesNo(program_splice)
             << ", duration set: " << UString::YesNo(use_duration)
             << ", immediate: " << UString::YesNo(immediate)
             << std::endl;

        if (program_splice && !immediate) {
            // The complete program switches at a given time.
            strm << margin << UString::Format(u"Time PTS: 0x%09X (%d)", {program_pts, program_pts}) << std::endl;
        }
        if (!program_splice) {
            // Program components switch individually.
            strm << margin << "Number of components: " << components_pts.size() << std::endl;
            for (PTSByComponent::const_iterator it = components_pts.begin(); it != components_pts.end(); ++it) {
                strm << margin << UString::Format(u"  Component tag: 0x%X (%d)", {it->first, it->first});
                if (!immediate) {
                    strm << UString::Format(u", time PTS: 0x%09X (%d)", {it->second, it->second});
                }
                strm << std::endl;
            }
        }
        if (use_duration) {
            strm << margin << UString::Format(u"Duration PTS: 0x%09X (%d), auto return: %s", {duration_pts, duration_pts, UString::YesNo(auto_return)}) << std::endl;
        }
        strm << margin << UString::Format(u"Unique program id: 0x%X (%d), avail: 0x%X (%d), avails expected: %d", {program_id, program_id, avail_num, avails_expected, avails_expected}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Deserialize a SpliceInsert command from binary data.
//----------------------------------------------------------------------------

int ts::SpliceInsert::deserialize(const uint8_t* data, size_t size)
{
    const uint8_t* const start = data;
    clear();
    if (size < 5) {
        return -1; // too short
    }

    event_id = GetUInt32(data);
    canceled = (data[4] & 0x80) != 0;
    data += 5; size -= 5;

    if (canceled) {
        return data - start;  // end of command
    }
    if (size < 1) {
        return -1; // too short
    }

    splice_out = (data[0] & 0x80) != 0;
    program_splice = (data[0] & 0x40) != 0;
    use_duration = (data[0] & 0x20) != 0;
    immediate = (data[0] & 0x10) != 0;
    data++; size--;

    if (program_splice && !immediate) {
        // The complete program switches at a given time.
        if (!GetSpliceTime(program_pts, data, size)) {
            return -1; // invalid
        }
    }
    if (!program_splice) {
        // Program components switch individually.
        if (size < 1) {
            return -1; // too short
        }
        size_t count = data[0];
        data++; size--;
        while (count-- > 0) {
            if (size < 1) {
                return -1; // too short
            }
            const uint8_t ctag = data[0];
            uint64_t pts = ~0;
            data++; size--;
            if (!immediate && !GetSpliceTime(pts, data, size)) {
                return -1; // invalid
            }
            components_pts.insert(std::make_pair(ctag, pts));
        }
    }
    if (use_duration) {
        if (size < 5) {
            return -1; // too short
        }
        auto_return = (data[0] & 0x80) != 0;
        duration_pts = (uint64_t(data[0] & 0x01) << 32) | uint64_t(GetUInt32(data + 1));
        data += 5; size -= 5;
    }
    if (size < 4) {
        return -1; // too short
    }
    program_id = GetUInt16(data);
    avail_num = data[2];
    avails_expected = data[3];
    data+= 4; size -= 4;
    return data - start;
}


//----------------------------------------------------------------------------
// Get a splice_time structure, skip the data area.
//----------------------------------------------------------------------------

bool ts::SpliceInsert::GetSpliceTime(uint64_t& pts, const uint8_t*& data, size_t& size)
{
    if (size >= 1 && (data[0] & 0x80) == 0) {
        pts = ~0; // unspecified PTS value
        data++; size--;
        return true;
    }
    else if (size >= 5 && (data[0] & 0x80) != 0) {
        pts = (uint64_t(data[0] & 0x01) << 32) | uint64_t(GetUInt32(data + 1));
        data += 5; size -= 5;
        return true;
    }
    else {
        return false; // invalid
    }
}


//----------------------------------------------------------------------------
// Serialize the SpliceInsert command.
//----------------------------------------------------------------------------

void ts::SpliceInsert::serialize(ByteBlock& data) const
{
    data.appendUInt32(event_id);
    data.appendUInt8(canceled ? 0xFF : 0x7F);

    if (!canceled) {
        data.appendUInt8((splice_out ? 0x80 : 0x00) |
                         (program_splice ? 0x40 : 0x00) |
                         (use_duration ? 0x20 : 0x00) |
                         (immediate ? 0x10 : 0x00) |
                         0x0F);
        if (program_splice && !immediate) {
            data.appendUInt8(0xFE | uint8_t(program_pts >> 32));
            data.appendUInt32(uint32_t(program_pts));
        }
        if (!program_splice) {
            data.appendUInt8(uint8_t(components_pts.size()));
            for (PTSByComponent::const_iterator it = components_pts.begin(); it != components_pts.end(); ++it) {
                data.appendUInt8(it->first);
                if (!immediate) {
                    data.appendUInt8(0xFE | uint8_t(it->second >> 32));
                    data.appendUInt32(uint32_t(it->second));
                }
            }
        }
        if (use_duration) {
            data.appendUInt8((auto_return ? 0xFE : 0x7E) | uint8_t(duration_pts >> 32));
            data.appendUInt32(uint32_t(duration_pts));
        }
        data.appendUInt16(program_id);
        data.appendUInt8(avail_num);
        data.appendUInt8(avails_expected);
    }
}
