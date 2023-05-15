//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-256 hash.
//
//  Arm64 acceleration based on public domain code from Arm.
//
//----------------------------------------------------------------------------
//
// Implementation of SHA-256 using accelerated instructions, when available.
// This module is compiled with special options to use optional instructions
// for the target architecture. It may fail when these instructions are not
// implemented in the current CPU. Consequently, this module shall not be
// called when these instructions are not implemented.
//
//----------------------------------------------------------------------------

#include "tsSHA256.h"
#include "tsCryptoAcceleration.h"

// Check if Arm-64 SHA-256 instructions can be used in asm() directives and intrinsics.
#if defined(__ARM_FEATURE_SHA2) && !defined(TS_NO_ARM_SHA256_INSTRUCTIONS)
    #define TS_ARM_SHA256_INSTRUCTIONS 1
#endif

#if defined(TS_ARM_SHA256_INSTRUCTIONS)
    #include <arm_neon.h>
#endif

// "Hidden" exported bool to inform the SysInfo class that we have compiled accelerated instructions.
extern const bool tsSHA256IsAccelerated =
#if defined(TS_ARM_SHA256_INSTRUCTIONS)
    true;
#else
    false;
#endif

// Don't complain about assert(false) when acceleration is not implemented.
TS_LLVM_NOWARNING(missing-noreturn)


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA256::compressAccel(const uint8_t* buf)
{
#if defined(TS_ARM_SHA256_INSTRUCTIONS)
    // Load initial values.
    uint32x4_t state0 = vld1q_u32(&_state[0]);
    uint32x4_t state1 = vld1q_u32(&_state[4]);

    // Save current state.
    const uint32x4_t previous_state0 = state0;
    const uint32x4_t previous_state1 = state1;

    // Load input block.
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

    // Rounds 0-3
    uint32x4_t msg_k = vaddq_u32(msg0, vld1q_u32(&K[4*0]));
    uint32x4_t tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg0 = vsha256su1q_u32(vsha256su0q_u32(msg0, msg1), msg2, msg3);

    // Rounds 4-7
    msg_k = vaddq_u32(msg1, vld1q_u32(&K[4*1]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg1 = vsha256su1q_u32(vsha256su0q_u32(msg1, msg2), msg3, msg0);

    // Rounds 8-11
    msg_k = vaddq_u32(msg2, vld1q_u32(&K[4*2]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg2 = vsha256su1q_u32(vsha256su0q_u32(msg2, msg3), msg0, msg1);

    // Rounds 12-15
    msg_k = vaddq_u32(msg3, vld1q_u32(&K[4*3]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg3 = vsha256su1q_u32(vsha256su0q_u32(msg3, msg0), msg1, msg2);

    // Rounds 16-19
    msg_k = vaddq_u32(msg0, vld1q_u32(&K[4*4]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg0 = vsha256su1q_u32(vsha256su0q_u32(msg0, msg1), msg2, msg3);

    // Rounds 20-23
    msg_k = vaddq_u32(msg1, vld1q_u32(&K[4*5]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg1 = vsha256su1q_u32(vsha256su0q_u32(msg1, msg2), msg3, msg0);

    // Rounds 24-27
    msg_k = vaddq_u32(msg2, vld1q_u32(&K[4*6]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg2 = vsha256su1q_u32(vsha256su0q_u32(msg2, msg3), msg0, msg1);

    // Rounds 28-31
    msg_k = vaddq_u32(msg3, vld1q_u32(&K[4*7]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg3 = vsha256su1q_u32(vsha256su0q_u32(msg3, msg0), msg1, msg2);

    // Rounds 32-35
    msg_k = vaddq_u32(msg0, vld1q_u32(&K[4*8]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg0 = vsha256su1q_u32(vsha256su0q_u32(msg0, msg1), msg2, msg3);

    // Rounds 36-39
    msg_k = vaddq_u32(msg1, vld1q_u32(&K[4*9]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg1 = vsha256su1q_u32(vsha256su0q_u32(msg1, msg2), msg3, msg0);

    // Rounds 40-43
    msg_k = vaddq_u32(msg2, vld1q_u32(&K[4*10]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg2 = vsha256su1q_u32(vsha256su0q_u32(msg2, msg3), msg0, msg1);

    // Rounds 44-47
    msg_k = vaddq_u32(msg3, vld1q_u32(&K[4*11]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;
    msg3 = vsha256su1q_u32(vsha256su0q_u32(msg3, msg0), msg1, msg2);

    // Rounds 48-51
    msg_k = vaddq_u32(msg0, vld1q_u32(&K[4*12]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;

    // Rounds 52-55
    msg_k = vaddq_u32(msg1, vld1q_u32(&K[4*13]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;

    // Rounds 56-59
    msg_k = vaddq_u32(msg2, vld1q_u32(&K[4*14]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;

    // Rounds 60-63
    msg_k = vaddq_u32(msg3, vld1q_u32(&K[4*15]));
    tmp_state = vsha256hq_u32(state0, state1, msg_k);
    state1 = vsha256h2q_u32(state1, state0, msg_k);
    state0 = tmp_state;

    // Add back to state
    state0 = vaddq_u32(state0, previous_state0);
    state1 = vaddq_u32(state1, previous_state1);

    // Save state
    vst1q_u32(&_state[0], state0);
    vst1q_u32(&_state[4], state1);
#else
    // Shall not be called.
    assert(false);
#endif
}
