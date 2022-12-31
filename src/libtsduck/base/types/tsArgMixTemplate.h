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
// Return ArgMix value as an integer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
INT ts::ArgMix::toInteger(bool raw) const
{
    switch (_type) {
        case INTEGER | BIT32 | SIGNED:
            return static_cast<INT>(_value.int32);
        case INTEGER | BIT1: // bool
        case INTEGER | BIT32:
            return static_cast<INT>(_value.uint32);
        case INTEGER | BIT64 | SIGNED:
            return static_cast<INT>(_value.int64);
        case INTEGER | BIT64:
            return static_cast<INT>(_value.uint64);
        case POINTER | INTEGER | BIT8  | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int8_t*>(_value.intptr));
        case POINTER | INTEGER | BIT8:
            return static_cast<INT>(*reinterpret_cast<uint8_t*>(_value.intptr));
        case POINTER | INTEGER | BIT16 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int16_t*>(_value.intptr));
        case POINTER | INTEGER | BIT16:
            return static_cast<INT>(*reinterpret_cast<uint16_t*>(_value.intptr));
        case POINTER | INTEGER | BIT32 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int32_t*>(_value.intptr));
        case POINTER | INTEGER | BIT32:
            return static_cast<INT>(*reinterpret_cast<uint32_t*>(_value.intptr));
        case POINTER | INTEGER | BIT64 | SIGNED:
            return static_cast<INT>(*reinterpret_cast<int64_t*>(_value.intptr));
        case POINTER | INTEGER | BIT64:
            return static_cast<INT>(*reinterpret_cast<uint64_t*>(_value.intptr));
        case ANUMBER:
            return static_cast<INT>(_value.anumber->toInt64());
        default:
            return static_cast<INT>(0);
    }
}


//----------------------------------------------------------------------------
// Store an integer value in the argument data, for pointers to integer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::ArgMix::storeInteger(INT i) const
{
    switch (_type) {
        case POINTER | INTEGER | BIT8  | SIGNED:
            *reinterpret_cast<int8_t*>(_value.intptr) = static_cast<int8_t>(i);
            return true;
        case POINTER | INTEGER | BIT8:
            *reinterpret_cast<uint8_t*>(_value.intptr) = static_cast<uint8_t>(i);
            return true;
        case POINTER | INTEGER | BIT16 | SIGNED:
            *reinterpret_cast<int16_t*>(_value.intptr) = static_cast<int16_t>(i);
            return true;
        case POINTER | INTEGER | BIT16:
            *reinterpret_cast<uint16_t*>(_value.intptr) = static_cast<uint16_t>(i);
            return true;
        case POINTER | INTEGER | BIT32 | SIGNED:
            *reinterpret_cast<int32_t*>(_value.intptr) = static_cast<int32_t>(i);
            return true;
        case POINTER | INTEGER | BIT32:
            *reinterpret_cast<uint32_t*>(_value.intptr) = static_cast<uint32_t>(i);
            return true;
        case POINTER | INTEGER | BIT64 | SIGNED:
            *reinterpret_cast<int64_t*>(_value.intptr) = static_cast<int64_t>(i);
            return true;
        case POINTER | INTEGER | BIT64:
            *reinterpret_cast<uint64_t*>(_value.intptr) = static_cast<uint64_t>(i);
            return true;
        default:
            // Not a pointer to integer.
            return false;
    }
}
