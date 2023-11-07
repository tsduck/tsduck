//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMemory.h"


//----------------------------------------------------------------------------
// Check if a memory area starts with the specified prefix
//----------------------------------------------------------------------------

bool ts::StartsWith(const void* area, size_t area_size, const void* prefix, size_t prefix_size)
{
    if (prefix_size == 0 || area_size < prefix_size) {
        return false;
    }
    else {
        return std::memcmp(area, prefix, prefix_size) == 0;
    }
}


//----------------------------------------------------------------------------
// Locate a pattern into a memory area. Return 0 if not found
//----------------------------------------------------------------------------

const uint8_t* ts::LocatePattern(const void* area, size_t area_size, const void* pattern, size_t pattern_size)
{
    if (pattern_size > 0) {
        const uint8_t* a = reinterpret_cast<const uint8_t*>(area);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(pattern);
        while (area_size >= pattern_size) {
            if (*a == *p && std::memcmp(a, p, pattern_size) == 0) {
                return a;
            }
            ++a;
            --area_size;
        }
    }
    return nullptr; // not found
}


//----------------------------------------------------------------------------
// Check if a memory area contains all identical byte values.
//----------------------------------------------------------------------------

bool ts::IdenticalBytes(const void * area, size_t area_size)
{
    if (area_size < 2) {
        return false;
    }
    else {
        const uint8_t* d = reinterpret_cast<const uint8_t*>(area);
        for (size_t i = 0; i < area_size - 1; ++i) {
            if (d[i] != d[i + 1]) {
                return false;
            }
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Memory accesses with non-natural sizes
//----------------------------------------------------------------------------

#if !defined(TS_STRICT_MEMORY_ALIGN)

// 24 bits

uint32_t ts::GetUInt24BE(const void* p)
{
    return (static_cast<uint32_t>(GetUInt16BE(p)) << 8) | *(static_cast<const uint8_t*>(p) + 2);
}

uint32_t ts::GetUInt24LE(const void* p)
{
    return (static_cast<uint32_t>(*(static_cast<const uint8_t*>(p) + 2)) << 16) | GetUInt16LE(p);
}

void ts::PutUInt24BE(void* p, uint32_t i)
{
    *(static_cast<uint16_t*>(p)) = CondByteSwap16BE(static_cast<uint16_t>(i >> 8));
    *(static_cast<uint8_t*>(p) + 2) = static_cast<uint8_t>(i);
}

void ts::PutUInt24LE(void* p, uint32_t i)
{
    *(static_cast<uint16_t*>(p)) = CondByteSwap16LE(static_cast<uint16_t>(i));
    *(static_cast<uint8_t*>(p) + 2) = static_cast<uint8_t>(i >> 16);
}

// 40 bits

uint64_t ts::GetUInt40BE(const void* p)
{
    return (static_cast<uint64_t>(GetUInt32BE(p)) << 8) | *(static_cast<const uint8_t*>(p) + 4);
}

uint64_t ts::GetUInt40LE(const void* p)
{
    return (static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 32) | GetUInt32LE(p);
}

void ts::PutUInt40BE(void* p, uint64_t i)
{
    *(static_cast<uint8_t*>(p)) = static_cast<uint8_t>(i >> 32);
    *(reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + 1)) = CondByteSwap32BE(static_cast<uint32_t>(i));
}

void ts::PutUInt40LE(void* p, uint64_t i)
{
    *(static_cast<uint32_t*>(p)) = CondByteSwap32LE(static_cast<uint32_t>(i));
    *(static_cast<uint8_t*>(p) + 4) = static_cast<uint8_t>(i >> 32);
}

// 48 bits

uint64_t ts::GetUInt48BE(const void* p)
{
    return (static_cast<uint64_t>(GetUInt32BE(p)) << 16) | GetUInt16BE(static_cast<const uint8_t*>(p) + 4);
}

uint64_t ts::GetUInt48LE(const void* p)
{
    return (static_cast<uint64_t>(GetUInt16LE(static_cast<const uint8_t*>(p) + 4)) << 32) | GetUInt32LE(p);
}

void ts::PutUInt48BE(void* p, uint64_t i)
{
    *(static_cast<uint16_t*>(p)) = CondByteSwap16BE(static_cast<uint16_t>(i >> 32));
    *(reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + 2)) = CondByteSwap32BE(static_cast<uint32_t>(i));
}

void ts::PutUInt48LE(void* p, uint64_t i)
{
    *(static_cast<uint32_t*>(p)) = CondByteSwap32LE(static_cast<uint32_t>(i));
    *(reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + 4)) = CondByteSwap16LE(static_cast<uint16_t>(i >> 32));
}

#endif


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
