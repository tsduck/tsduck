//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-1 hash.
//
//  Implementation based on LibTomCrypt (http://www.libtom.org/)
//  by Tom St Denis (tomstdenis@gmail.com)
//
//  Usage in TSDuck allowed based on LibTomCrypt licence:
//    << LibTomCrypt is public domain. The library is free for >>
//    << all purposes without any express guarantee it works.  >>
//
//----------------------------------------------------------------------------

#include "tsSHA1.h"
#include "tsMemory.h"
#include "tsRotate.h"
#include "tsSysInfo.h"

#if defined(TS_ARM_SHA1_INSTRUCTIONS)
#include <arm_neon.h>
namespace {
    // Runtime check once if Arm-64 SHA-1 instructions are supported on this CPU.
    volatile bool _sha1_checked = false;
    volatile bool _sha1_supported = false;
    volatile uint32x4_t C0;
    volatile uint32x4_t C1;
    volatile uint32x4_t C2;
    volatile uint32x4_t C3;
}
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SHA1::SHA1() :
    _length(0),
    _curlen(0)
{
#if defined(TS_ARM_SHA1_INSTRUCTIONS)
    // When SHA-1 instructions are compiled, check once if supported at runtime.
    // This logic does not require explicit synchronization.
    if (!_sha1_checked) {
        _sha1_supported = SysInfo::Instance()->sha1Instructions();
        if (_sha1_supported) {
            C0 = vdupq_n_u32(0x5A827999);
            C1 = vdupq_n_u32(0x6ED9EBA1);
            C2 = vdupq_n_u32(0x8F1BBCDC);
            C3 = vdupq_n_u32(0xCA62C1D6);
        }
        _sha1_checked = true;
    }
#endif

    // Initialize internal state.
    SHA1::init();
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SHA1::init()
{
    _state[0] = 0x67452301UL;
    _state[1] = 0xEFCDAB89UL;
    _state[2] = 0x98BADCFEUL;
    _state[3] = 0x10325476UL;
    _state[4] = 0xC3D2E1F0UL;
    _curlen = 0;
    _length = 0;
    return true;
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA1::compress(const uint8_t* buf)
{
#if defined(TS_ARM_SHA1_INSTRUCTIONS)
    // The acceleration, when available, is located inside the compress() method.
    // Based on public domain code from Arm.
    if (_sha1_supported) {

        // Copy state
        uint32x4_t abcd = vld1q_u32(_state);
        uint32_t e = _state[4];

        const uint32_t* buf32 = reinterpret_cast<const uint32_t*>(buf);
        uint32x4_t msg0 = vld1q_u32(buf32 + 0);
        uint32x4_t msg1 = vld1q_u32(buf32 + 4);
        uint32x4_t msg2 = vld1q_u32(buf32 + 8);
        uint32x4_t msg3 = vld1q_u32(buf32 + 12);

        msg0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg0)));
        msg1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg1)));
        msg2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg2)));
        msg3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(msg3)));

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

        // End of specialized implementation.
        return;
    }
