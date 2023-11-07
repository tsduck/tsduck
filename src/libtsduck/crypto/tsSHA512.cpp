//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-512 hash.
//
//  Implementation based on LibTomCrypt (http://www.libtom.org/)
//  by Tom St Denis (tomstdenis@gmail.com)
//
//  Usage in TSDuck allowed based on LibTomCrypt licence:
//    << LibTomCrypt is public domain. The library is free for >>
//    << all purposes without any express guarantee it works.  >>
//
//----------------------------------------------------------------------------

#include "tsSHA512.h"
#include "tsMemory.h"
#include "tsRotate.h"
#include "tsSysInfo.h"

// Runtime check once if accelerated SHA-256 instructions are supported on this CPU.
volatile bool ts::SHA512::_accel_checked = false;
volatile bool ts::SHA512::_accel_supported = false;

#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) (((x | y) & z) | (x & y))
#define S(x, n)    (ROR64c((x), (n)))
#define R(x, n)    (((x) & 0xFFFFFFFFFFFFFFFF) >> uint64_t(n))
#define Sigma0(x)  (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)  (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)  (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)  (S(x, 19) ^ S(x, 61) ^ R(x, 6))


//----------------------------------------------------------------------------
// The K array
//----------------------------------------------------------------------------

const uint64_t ts::SHA512::K[80] = {
    0x428A2F98D728AE22, 0x7137449123EF65CD, 0xB5C0FBCFEC4D3B2F, 0xE9B5DBA58189DBBC,
    0x3956C25BF348B538, 0x59F111F1B605D019, 0x923F82A4AF194F9B, 0xAB1C5ED5DA6D8118,
    0xD807AA98A3030242, 0x12835B0145706FBE, 0x243185BE4EE4B28C, 0x550C7DC3D5FFB4E2,
    0x72BE5D74F27B896F, 0x80DEB1FE3B1696B1, 0x9BDC06A725C71235, 0xC19BF174CF692694,
    0xE49B69C19EF14AD2, 0xEFBE4786384F25E3, 0x0FC19DC68B8CD5B5, 0x240CA1CC77AC9C65,
    0x2DE92C6F592B0275, 0x4A7484AA6EA6E483, 0x5CB0A9DCBD41FBD4, 0x76F988DA831153B5,
    0x983E5152EE66DFAB, 0xA831C66D2DB43210, 0xB00327C898FB213F, 0xBF597FC7BEEF0EE4,
    0xC6E00BF33DA88FC2, 0xD5A79147930AA725, 0x06CA6351E003826F, 0x142929670A0E6E70,
    0x27B70A8546D22FFC, 0x2E1B21385C26C926, 0x4D2C6DFC5AC42AED, 0x53380D139D95B3DF,
    0x650A73548BAF63DE, 0x766A0ABB3C77B2A8, 0x81C2C92E47EDAEE6, 0x92722C851482353B,
    0xA2BFE8A14CF10364, 0xA81A664BBC423001, 0xC24B8B70D0F89791, 0xC76C51A30654BE30,
    0xD192E819D6EF5218, 0xD69906245565A910, 0xF40E35855771202A, 0x106AA07032BBD1B8,
    0x19A4C116B8D2D0C8, 0x1E376C085141AB53, 0x2748774CDF8EEB99, 0x34B0BCB5E19B48A8,
    0x391C0CB3C5C95A63, 0x4ED8AA4AE3418ACB, 0x5B9CCA4F7763E373, 0x682E6FF3D6B2B8A3,
    0x748F82EE5DEFB2FC, 0x78A5636F43172F60, 0x84C87814A1F0AB72, 0x8CC702081A6439EC,
    0x90BEFFFA23631E28, 0xA4506CEBDE82BDE9, 0xBEF9A3F7B2C67915, 0xC67178F2E372532B,
    0xCA273ECEEA26619C, 0xD186B8C721C0C207, 0xEADA7DD6CDE0EB1E, 0xF57D4F7FEE6ED178,
    0x06F067AA72176FBA, 0x0A637DC5A2C898A6, 0x113F9804BEF90DAE, 0x1B710B35131C471B,
    0x28DB77F523047D84, 0x32CAAB7B40C72493, 0x3C9EBE0A15C9BEBC, 0x431D67C49C100D4C,
    0x4CC5D4BECB3E42B6, 0x597F299CFC657E2A, 0x5FCB6FAB3AD6FAEC, 0x6C44198C4A475817
};


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SHA512::SHA512()
{
    // Check once if SHA-512 acceleration is supported at runtime.
    // This logic does not require explicit synchronization.
    if (!_accel_checked) {
        _accel_supported = SysInfo::Instance().sha512Instructions();
        _accel_checked = true;
    }

    // Initialize internal state.
    SHA512::init();
}


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::UString ts::SHA512::name() const
{
    return u"SHA-512";
}
size_t ts::SHA512::hashSize() const
{
    return HASH_SIZE;
}
size_t ts::SHA512::blockSize() const
{
    return BLOCK_SIZE;
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
//----------------------------------------------------------------------------

bool ts::SHA512::init()
{
    _curlen = 0;
    _length = 0;
    _state[0] = 0x6A09E667F3BCC908;
    _state[1] = 0xBB67AE8584CAA73B;
    _state[2] = 0x3C6EF372FE94F82B;
    _state[3] = 0xA54FF53A5F1D36F1;
    _state[4] = 0x510E527FADE682D1;
    _state[5] = 0x9B05688C2B3E6C1F;
    _state[6] = 0x1F83D9ABFB41BD6B;
    _state[7] = 0x5BE0CD19137E2179;
    return true;
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA512::compress(const uint8_t* buf)
{
    if (_accel_supported) {
        compressAccel(buf);
    }
    else {
        // Portable implementation.
        uint64_t S[8], W[80];

        // Copy state into S
        for (size_t i = 0; i < 8; i++) {
            S[i] = _state[i];
        }

        // Copy the state into 1024-bits into W[0..15]
        for (size_t i = 0; i < 16; i++) {
            W[i] = GetUInt64(buf + 8*i);
        }

        // Fill W[16..79]
        for (size_t i = 16; i < 80; i++) {
            W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
        }

        // Compress
        uint64_t t0, t1;
#define RND(a,b,c,d,e,f,g,h,i)                              \
            t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i]; \
            t1 = Sigma0(a) + Maj(a, b, c);                  \
            d += t0;                                        \
            h  = t0 + t1

        for (size_t i = 0; i < 80; i += 8) {
            RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i+0);
            RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], i+1);
            RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], i+2);
            RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], i+3);
            RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], i+4);
            RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], i+5);
            RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], i+6);
            RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], i+7);
        }

