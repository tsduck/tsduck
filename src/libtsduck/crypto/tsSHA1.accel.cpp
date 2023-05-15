//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-1 hash.
//
//  Arm64 acceleration based on public domain code from Arm.
//
//----------------------------------------------------------------------------
//
// Implementation of SHA-1 using accelerated instructions, when available.
// This module is compiled with special options to use optional instructions
// for the target architecture. It may fail when these instructions are not
// implemented in the current CPU. Consequently, this module shall not be
// called when these instructions are not implemented.
//
//----------------------------------------------------------------------------

#include "tsSHA1.h"
#include "tsCryptoAcceleration.h"

// Check if Arm-64 SHA-1 instructions can be used in asm() directives and intrinsics.
#if defined(__ARM_FEATURE_CRYPTO) && !defined(TS_NO_ARM_SHA1_INSTRUCTIONS)
    #define TS_ARM_SHA1_INSTRUCTIONS 1
#endif

#if defined(TS_ARM_SHA1_INSTRUCTIONS)
#include <arm_neon.h>
namespace {
    volatile uint32x4_t C0;
    volatile uint32x4_t C1;
    volatile uint32x4_t C2;
    volatile uint32x4_t C3;
}
#endif

// "Hidden" exported bool to inform the SysInfo class that we have compiled accelerated instructions.
extern const bool tsSHA1IsAccelerated =
#if defined(TS_ARM_SHA1_INSTRUCTIONS)
    true;
#else
    false;
#endif

// Don't complain about assert(false) when acceleration is not implemented.
TS_LLVM_NOWARNING(missing-noreturn)


//----------------------------------------------------------------------------
// Static initialization.
//----------------------------------------------------------------------------

void ts::SHA1::initAccel()
{
#if defined(TS_ARM_SHA1_INSTRUCTIONS)
    C0 = vdupq_n_u32(0x5A827999);
    C1 = vdupq_n_u32(0x6ED9EBA1);
    C2 = vdupq_n_u32(0x8F1BBCDC);
    C3 = vdupq_n_u32(0xCA62C1D6);
#else
    // Shall not be called.
    assert(false);
#endif
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA1::compressAccel(const uint8_t* buf)
{
#if defined(TS_ARM_SHA1_INSTRUCTIONS)
    // Copy state
    uint32x4_t abcd = vld1q_u32(_state);
    uint32_t e = _state[4];

    const uint32_t* buf32 = reinterpret_cast<const uint32_t*>(buf);
    uint32x4_t msg0 = vld1q_u32(buf32 + 0);
    uint32x4_t msg1 = vld1q_u32(buf32 + 4);
    uint32x4_t msg2 = vld1q_u32(buf32 + 8);
    uint32x4_t msg3 = vld1q_u32(buf32 + 12);

    // Swap bytes if little endian Arm64.
#if defined(TS_LITTLE_ENDIAN)
    msg0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg0)));
    msg1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg1)));
    msg2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg2)));
    msg3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg3)));
#endif

    uint32x4_t tmp = vaddq_u32(msg0, C0);
    uint32x4_t tmp1 = vaddq_u32(msg1, C0);

    // Rounds 0-3
    uint32_t e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg2, C0);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 4-7
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, C0);
    msg0 = vsha1su1q_u32(msg0, msg3);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 8-11
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg0, C0);
    msg1 = vsha1su1q_u32(msg1, msg0);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 12-15
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, C1);
    msg2 = vsha1su1q_u32(msg2, msg1);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 16-19
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg2, C1);
    msg3 = vsha1su1q_u32(msg3, msg2);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 20-23
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, C1);
    msg0 = vsha1su1q_u32(msg0, msg3);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 24-27
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg0, C1);
    msg1 = vsha1su1q_u32(msg1, msg0);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 28-31
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, C1);
    msg2 = vsha1su1q_u32(msg2, msg1);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 32-35
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg2, C2);
    msg3 = vsha1su1q_u32(msg3, msg2);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 36-39
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, C2);
    msg0 = vsha1su1q_u32(msg0, msg3);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 40-43
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg0, C2);
    msg1 = vsha1su1q_u32(msg1, msg0);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 44-47
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, C2);
    msg2 = vsha1su1q_u32(msg2, msg1);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 48-51
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg2, C2);
    msg3 = vsha1su1q_u32(msg3, msg2);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 52-55
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, C3);
    msg0 = vsha1su1q_u32(msg0, msg3);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 56-59
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg0, C3);
    msg1 = vsha1su1q_u32(msg1, msg0);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 60-63
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, C3);
    msg2 = vsha1su1q_u32(msg2, msg1);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 64-67
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e, tmp);
    tmp = vaddq_u32(msg2, C3);
    msg3 = vsha1su1q_u32(msg3, msg2);
    // msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 68-71
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, C3);
    // msg0 = vsha1su1q_u32(msg0, msg3);

    // Rounds 72-75
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e, tmp);

    // Rounds 76-79
    e = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);

    // Store state: add ABCD E to state 0..5
    vst1q_u32(_state, vaddq_u32(vld1q_u32(_state), abcd));
    _state[4] += e;
#else
    // Shall not be called.
    assert(false);
#endif
}
