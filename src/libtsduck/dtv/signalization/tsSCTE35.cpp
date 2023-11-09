//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSCTE35.h"
#include "tsTS.h"


//----------------------------------------------------------------------------
// SpliceTime methods
//----------------------------------------------------------------------------

// Convert the SpliceTime structure to string.
ts::UString ts::SpliceTime::toString() const
{
    return has_value() ? PTSToString(value()) : u"unset";
}

// Deserialize a SpliceTime structure from binary data.
int ts::SpliceTime::deserialize(const uint8_t* data, size_t size)
{
    if (size < 1) {
        return -1; // too short.
    }
    else if ((data[0] & 0x80) == 0) {
        reset();
        return 1;
    }
    else if (size < 5) {
        return -1; // too short.
    }
    else {
        *this = (uint64_t(data[0] & 0x01) << 32) | uint64_t(GetUInt32(data + 1));
        return 5;
    }
}

// Serialize the SpliceTime structure.
void ts::SpliceTime::serialize(ByteBlock& data) const
{
    if (has_value()) {
        data.appendUInt8(0xFE | uint8_t(value() >> 32));
        data.appendUInt32(uint32_t(value()));
    }
    else {
        data.appendUInt8(0x7F);
    }
}
