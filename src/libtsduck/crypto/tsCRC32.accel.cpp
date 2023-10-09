//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
