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
//
// Implementation of CRC32 using accelerated instructions, when available.
// This module is compiled with special options to use optional instructions
// for the target architecture. It may fail when these instructions are not
// implemented in the current CPU. Consequently, this module shall not be
// called when these instructions are not implemented.
//
//----------------------------------------------------------------------------

#include "tsCRC32.h"
#include "tsCryptoAcceleration.h"

// Check if Arm-64 CRC32 instructions can be used in asm() directives.
#if defined(__ARM_FEATURE_CRC32) && !defined(TS_NO_ARM_CRC32_INSTRUCTIONS)
    #define TS_ARM_CRC32_INSTRUCTIONS 1
#endif

// "Hidden" exported bool to inform the SysInfo class that we have compiled accelerated instructions.
extern const bool tsCRC32IsAccelerated =
#if defined(TS_ARM_CRC32_INSTRUCTIONS)
    true;
#else
    false;
#endif

// Don't complain about assert(false) when acceleration is not implemented.
TS_LLVM_NOWARNING(missing-noreturn)


//----------------------------------------------------------------------------
// Get the value of the CRC32 as computed so far.
//----------------------------------------------------------------------------

uint32_t ts::CRC32::valueAccel() const
{
#if defined(TS_ARM_CRC32_INSTRUCTIONS)
    // With the Arm64 CRC32 instructions, we need to reverse the 32 bits in the result.
    uint32_t x;
    asm("rbit %w0, %w1" : "=r" (x) : "r" (_fcs));
    return x;
#else
    // Shall not be called.
    assert(false);
    return 0;
#endif
}


//----------------------------------------------------------------------------
// Basic operations for the Arm64 CRC32 instructions.
//----------------------------------------------------------------------------

#if defined(TS_ARM_CRC32_INSTRUCTIONS)
namespace {

    // Arm Architecture Reference Manual, about the CRC32 instructions: "To align
    // with common usage, the bit order of the values is reversed as part of the
    // operation". However, the CRC32 computation for MPEG2-TS does not reverse
    // the bits. Consequently, we have to reverse the bits again on input and
    // output. We do this using 2 Arm64 instructions (would be dreadful in C++).

    // Reverse all bits inside each individual byte of a 64-bit value.
    // Then, add the 64-bit result in the CRC32 computation.
    inline __attribute__((always_inline)) void crcAdd64(uint32_t& fcs, uint64_t x)
    {
        asm("rbit   %1, %1\n"
            "rev    %1, %1\n"
            "crc32x %w0, %w0, %1"
            : "+r" (fcs) : "r" (x));
    }

    // Same thing on one byte only.
    inline __attribute__((always_inline)) void crcAdd8(uint32_t& fcs, uint8_t x)
    {
        asm("rbit   %1, %1\n"
            "rev    %1, %1\n"
            "crc32b %w0, %w0, %w1"
            : "+r" (fcs) : "r" (uint64_t(x)));
    }
}
#endif


//----------------------------------------------------------------------------
// Continue the computation of a data area, following a previous CRC32.
//----------------------------------------------------------------------------

void ts::CRC32::addAccel(const void* data, size_t size)
{
#if defined(TS_ARM_CRC32_INSTRUCTIONS)
    // Add 8-bit values until an address aligned on 8 bytes.
    const uint8_t* cp8 = reinterpret_cast<const uint8_t*>(data);
    while (size != 0 && (uint64_t(cp8) & 0x03) != 0) {
        crcAdd8(_fcs, *cp8++);
        --size;
    }

    // Add 64-bit values until an address aligned on 64 bytes.
    const uint64_t* cp64 = reinterpret_cast<const uint64_t*>(cp8);
    while (size >= 8 && (uint64_t(cp64) & 0x07) != 0) {
        crcAdd64(_fcs, *cp64++);
        size -= 8;
    }

    // Add 8 * 64-bit values until less than 64 bytes (manual loop unroll).
    while (size >= 64) {
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        crcAdd64(_fcs, *cp64++);
        size -= 64;
    }

    // Add 64-bit values until less than 8 bytes.
    while (size >= 8) {
        crcAdd64(_fcs, *cp64++);
        size -= 8;
    }

    // Add remaining bytes.
    cp8 = reinterpret_cast<const uint8_t*>(cp64);
    while (size--) {
        crcAdd8(_fcs, *cp8++);
    }
#else
    // Shall not be called.
    assert(false);
#endif
}
