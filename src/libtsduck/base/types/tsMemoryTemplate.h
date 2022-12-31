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

#pragma once


//----------------------------------------------------------------------------
// Memory access with strict alignment.
//----------------------------------------------------------------------------

#if defined(TS_STRICT_MEMORY_ALIGN)

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntBE(const void* p)
{
    switch (sizeof(INT)) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16BE(p));
        case 4: return static_cast<INT>(GetUInt32BE(p));
        case 8: return static_cast<INT>(GetUInt64BE(p));
        default: assert (false); return 0;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntLE(const void* p)
{
    switch (sizeof(INT)) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16LE(p));
        case 4: return static_cast<INT>(GetUInt32LE(p));
        case 8: return static_cast<INT>(GetUInt64LE(p));
        default: assert (false); return 0;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::GetIntBE(const void* p, INT& i)
{
    switch (sizeof(INT)) {
        case 1: i = static_cast<INT>(GetUInt8(p)); break;
        case 2: i = static_cast<INT>(GetUInt16BE(p)); break;
        case 4: i = static_cast<INT>(GetUInt32BE(p)); break;
        case 8: i = static_cast<INT>(GetUInt64BE(p)); break;
        default: assert(false); i = 0;  break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::GetIntLE(const void* p, INT& i)
{
    switch (sizeof(INT)) {
        case 1: i = static_cast<INT>(GetUInt8(p)); break;
        case 2: i = static_cast<INT>(GetUInt16LE(p)); break;
        case 4: i = static_cast<INT>(GetUInt32LE(p)); break;
        case 8: i = static_cast<INT>(GetUInt64LE(p)); break;
        default: assert(false); i = 0;  break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntBE(void* p, INT i)
{
    switch (sizeof(INT)) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16BE(p, static_cast<uint16_t>(i)); break;
        case 4: PutUInt32BE(p, static_cast<uint32_t>(i)); break;
        case 8: PutUInt64BE(p, static_cast<uint64_t>(i)); break;
        default: assert(false); break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntLE(void* p, INT i)
{
    switch (sizeof(INT)) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16LE(p, static_cast<uint16_t>(i)); break;
        case 4: PutUInt32LE(p, static_cast<uint32_t>(i)); break;
        case 8: PutUInt64LE(p, static_cast<uint64_t>(i)); break;
        default: assert(false); break;
    }
}

#endif


//----------------------------------------------------------------------------
// Variable-length integers serialization.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntVarBE(const void* p, size_t size)
{
    switch (size) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16BE(p));
        case 3: return static_cast<INT>(GetUInt24BE(p));
        case 4: return static_cast<INT>(GetUInt32BE(p));
        case 5: return static_cast<INT>(GetUInt40BE(p));
        case 6: return static_cast<INT>(GetUInt48BE(p));
        case 8: return static_cast<INT>(GetUInt64BE(p));
        default: return static_cast<INT>(0);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::GetIntVarLE(const void* p, size_t size)
{
    switch (size) {
        case 1: return static_cast<INT>(GetUInt8(p));
        case 2: return static_cast<INT>(GetUInt16LE(p));
        case 3: return static_cast<INT>(GetUInt24LE(p));
        case 4: return static_cast<INT>(GetUInt32LE(p));
        case 5: return static_cast<INT>(GetUInt40LE(p));
        case 6: return static_cast<INT>(GetUInt48LE(p));
        case 8: return static_cast<INT>(GetUInt64LE(p));
        default: return static_cast<INT>(0);
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntVarBE(void* p, size_t size, INT i)
{
    switch (size) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16BE(p, static_cast<uint16_t>(i)); break;
        case 3: PutUInt24BE(p, static_cast<uint32_t>(i)); break;
        case 4: PutUInt32BE(p, static_cast<uint32_t>(i)); break;
        case 5: PutUInt40BE(p, static_cast<uint64_t>(i)); break;
        case 6: PutUInt48BE(p, static_cast<uint64_t>(i)); break;
        case 8: PutUInt64BE(p, static_cast<uint64_t>(i)); break;
        default: break;
    }
}

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::PutIntVarLE(void* p, size_t size, INT i)
{
    switch (size) {
        case 1: PutUInt8(p, static_cast<uint8_t>(i)); break;
        case 2: PutUInt16LE(p, static_cast<uint16_t>(i)); break;
        case 3: PutUInt24LE(p, static_cast<uint32_t>(i)); break;
        case 4: PutUInt32LE(p, static_cast<uint32_t>(i)); break;
        case 5: PutUInt40LE(p, static_cast<uint64_t>(i)); break;
        case 6: PutUInt48LE(p, static_cast<uint64_t>(i)); break;
        case 8: PutUInt64LE(p, static_cast<uint64_t>(i)); break;
        default: break;
    }
}
