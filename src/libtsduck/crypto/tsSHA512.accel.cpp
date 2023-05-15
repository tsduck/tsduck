//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-512 hash.
//
//  Arm64 acceleration based on public domain code from Arm.
//
//----------------------------------------------------------------------------
//
// Implementation of SHA-512 using accelerated instructions, when available.
// This module is compiled with special options to use optional instructions
// for the target architecture. It may fail when these instructions are not
// implemented in the current CPU. Consequently, this module shall not be
// called when these instructions are not implemented.
//
//----------------------------------------------------------------------------

#include "tsSHA512.h"
#include "tsCryptoAcceleration.h"

// Check if Arm-64 SHA-256 instructions can be used in asm() directives and intrinsics.
#if defined(__ARM_FEATURE_SHA512) && !defined(TS_NO_ARM_SHA512_INSTRUCTIONS)
    #define TS_ARM_SHA512_INSTRUCTIONS 1
#endif

#if defined(TS_ARM_SHA512_INSTRUCTIONS)
    #include <arm_neon.h>
#endif

// "Hidden" exported bool to inform the SysInfo class that we have compiled accelerated instructions.
extern const bool tsSHA512IsAccelerated =
#if defined(TS_ARM_SHA512_INSTRUCTIONS)
    true;
#else
    false;
#endif

// Don't complain about assert(false) when acceleration is not implemented.
TS_LLVM_NOWARNING(missing-noreturn)


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA512::compressAccel(const uint8_t* buf)
{
#if defined(TS_ARM_SHA512_INSTRUCTIONS)
    // Load initial values.
    uint64x2_t ab = vld1q_u64(&_state[0]);
    uint64x2_t cd = vld1q_u64(&_state[2]);
    uint64x2_t ef = vld1q_u64(&_state[4]);
    uint64x2_t gh = vld1q_u64(&_state[6]);

    // Save current state.
    uint64x2_t previous_ab = ab;
    uint64x2_t previous_cd = cd;
    uint64x2_t previous_ef = ef;
    uint64x2_t previous_gh = gh;

    // Load input block.
    const uint8_t* buf8 = reinterpret_cast<const uint8_t*>(buf);
    uint64x2_t s0 = uint64x2_t(vld1q_u8(buf8 + 16 * 0));
    uint64x2_t s1 = uint64x2_t(vld1q_u8(buf8 + 16 * 1));
    uint64x2_t s2 = uint64x2_t(vld1q_u8(buf8 + 16 * 2));
    uint64x2_t s3 = uint64x2_t(vld1q_u8(buf8 + 16 * 3));
    uint64x2_t s4 = uint64x2_t(vld1q_u8(buf8 + 16 * 4));
    uint64x2_t s5 = uint64x2_t(vld1q_u8(buf8 + 16 * 5));
    uint64x2_t s6 = uint64x2_t(vld1q_u8(buf8 + 16 * 6));
    uint64x2_t s7 = uint64x2_t(vld1q_u8(buf8 + 16 * 7));

    // Swap bytes if little endian Arm64.
#if defined(TS_LITTLE_ENDIAN)
    s0 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s0)));
    s1 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s1)));
    s2 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s2)));
    s3 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s3)));
    s4 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s4)));
    s5 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s5)));
    s6 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s6)));
    s7 = vreinterpretq_u64_u8(vrev64q_u8(vreinterpretq_u8_u64(s7)));
