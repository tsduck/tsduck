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
TSDUCK_SOURCE;

#define Ch(x,y,z)  (z ^ (x & (y ^ z)))
#define Maj(x,y,z) (((x | y) & z) | (x & y))
#define S(x, n)    (ROR64c(x, n))
#define R(x, n)    (((x) & TS_UCONST64(0xFFFFFFFFFFFFFFFF)) >> uint64_t(n))
#define Sigma0(x)  (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define Sigma1(x)  (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define Gamma0(x)  (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define Gamma1(x)  (S(x, 19) ^ S(x, 61) ^ R(x, 6))


//----------------------------------------------------------------------------
// The K array
//----------------------------------------------------------------------------

namespace {
    const uint64_t K[80] = {
        TS_UCONST64(0x428a2f98d728ae22), TS_UCONST64(0x7137449123ef65cd),
        TS_UCONST64(0xb5c0fbcfec4d3b2f), TS_UCONST64(0xe9b5dba58189dbbc),
        TS_UCONST64(0x3956c25bf348b538), TS_UCONST64(0x59f111f1b605d019),
        TS_UCONST64(0x923f82a4af194f9b), TS_UCONST64(0xab1c5ed5da6d8118),
        TS_UCONST64(0xd807aa98a3030242), TS_UCONST64(0x12835b0145706fbe),
        TS_UCONST64(0x243185be4ee4b28c), TS_UCONST64(0x550c7dc3d5ffb4e2),
        TS_UCONST64(0x72be5d74f27b896f), TS_UCONST64(0x80deb1fe3b1696b1),
        TS_UCONST64(0x9bdc06a725c71235), TS_UCONST64(0xc19bf174cf692694),
        TS_UCONST64(0xe49b69c19ef14ad2), TS_UCONST64(0xefbe4786384f25e3),
        TS_UCONST64(0x0fc19dc68b8cd5b5), TS_UCONST64(0x240ca1cc77ac9c65),
        TS_UCONST64(0x2de92c6f592b0275), TS_UCONST64(0x4a7484aa6ea6e483),
        TS_UCONST64(0x5cb0a9dcbd41fbd4), TS_UCONST64(0x76f988da831153b5),
        TS_UCONST64(0x983e5152ee66dfab), TS_UCONST64(0xa831c66d2db43210),
        TS_UCONST64(0xb00327c898fb213f), TS_UCONST64(0xbf597fc7beef0ee4),
        TS_UCONST64(0xc6e00bf33da88fc2), TS_UCONST64(0xd5a79147930aa725),
        TS_UCONST64(0x06ca6351e003826f), TS_UCONST64(0x142929670a0e6e70),
        TS_UCONST64(0x27b70a8546d22ffc), TS_UCONST64(0x2e1b21385c26c926),
        TS_UCONST64(0x4d2c6dfc5ac42aed), TS_UCONST64(0x53380d139d95b3df),
        TS_UCONST64(0x650a73548baf63de), TS_UCONST64(0x766a0abb3c77b2a8),
        TS_UCONST64(0x81c2c92e47edaee6), TS_UCONST64(0x92722c851482353b),
        TS_UCONST64(0xa2bfe8a14cf10364), TS_UCONST64(0xa81a664bbc423001),
        TS_UCONST64(0xc24b8b70d0f89791), TS_UCONST64(0xc76c51a30654be30),
        TS_UCONST64(0xd192e819d6ef5218), TS_UCONST64(0xd69906245565a910),
        TS_UCONST64(0xf40e35855771202a), TS_UCONST64(0x106aa07032bbd1b8),
        TS_UCONST64(0x19a4c116b8d2d0c8), TS_UCONST64(0x1e376c085141ab53),
        TS_UCONST64(0x2748774cdf8eeb99), TS_UCONST64(0x34b0bcb5e19b48a8),
        TS_UCONST64(0x391c0cb3c5c95a63), TS_UCONST64(0x4ed8aa4ae3418acb),
        TS_UCONST64(0x5b9cca4f7763e373), TS_UCONST64(0x682e6ff3d6b2b8a3),
        TS_UCONST64(0x748f82ee5defb2fc), TS_UCONST64(0x78a5636f43172f60),
        TS_UCONST64(0x84c87814a1f0ab72), TS_UCONST64(0x8cc702081a6439ec),
        TS_UCONST64(0x90befffa23631e28), TS_UCONST64(0xa4506cebde82bde9),
        TS_UCONST64(0xbef9a3f7b2c67915), TS_UCONST64(0xc67178f2e372532b),
        TS_UCONST64(0xca273eceea26619c), TS_UCONST64(0xd186b8c721c0c207),
        TS_UCONST64(0xeada7dd6cde0eb1e), TS_UCONST64(0xf57d4f7fee6ed178),
        TS_UCONST64(0x06f067aa72176fba), TS_UCONST64(0x0a637dc5a2c898a6),
        TS_UCONST64(0x113f9804bef90dae), TS_UCONST64(0x1b710b35131c471b),
        TS_UCONST64(0x28db77f523047d84), TS_UCONST64(0x32caab7b40c72493),
        TS_UCONST64(0x3c9ebe0a15c9bebc), TS_UCONST64(0x431d67c49c100d4c),
        TS_UCONST64(0x4cc5d4becb3e42b6), TS_UCONST64(0x597f299cfc657e2a),
        TS_UCONST64(0x5fcb6fab3ad6faec), TS_UCONST64(0x6c44198c4a475817)
    };
}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SHA512::SHA512() :
    _length(0),
    _curlen(0)
{
    init();
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SHA512::init()
{
    _curlen = 0;
    _length = 0;
    _state[0] = TS_UCONST64(0x6a09e667f3bcc908);
    _state[1] = TS_UCONST64(0xbb67ae8584caa73b);
    _state[2] = TS_UCONST64(0x3c6ef372fe94f82b);
    _state[3] = TS_UCONST64(0xa54ff53a5f1d36f1);
    _state[4] = TS_UCONST64(0x510e527fade682d1);
    _state[5] = TS_UCONST64(0x9b05688c2b3e6c1f);
    _state[6] = TS_UCONST64(0x1f83d9abfb41bd6b);
    _state[7] = TS_UCONST64(0x5be0cd19137e2179);
    return true;
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::SHA512::compress(const uint8_t* buf)
{
    uint64_t S[8], W[80], t0, t1;

    /* copy state into S */
    for (size_t i = 0; i < 8; i++) {
        S[i] = _state[i];
    }

    /* copy the state into 1024-bits into W[0..15] */
    for (size_t i = 0; i < 16; i++) {
        W[i] = GetUInt64(buf + 8*i);
    }

    /* fill W[16..79] */
    for (size_t i = 16; i < 80; i++) {
        W[i] = Gamma1(W[i - 2]) + W[i - 7] + Gamma0(W[i - 15]) + W[i - 16];
    }

    /* Compress */
#define RND(a,b,c,d,e,f,g,h,i)                       \
    t0 = h + Sigma1(e) + Ch(e, f, g) + K[i] + W[i];  \
    t1 = Sigma0(a) + Maj(a, b, c);                   \
    d += t0;                                         \
    h  = t0 + t1

    for (size_t i = 0; i < 80; i += 8) {
        RND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
        RND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
        RND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
        RND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
        RND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
        RND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
        RND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
        RND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
     }

#undef RND

    /* feedback */
    for (size_t i = 0; i < 8; i++) {
        _state[i] = _state[i] + S[i];
    }
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::SHA512::add(const void* data, size_t size)
{
    const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
    size_t n;

    if (_curlen >= sizeof(_buf)) {
        return false;
    }
    while (size > 0) {
        if (_curlen == 0 && size >= BLOCK_SIZE) {
            compress(in);
            _length += BLOCK_SIZE * 8;
            in += BLOCK_SIZE;
            size -= BLOCK_SIZE;
        }
        else {
            n = std::min(size, (BLOCK_SIZE - _curlen));
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

bool ts::SHA512::getHash (void* hash, size_t bufsize, size_t* retsize)
{
    if (_curlen >= sizeof(_buf) || bufsize < HASH_SIZE) {
        return false;
    }

    /* increase the length of the message */
    _length += _curlen * 8;

    /* append the '1' bit */
    _buf[_curlen++] = 0x80;

    /* if the length is currently above 112 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (_curlen > 112) {
        while (_curlen < 128) {
            _buf[_curlen++] = 0;
        }
        compress (_buf);
        _curlen = 0;
    }

    /* pad upto 120 bytes of zeroes
     * note: that from 112 to 120 is the 64 MSB of the length.  We assume that you won't hash
     * > 2^64 bits of data... :-)
     */
    while (_curlen < 120) {
        _buf[_curlen++] = 0;
    }

    /* store length */
    PutUInt64 (_buf + 120, _length);
    compress (_buf);

    /* copy output */
    uint8_t* out = reinterpret_cast<uint8_t*> (hash);
    for (size_t i = 0; i < 8; i++) {
        PutUInt64 (out + 8*i, _state[i]);
    }

    if (retsize != nullptr) {
        *retsize = HASH_SIZE;
    }
    return true;
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
