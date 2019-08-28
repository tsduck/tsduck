//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsPlatform.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Memory access with strict alignment.
//----------------------------------------------------------------------------

#if defined(TS_STRICT_MEMORY_ALIGN)

uint16_t ts::GetUInt16BE(const void* p)
{
    uint16_t value = *(static_cast<const uint8_t*>(p)) << 8;
    value |= *(static_cast<const uint8_t*>(p) + 1);
    return value;
}

uint32_t ts::GetUInt24BE(const void* p)
{
    uint32_t value = *(static_cast<const uint8_t*>(p)) << 16;
    value |= *(static_cast<const uint8_t*>(p) + 1) << 8;
    value |= *(static_cast<const uint8_t*>(p) + 2);
    return value;
}

uint32_t ts::GetUInt32BE(const void* p)
{
    uint32_t value = *(static_cast<const uint8_t*>(p)) << 24;
    value |= *(static_cast<const uint8_t*>(p) + 1) << 16;
    value |= *(static_cast<const uint8_t*>(p) + 2) << 8;
    value |= *(static_cast<const uint8_t*>(p) + 3);
    return value;
}

uint64_t ts::GetUInt40BE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p))) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4));
    return value;
}

uint64_t ts::GetUInt48BE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p))) << 40;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5));
    return value;
}

uint64_t ts::GetUInt64BE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p))) << 56;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 48;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 40;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 6)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 7));
    return value;
}

uint16_t ts::GetUInt16LE(const void* p)
{
    uint16_t value = *(static_cast<const uint8_t*>(p) + 1) << 8;
    value |= *(static_cast<const uint8_t*>(p));
    return value;
}

uint32_t ts::GetUInt24LE(const void* p)
{
    uint32_t value = *(static_cast<const uint8_t*>(p) + 2) << 16;
    value |= *(static_cast<const uint8_t*>(p) + 1) << 8;
    value |= *(static_cast<const uint8_t*>(p));
    return value;
}

uint32_t ts::GetUInt32LE(const void* p)
{
    uint32_t value = *(static_cast<const uint8_t*>(p) + 3) << 24;
    value |= *(static_cast<const uint8_t*>(p) + 2) << 16;
    value |= *(static_cast<const uint8_t*>(p) + 1) << 8;
    value |= *(static_cast<const uint8_t*>(p));
    return value;
}

uint64_t ts::GetUInt40LE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p)));
    return value;
}

uint64_t ts::GetUInt48LE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5)) << 40;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p)));
    return value;
}

uint64_t ts::GetUInt64LE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 7)) << 56;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 6)) << 48;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5)) << 40;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p)));
    return value;
}

void ts::PutUInt16BE(void* p, uint16_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 8);
    data[1] = static_cast<uint8_t>(i);
}

void ts::PutUInt24BE(void* p, uint32_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 16);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i);
}

void ts::PutUInt32BE(void* p, uint32_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 24);
    data[1] = static_cast<uint8_t>(i >> 16);
    data[2] = static_cast<uint8_t>(i >> 8);
    data[3] = static_cast<uint8_t>(i);
}

void ts::PutUInt40BE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 32);
    data[1] = static_cast<uint8_t>(i >> 24);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 8);
    data[4] = static_cast<uint8_t>(i);
}

void ts::PutUInt48BE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 40);
    data[1] = static_cast<uint8_t>(i >> 32);
    data[2] = static_cast<uint8_t>(i >> 24);
    data[3] = static_cast<uint8_t>(i >> 16);
    data[4] = static_cast<uint8_t>(i >> 8);
    data[5] = static_cast<uint8_t>(i);
}

void ts::PutUInt64BE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 56);
    data[1] = static_cast<uint8_t>(i >> 48);
    data[2] = static_cast<uint8_t>(i >> 40);
    data[3] = static_cast<uint8_t>(i >> 32);
    data[4] = static_cast<uint8_t>(i >> 24);
    data[5] = static_cast<uint8_t>(i >> 16);
    data[6] = static_cast<uint8_t>(i >> 8);
    data[7] = static_cast<uint8_t>(i);
}

void ts::PutUInt16LE(void* p, uint16_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
}

void ts::PutUInt24LE(void* p, uint32_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
}

void ts::PutUInt32LE(void* p, uint32_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 24);
}

void ts::PutUInt40LE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 24);
    data[4] = static_cast<uint8_t>(i >> 32);
}

void ts::PutUInt48LE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 24);
    data[4] = static_cast<uint8_t>(i >> 32);
    data[5] = static_cast<uint8_t>(i >> 40);
}

void ts::PutUInt64LE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 24);
    data[4] = static_cast<uint8_t>(i >> 32);
    data[5] = static_cast<uint8_t>(i >> 40);
    data[6] = static_cast<uint8_t>(i >> 48);
    data[7] = static_cast<uint8_t>(i >> 56);
}

#endif