#endif

    // Rounds 0 and 1
    uint64x2_t initial_sum = vaddq_u64(s0, vld1q_u64(&K[0]));
    uint64x2_t sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
    uint64x2_t intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
    gh = vsha512h2q_u64(intermed, cd, ab);
    cd = vaddq_u64(cd, intermed);

    // Rounds 2 and 3
    initial_sum = vaddq_u64(s1, vld1q_u64(&K[2]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
    intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
    ef = vsha512h2q_u64(intermed, ab, gh);
    ab = vaddq_u64(ab, intermed);

    // Rounds 4 and 5
    initial_sum = vaddq_u64(s2, vld1q_u64(&K[4]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
    intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
    cd = vsha512h2q_u64(intermed, gh, ef);
    gh = vaddq_u64(gh, intermed);

    // Rounds 6 and 7
    initial_sum = vaddq_u64(s3, vld1q_u64(&K[6]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
    intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
    ab = vsha512h2q_u64(intermed, ef, cd);
    ef = vaddq_u64(ef, intermed);

    // Rounds 8 and 9
    initial_sum = vaddq_u64(s4, vld1q_u64(&K[8]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
    intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
    gh = vsha512h2q_u64(intermed, cd, ab);
    cd = vaddq_u64(cd, intermed);

    // Rounds 10 and 11
    initial_sum = vaddq_u64(s5, vld1q_u64(&K[10]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
    intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
    ef = vsha512h2q_u64(intermed, ab, gh);
    ab = vaddq_u64(ab, intermed);

    // Rounds 12 and 13
    initial_sum = vaddq_u64(s6, vld1q_u64(&K[12]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
    intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
    cd = vsha512h2q_u64(intermed, gh, ef);
    gh = vaddq_u64(gh, intermed);

    // Rounds 14 and 15
    initial_sum = vaddq_u64(s7, vld1q_u64(&K[14]));
    sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
    intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
    ab = vsha512h2q_u64(intermed, ef, cd);
    ef = vaddq_u64(ef, intermed);

    for (unsigned int t = 16; t < 80; t += 16) {
        // Rounds t and t + 1
        s0 = vsha512su1q_u64(vsha512su0q_u64(s0, s1), s7, vextq_u64(s4, s5, 1));
        initial_sum = vaddq_u64(s0, vld1q_u64(&K[t]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
        intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
        gh = vsha512h2q_u64(intermed, cd, ab);
        cd = vaddq_u64(cd, intermed);

        // Rounds t + 2 and t + 3
        s1 = vsha512su1q_u64(vsha512su0q_u64(s1, s2), s0, vextq_u64(s5, s6, 1));
        initial_sum = vaddq_u64(s1, vld1q_u64(&K[t + 2]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
        intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
        ef = vsha512h2q_u64(intermed, ab, gh);
        ab = vaddq_u64(ab, intermed);

        // Rounds t + 4 and t + 5
        s2 = vsha512su1q_u64(vsha512su0q_u64(s2, s3), s1, vextq_u64(s6, s7, 1));
        initial_sum = vaddq_u64(s2, vld1q_u64(&K[t + 4]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
        intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
        cd = vsha512h2q_u64(intermed, gh, ef);
        gh = vaddq_u64(gh, intermed);

        // Rounds t + 6 and t + 7
        s3 = vsha512su1q_u64(vsha512su0q_u64(s3, s4), s2, vextq_u64(s7, s0, 1));
        initial_sum = vaddq_u64(s3, vld1q_u64(&K[t + 6]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
        intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
        ab = vsha512h2q_u64(intermed, ef, cd);
        ef = vaddq_u64(ef, intermed);

        // Rounds t + 8 and t + 9
        s4 = vsha512su1q_u64(vsha512su0q_u64(s4, s5), s3, vextq_u64(s0, s1, 1));
        initial_sum = vaddq_u64(s4, vld1q_u64(&K[t + 8]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), gh);
        intermed = vsha512hq_u64(sum, vextq_u64(ef, gh, 1), vextq_u64(cd, ef, 1));
        gh = vsha512h2q_u64(intermed, cd, ab);
        cd = vaddq_u64(cd, intermed);

        // Rounds t + 10 and t + 11
        s5 = vsha512su1q_u64(vsha512su0q_u64(s5, s6), s4, vextq_u64(s1, s2, 1));
        initial_sum = vaddq_u64(s5, vld1q_u64(&K[t + 10]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ef);
        intermed = vsha512hq_u64(sum, vextq_u64(cd, ef, 1), vextq_u64(ab, cd, 1));
        ef = vsha512h2q_u64(intermed, ab, gh);
        ab = vaddq_u64(ab, intermed);

        // Rounds t + 12 and t + 13
        s6 = vsha512su1q_u64(vsha512su0q_u64(s6, s7), s5, vextq_u64(s2, s3, 1));
        initial_sum = vaddq_u64(s6, vld1q_u64(&K[t + 12]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), cd);
        intermed = vsha512hq_u64(sum, vextq_u64(ab, cd, 1), vextq_u64(gh, ab, 1));
        cd = vsha512h2q_u64(intermed, gh, ef);
        gh = vaddq_u64(gh, intermed);

        // Rounds t + 14 and t + 15
        s7 = vsha512su1q_u64(vsha512su0q_u64(s7, s0), s6, vextq_u64(s3, s4, 1));
        initial_sum = vaddq_u64(s7, vld1q_u64(&K[t + 14]));
        sum = vaddq_u64(vextq_u64(initial_sum, initial_sum, 1), ab);
        intermed = vsha512hq_u64(sum, vextq_u64(gh, ab, 1), vextq_u64(ef, gh, 1));
        ab = vsha512h2q_u64(intermed, ef, cd);
        ef = vaddq_u64(ef, intermed);
    }

    // Add back to state
    ab = vaddq_u64(ab, previous_ab);
    cd = vaddq_u64(cd, previous_cd);
    ef = vaddq_u64(ef, previous_ef);
    gh = vaddq_u64(gh, previous_gh);

    // Save state
    vst1q_u64(&_state[0], ab);
    vst1q_u64(&_state[2], cd);
    vst1q_u64(&_state[4], ef);
    vst1q_u64(&_state[6], gh);
#else
    // Shall not be called.
    assert(false);
#endif
}
