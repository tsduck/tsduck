//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMemory.h"


//----------------------------------------------------------------------------
// Check if a memory area starts with the specified prefix
//----------------------------------------------------------------------------

bool ts::StartsWith(const void* area, size_t area_size, const void* prefix, size_t prefix_size)
{
    return area_size >= prefix_size && MemEqual(area, prefix, prefix_size);
}


//----------------------------------------------------------------------------
// Locate a pattern into a memory area. Return 0 if not found
//----------------------------------------------------------------------------

const uint8_t* ts::LocatePattern(const void* area, size_t area_size, const void* pattern, size_t pattern_size)
{
    // Warning: this function emerged as a bottleneck in perfomance analysis on TSDuck.
    // Some optimizations have been added to improve this. Think twice before modifying it.
    if (pattern_size == 0) {
        return nullptr;
    }
    else if (pattern_size == 1) {
        const uint8_t val = *reinterpret_cast<const uint8_t*>(pattern);
        return reinterpret_cast<const uint8_t*>(std::memchr(area, val, area_size));
    }
    else {
        const uint8_t* a = reinterpret_cast<const uint8_t*>(area);
        const uint8_t* const p = reinterpret_cast<const uint8_t*>(pattern);
        const uint8_t* const p1 = p + 1;
        const size_t last = pattern_size - 1;
        const size_t sublen = pattern_size - 2;
        while (area_size >= pattern_size) {
            if (*a == *p && a[last] == p[last] && MemEqual(a + 1, p1, sublen)) {
                return a;
            }
            ++a;
            --area_size;
        }
        return nullptr; // not found
    }
}


//----------------------------------------------------------------------------
// Locate a 3-byte pattern 00 00 XY into a memory area.
//----------------------------------------------------------------------------

const uint8_t* ts::LocateZeroZero(const void* area, size_t area_size, uint8_t third)
{
    const uint8_t* a = reinterpret_cast<const uint8_t*>(area);
    while (area_size >= 3) {
        const uint8_t* next = reinterpret_cast<const uint8_t*>(std::memchr(a, 0x00, area_size - 2));
        if (next == nullptr) {
            return nullptr;
        }
        else if (next[1] != 0x00) {
            area_size -= (next - a) + 2;
            a = next + 2;
        }
        else if (next[2] == third) {
            return next;
        }
        else {
            area_size -= (next - a) + 1;
            a = next + 1;
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Check if a memory area contains all identical byte values.
//----------------------------------------------------------------------------

bool ts::IdenticalBytes(const void* area, size_t area_size)
{
    if (area_size < 2) {
        return false;
    }
    else {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(area);
        const uint8_t* const end = p + area_size;
        const uint8_t val = *p++;
        while (p < end) {
            if (*p++ != val) {
                return false;
            }
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Compute an exclusive or over memory areas.
//----------------------------------------------------------------------------

void ts::MemXor(void* dest, const void* src1, const void* src2, size_t size)
{
    while (size >= 8) {
        *reinterpret_cast<uint64_t*>(dest) = *reinterpret_cast<const uint64_t*>(src1) ^ *reinterpret_cast<const uint64_t*>(src2);
        dest = reinterpret_cast<uint8_t*>(dest) + 8;
        src1 = reinterpret_cast<const uint8_t*>(src1) + 8;
        src2 = reinterpret_cast<const uint8_t*>(src2) + 8;
        size -= 8;
    }
    if (size >= 4) {
        *reinterpret_cast<uint32_t*>(dest) = *reinterpret_cast<const uint32_t*>(src1) ^ *reinterpret_cast<const uint32_t*>(src2);
        dest = reinterpret_cast<uint8_t*>(dest) + 4;
        src1 = reinterpret_cast<const uint8_t*>(src1) + 4;
        src2 = reinterpret_cast<const uint8_t*>(src2) + 4;
        size -= 4;
    }
    while (size > 0) {
        *reinterpret_cast<uint8_t*>(dest) = *reinterpret_cast<const uint8_t*>(src1) ^ *reinterpret_cast<const uint8_t*>(src2);
        dest = reinterpret_cast<uint8_t*>(dest) + 1;
        src1 = reinterpret_cast<const uint8_t*>(src1) + 1;
        src2 = reinterpret_cast<const uint8_t*>(src2) + 1;
        size -= 1;
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

// 56 bits

uint64_t ts::GetUInt56BE(const void* p)
{
    return (static_cast<uint64_t>(GetUInt32BE(p)) << 24) | GetUInt24BE(static_cast<const uint8_t*>(p) + 4);
}

uint64_t ts::GetUInt56LE(const void* p)
{
    return (static_cast<uint64_t>(GetUInt24LE(static_cast<const uint8_t*>(p) + 4)) << 32) | GetUInt32LE(p);
}

void ts::PutUInt56BE(void* p, uint64_t i)
{
    *(static_cast<uint8_t*>(p)) = static_cast<uint8_t>(i >> 48);
    *(reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + 1)) = CondByteSwap16BE(static_cast<uint16_t>(i >> 32));
    *(reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(p) + 3)) = CondByteSwap32BE(static_cast<uint32_t>(i));
}

void ts::PutUInt56LE(void* p, uint64_t i)
{
    *(static_cast<uint32_t*>(p)) = CondByteSwap32LE(static_cast<uint32_t>(i));
    *(reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(p) + 4)) = CondByteSwap16LE(static_cast<uint16_t>(i >> 32));
    *(static_cast<uint8_t*>(p) + 6) = static_cast<uint8_t>(i >> 48);
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

uint64_t ts::GetUInt56BE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p))) << 48;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 1)) << 40;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 2)) << 32;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 3)) << 24;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 4)) << 16;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5)) << 8;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 6));
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

uint64_t ts::GetUInt56LE(const void* p)
{
    uint64_t value = static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 6)) << 48;
    value |= static_cast<uint64_t>(*(static_cast<const uint8_t*>(p) + 5)) << 40;
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

void ts::PutUInt56BE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i >> 48);
    data[1] = static_cast<uint8_t>(i >> 40);
    data[2] = static_cast<uint8_t>(i >> 32);
    data[3] = static_cast<uint8_t>(i >> 24);
    data[4] = static_cast<uint8_t>(i >> 16);
    data[5] = static_cast<uint8_t>(i >> 8);
    data[6] = static_cast<uint8_t>(i);
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

void ts::PutUInt56LE(void* p, uint64_t i)
{
    uint8_t *data = static_cast<uint8_t*>(p);
    data[0] = static_cast<uint8_t>(i);
    data[1] = static_cast<uint8_t>(i >> 8);
    data[2] = static_cast<uint8_t>(i >> 16);
    data[3] = static_cast<uint8_t>(i >> 24);
    data[4] = static_cast<uint8_t>(i >> 32);
    data[5] = static_cast<uint8_t>(i >> 40);
    data[6] = static_cast<uint8_t>(i >> 48);
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