#undef RND

        // Feedback
        for (size_t i = 0; i < 8; i++) {
            _state[i] = _state[i] + S[i];
        }
    }
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
//----------------------------------------------------------------------------

bool ts::SHA512::add(const void* data, size_t size)
{
    // Filter invalid internal state.
    if (_curlen >= sizeof(_buf)) {
        return false;
    }

    const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
    while (size > 0) {
        if (_curlen == 0 && size >= BLOCK_SIZE) {
            // Compress one 1024-bit block directly from user's buffer.
            compress(in);
            _length += BLOCK_SIZE * 8;
            in += BLOCK_SIZE;
            size -= BLOCK_SIZE;
        }
        else {
            // Partial block, Accumulate input data in internal buffer.
            size_t n = std::min(size, (BLOCK_SIZE - _curlen));
            std::memcpy(_buf + _curlen, in, n);
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
//----------------------------------------------------------------------------

bool ts::SHA512::getHash(void* hash, size_t bufsize, size_t* retsize)
{
    // Filter invalid internal state or invalid input.
    if (_curlen >= sizeof(_buf) || bufsize < HASH_SIZE) {
        return false;
    }

    // Increase the length of the message
    _length += _curlen * 8;

    // Append the '1' bit
    _buf[_curlen++] = 0x80;

    // Pad with zeroes and append 128-bit message length in bits.
    // If the length is currently above 112 bytes (no room for message length), append zeroes then compress.
    if (_curlen > 112) {
        Zero(_buf + _curlen, 128 - _curlen);
        compress(_buf);
        _curlen = 0;
    }

    // Pad up to 120 bytes with zeroes and append 64-bit message length in bits.
    // Note: zeroes from 112 to 120 are the 64 MSB of the length. We assume that you won't hash > 2^64 bits of data.
    Zero(_buf + _curlen, 120 - _curlen);
    PutUInt64(_buf + 120, _length);
    compress(_buf);

    // Copy output
    uint8_t* out = reinterpret_cast<uint8_t*>(hash);
    for (size_t i = 0; i < 8; i++) {
        PutUInt64(out + 8*i, _state[i]);
    }

    if (retsize != nullptr) {
        *retsize = HASH_SIZE;
    }
    return true;
}
