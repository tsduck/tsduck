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

#include "tsSCTE35.h"
#include "tsTS.h"


//----------------------------------------------------------------------------
// SpliceTime methods
//----------------------------------------------------------------------------

// Virtual destructor.
ts::SpliceTime::~SpliceTime()
{
}

// Convert the SpliceTime structure to string.
ts::UString ts::SpliceTime::toString() const
{
    return set() ? PTSToString(value()) : u"unset";
}

// Deserialize a SpliceTime structure from binary data.
int ts::SpliceTime::deserialize(const uint8_t* data, size_t size)
{
    if (size < 1) {
        return -1; // too short.
    }
    else if ((data[0] & 0x80) == 0) {
        clear();
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
    if (set()) {
        data.appendUInt8(0xFE | uint8_t(value() >> 32));
        data.appendUInt32(uint32_t(value()));
    }
    else {
        data.appendUInt8(0x7F);
    }
}


//----------------------------------------------------------------------------
// SplicePrivateCommand methods
//----------------------------------------------------------------------------

ts::SplicePrivateCommand::SplicePrivateCommand(uint32_t id) :
    identifier(id),
    private_bytes()
{
}