#endif

    // Portable implementation.
    // Copy state.
    uint32_t a = _state[0];
    uint32_t b = _state[1];
    uint32_t c = _state[2];
    uint32_t d = _state[3];
    uint32_t e = _state[4];

    // Copy input block (512 bits, 64 bytes, 16 uint32) into W[0..15]
    uint32_t i, W[80];
    for (i = 0; i < 16; i++) {
        W[i] = GetUInt32(buf + 4*i);
    }

    // Expand it over 320 bytes (80 uint32)
    for (i = 16; i < 80; i++) {
        W[i] = ROLc(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    // Compress
    #define F0(x,y,z) (z ^ (x & (y ^ z)))
    #define F1(x,y,z) (x ^ y ^ z)
    #define F2(x,y,z) ((x & y) | (z & (x | y)))
    #define F3(x,y,z) (x ^ y ^ z)

    #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30)
    #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30)
    #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30)
    #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30)

    // Round one
    i = 0;
    while (i < 20) {
        FF0(a,b,c,d,e,i++);
        FF0(e,a,b,c,d,i++);
        FF0(d,e,a,b,c,i++);
        FF0(c,d,e,a,b,i++);
        FF0(b,c,d,e,a,i++);
    }

    // Round two
    while (i < 40) {
        FF1(a,b,c,d,e,i++);
        FF1(e,a,b,c,d,i++);
        FF1(d,e,a,b,c,i++);
        FF1(c,d,e,a,b,i++);
        FF1(b,c,d,e,a,i++);
    }

    // Round three
    while (i < 60) {
        FF2(a,b,c,d,e,i++);
        FF2(e,a,b,c,d,i++);
        FF2(d,e,a,b,c,i++);
        FF2(c,d,e,a,b,i++);
        FF2(b,c,d,e,a,i++);
    }

    // Round four
    while (i < 80) {
        FF3(a,b,c,d,e,i++);
        FF3(e,a,b,c,d,i++);
        FF3(d,e,a,b,c,i++);
        FF3(c,d,e,a,b,i++);
        FF3(b,c,d,e,a,i++);
    }

    #undef FF0
    #undef FF1
    #undef FF2
    #undef FF3

    #undef F0
    #undef F1
    #undef F2
    #undef F3

    // Store
    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;
    _state[4] += e;
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SHA1::add(const void* data, size_t size)
{
    // Filter invalid internal state.
    if (_curlen >= sizeof(_buf)) {
        return false;
    }

    const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
    while (size > 0) {
        if (_curlen == 0 && size >= BLOCK_SIZE) {
            // Compress one 512-bit block directly from user's buffer.
            compress(in);
            _length += BLOCK_SIZE * 8;
            in += BLOCK_SIZE;
            size -= BLOCK_SIZE;
        }
        else {
            // Partial block, Accumulate input data in internal buffer.
            const size_t n = std::min(size, (BLOCK_SIZE - _curlen));
            ::memcpy(_buf + _curlen, in, n);
            _curlen += n;
            in += n;
            size -= n;
            if (_curlen == BLOCK_SIZE) {
                compress(_buf);
                _length += 8 * BLOCK_SIZE;
                _curlen = 0;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Get the resulting hash value.
// If retsize is non-zero, return the actual hash size.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SHA1::getHash(void* hash, size_t bufsize, size_t* retsize)
{
    // Filter invalid internal state or invalid input.
    if (_curlen >= sizeof(_buf) || bufsize < HASH_SIZE) {
        return false;
    }

    // Increase the length of the message
    _length += _curlen * 8;

    // Append the '1' bit
    _buf[_curlen++] = 0x80;

    // Pad with zeroes and append 64-bit message length in bits.
    // If the length is currently above 56 bytes (no room for message length), append zeroes then compress.
    if (_curlen > 56) {
        Zero(_buf + _curlen, 64 - _curlen);
        compress(_buf);
        _curlen = 0;
    }

    // Pad up to 56 bytes with zeroes and append 64-bit message length in bits.
    Zero(_buf + _curlen, 56 - _curlen);
    PutUInt64(_buf + 56, _length);
    compress(_buf);

    // Copy output
    uint8_t* const out = reinterpret_cast<uint8_t*>(hash);
    PutUInt32(out, _state[0]);
    PutUInt32(out + 4, _state[1]);
    PutUInt32(out + 8, _state[2]);
    PutUInt32(out + 12, _state[3]);
    PutUInt32(out + 16, _state[4]);

    if (retsize != nullptr) {
        *retsize = HASH_SIZE;
    }
    return true;
}


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::UString ts::SHA1::name() const
{
    return u"SHA-1";
}
size_t ts::SHA1::hashSize() const
{
    return HASH_SIZE;
}
size_t ts::SHA1::blockSize() const
{
    return BLOCK_SIZE;
}
