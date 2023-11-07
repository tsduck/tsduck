//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  SHA-256 hash.
//
//  Implementation based on LibTomCrypt (http://www.libtom.org/)
//  by Tom St Denis (tomstdenis@gmail.com)
//
//  Usage in TSDuck allowed based on LibTomCrypt licence:
//    << LibTomCrypt is public domain. The library is free for >>
//    << all purposes without any express guarantee it works.  >>
//
//----------------------------------------------------------------------------

#include "tsSHA256.h"
#include "tsMemory.h"
#include "tsRotate.h"
#include "tsSysInfo.h"

// Runtime check once if accelerated SHA-256 instructions are supported on this CPU.
volatile bool ts::SHA256::_accel_checked = false;
volatile bool ts::SHA256::_accel_supported = false;

#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) (((x | y) & z) | (x & y))
#define S(x, n)    (RORc((x),(n)))
#define R(x, n)    (((x) & 0xFFFFFFFF) >> (n))
#define Sigma0(x)  (S(x, 2) ^ S(x, 13) ^ S(x, 22))
#define Sigma1(x)  (S(x, 6) ^ S(x, 11) ^ S(x, 25))
#define Gamma0(x)  (S(x, 7) ^ S(x, 18) ^ R(x, 3))
#define Gamma1(x)  (S(x, 17) ^ S(x, 19) ^ R(x, 10))


//----------------------------------------------------------------------------
// The K array
//----------------------------------------------------------------------------

const uint32_t ts::SHA256::K[64] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SHA256::SHA256()
{
    // Check once if SHA-256 acceleration is supported at runtime.
    // This logic does not require explicit synchronization.
    if (!_accel_checked) {
        _accel_supported = SysInfo::Instance().sha256Instructions();
        _accel_checked = true;
    }

    // Initialize internal state.
    SHA256::init();
}


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::UString ts::SHA256::name() const
{
    return u"SHA-256";
}
size_t ts::SHA256::hashSize() const
{
    return HASH_SIZE;
}
size_t ts::SHA256::blockSize() const
{
    return BLOCK_SIZE;
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
//----------------------------------------------------------------------------

bool ts::SHA256::init()
{
    _curlen = 0;
    _length = 0;
    _state[0] = 0x6A09E667UL;
    _state[1] = 0xBB67AE85UL;
    _state[2] = 0x3C6EF372UL;
    _state[3] = 0xA54FF53AUL;
    _state[4] = 0x510E527FUL;
    _state[5] = 0x9B05688CUL;
    _state[6] = 0x1F83D9ABUL;
    _state[7] = 0x5BE0CD19UL;
    return true;
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA256::compress(const uint8_t* buf)
{
    if (_accel_supported) {
        compressAccel(buf);
    }
    else {
        // Portable implementation.
        uint32_t S[8], W[64];

        // Copy state into S
        for (size_t i = 0; i < 8; i++) {
            S[i] = _state[i];
        }

        // Copy the state into 512-bits into W[0..15]
        for (size_t i = 0; i < 16; i++) {
            W[i] = GetUInt32(buf + 4*i);
        }

        // Fill W[16..63]
        for (size_t i = 16; i < 64; i++) {
            W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
        }

        // Compress
        uint32_t t0, t1;
#define RND(a,b,c,d,e,f,g,h,i,ki)                         \
            t0 = h + Sigma1(e) + Ch(e, f, g) + ki + W[i]; \
            t1 = Sigma0(a) + Maj(a, b, c);                \
            d += t0;                                      \
            h  = t0 + t1

        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7],  0, 0x428A2F98);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6],  1, 0x71374491);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5],  2, 0xB5C0FBCF);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4],  3, 0xE9B5DBA5);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3],  4, 0x3956C25B);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2],  5, 0x59F111F1);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1],  6, 0x923F82A4);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0],  7, 0xAB1C5ED5);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7],  8, 0xD807AA98);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6],  9, 0x12835B01);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 10, 0x243185BE);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 11, 0x550C7DC3);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 12, 0x72BE5D74);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 13, 0x80DEB1FE);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 14, 0x9BDC06A7);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 15, 0xC19BF174);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 16, 0xE49B69C1);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 17, 0xEFBE4786);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 18, 0x0FC19DC6);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 19, 0x240CA1CC);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 20, 0x2DE92C6F);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 21, 0x4A7484AA);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 22, 0x5CB0A9DC);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 23, 0x76F988DA);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 24, 0x983E5152);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 25, 0xA831C66D);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 26, 0xB00327C8);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 27, 0xBF597FC7);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 28, 0xC6E00BF3);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 29, 0xD5A79147);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 30, 0x06CA6351);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 31, 0x14292967);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 32, 0x27B70A85);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 33, 0x2E1B2138);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 34, 0x4D2C6DFC);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 35, 0x53380D13);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 36, 0x650A7354);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 37, 0x766A0ABB);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 38, 0x81C2C92E);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 39, 0x92722C85);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 40, 0xA2BFE8A1);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 41, 0xA81A664B);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 42, 0xC24B8B70);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 43, 0xC76C51A3);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 44, 0xD192E819);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 45, 0xD6990624);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 46, 0xF40E3585);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 47, 0x106AA070);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 48, 0x19A4C116);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 49, 0x1E376C08);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 50, 0x2748774C);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 51, 0x34B0BCB5);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 52, 0x391C0CB3);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 53, 0x4ED8AA4A);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 54, 0x5B9CCA4F);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 55, 0x682E6FF3);
        RND(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], 56, 0x748F82EE);
        RND(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], 57, 0x78A5636F);
        RND(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], 58, 0x84C87814);
        RND(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], 59, 0x8CC70208);
        RND(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], 60, 0x90BEFFFA);
        RND(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], 61, 0xA4506CEB);
        RND(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], 62, 0xBEF9A3F7);
        RND(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], 63, 0xC67178F2);

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

bool ts::SHA256::add(const void* data, size_t size)
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

bool ts::SHA256::getHash(void* hash, size_t bufsize, size_t* retsize)
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
    uint8_t* out = reinterpret_cast<uint8_t*> (hash);
    for (size_t i = 0; i < 8; i++) {
        PutUInt32(out + 4*i, _state[i]);
    }

    if (retsize != nullptr) {
        *retsize = HASH_SIZE;
    }
    return true;
}
