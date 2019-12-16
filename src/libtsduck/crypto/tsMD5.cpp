//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  MD-5 hash.
//
//  Implementation based on LibTomCrypt (http://www.libtom.org/)
//  by Tom St Denis (tomstdenis@gmail.com)
//
//  Usage in TSDuck allowed based on LibTomCrypt licence:
//    << LibTomCrypt is public domain. The library is free for >>
//    << all purposes without any express guarantee it works.  >>
//
//----------------------------------------------------------------------------

#include "tsMD5.h"
#include "tsMemory.h"
TSDUCK_SOURCE;

#define F(x,y,z)  (z ^ (x & (y ^ z)))
#define G(x,y,z)  (y ^ (z & (y ^ x)))
#define H(x,y,z)  (x^y^z)
#define I(x,y,z)  (y^(x|(~z)))

#define FF(a,b,c,d,M,s,t) a = (a + F(b,c,d) + M + t); a = (ROLc(a, s) + b)
#define GG(a,b,c,d,M,s,t) a = (a + G(b,c,d) + M + t); a = (ROLc(a, s) + b)
#define HH(a,b,c,d,M,s,t) a = (a + H(b,c,d) + M + t); a = (ROLc(a, s) + b)
#define II(a,b,c,d,M,s,t) a = (a + I(b,c,d) + M + t); a = (ROLc(a, s) + b)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MD5::MD5() :
    _length(0),
    _curlen(0)
{
    init();
}


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::MD5::init()
{
    _state[0] = 0x67452301UL;
    _state[1] = 0xEFCDAB89UL;
    _state[2] = 0x98BADCFEUL;
    _state[3] = 0x10325476UL;
    _curlen = 0;
    _length = 0;
    return true;
}


//----------------------------------------------------------------------------
// Compress part of message
//----------------------------------------------------------------------------

void ts::MD5::compress (const uint8_t* buf)
{
    uint32_t i, W[16], a, b, c, d;

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        W[i] = GetUInt32LE (buf + 4*i);
    }

    /* copy state */
    a = _state[0];
    b = _state[1];
    c = _state[2];
    d = _state[3];

    FF(a,b,c,d,W[0],  7,0xd76aa478UL);
    FF(d,a,b,c,W[1], 12,0xe8c7b756UL);
    FF(c,d,a,b,W[2], 17,0x242070dbUL);
    FF(b,c,d,a,W[3], 22,0xc1bdceeeUL);
    FF(a,b,c,d,W[4],  7,0xf57c0fafUL);
    FF(d,a,b,c,W[5], 12,0x4787c62aUL);
    FF(c,d,a,b,W[6], 17,0xa8304613UL);
    FF(b,c,d,a,W[7], 22,0xfd469501UL);
    FF(a,b,c,d,W[8],  7,0x698098d8UL);
    FF(d,a,b,c,W[9], 12,0x8b44f7afUL);
    FF(c,d,a,b,W[10],17,0xffff5bb1UL);
    FF(b,c,d,a,W[11],22,0x895cd7beUL);
    FF(a,b,c,d,W[12], 7,0x6b901122UL);
    FF(d,a,b,c,W[13],12,0xfd987193UL);
    FF(c,d,a,b,W[14],17,0xa679438eUL);
    FF(b,c,d,a,W[15],22,0x49b40821UL);
    GG(a,b,c,d,W[1],  5,0xf61e2562UL);
    GG(d,a,b,c,W[6],  9,0xc040b340UL);
    GG(c,d,a,b,W[11],14,0x265e5a51UL);
    GG(b,c,d,a,W[0], 20,0xe9b6c7aaUL);
    GG(a,b,c,d,W[5],  5,0xd62f105dUL);
    GG(d,a,b,c,W[10], 9,0x02441453UL);
    GG(c,d,a,b,W[15],14,0xd8a1e681UL);
    GG(b,c,d,a,W[4], 20,0xe7d3fbc8UL);
    GG(a,b,c,d,W[9],  5,0x21e1cde6UL);
    GG(d,a,b,c,W[14], 9,0xc33707d6UL);
    GG(c,d,a,b,W[3], 14,0xf4d50d87UL);
    GG(b,c,d,a,W[8], 20,0x455a14edUL);
    GG(a,b,c,d,W[13], 5,0xa9e3e905UL);
    GG(d,a,b,c,W[2],  9,0xfcefa3f8UL);
    GG(c,d,a,b,W[7], 14,0x676f02d9UL);
    GG(b,c,d,a,W[12],20,0x8d2a4c8aUL);
    HH(a,b,c,d,W[5],  4,0xfffa3942UL);
    HH(d,a,b,c,W[8], 11,0x8771f681UL);
    HH(c,d,a,b,W[11],16,0x6d9d6122UL);
    HH(b,c,d,a,W[14],23,0xfde5380cUL);
    HH(a,b,c,d,W[1],  4,0xa4beea44UL);
    HH(d,a,b,c,W[4], 11,0x4bdecfa9UL);
    HH(c,d,a,b,W[7], 16,0xf6bb4b60UL);
    HH(b,c,d,a,W[10],23,0xbebfbc70UL);
    HH(a,b,c,d,W[13], 4,0x289b7ec6UL);
    HH(d,a,b,c,W[0], 11,0xeaa127faUL);
    HH(c,d,a,b,W[3], 16,0xd4ef3085UL);
    HH(b,c,d,a,W[6], 23,0x04881d05UL);
    HH(a,b,c,d,W[9],  4,0xd9d4d039UL);
    HH(d,a,b,c,W[12],11,0xe6db99e5UL);
    HH(c,d,a,b,W[15],16,0x1fa27cf8UL);
    HH(b,c,d,a,W[2], 23,0xc4ac5665UL);
    II(a,b,c,d,W[0],  6,0xf4292244UL);
    II(d,a,b,c,W[7], 10,0x432aff97UL);
    II(c,d,a,b,W[14],15,0xab9423a7UL);
    II(b,c,d,a,W[5], 21,0xfc93a039UL);
    II(a,b,c,d,W[12], 6,0x655b59c3UL);
    II(d,a,b,c,W[3], 10,0x8f0ccc92UL);
    II(c,d,a,b,W[10],15,0xffeff47dUL);
    II(b,c,d,a,W[1], 21,0x85845dd1UL);
    II(a,b,c,d,W[8],  6,0x6fa87e4fUL);
    II(d,a,b,c,W[15],10,0xfe2ce6e0UL);
    II(c,d,a,b,W[6], 15,0xa3014314UL);
    II(b,c,d,a,W[13],21,0x4e0811a1UL);
    II(a,b,c,d,W[4],  6,0xf7537e82UL);
    II(d,a,b,c,W[11],10,0xbd3af235UL);
    II(c,d,a,b,W[2], 15,0x2ad7d2bbUL);
    II(b,c,d,a,W[9], 21,0xeb86d391UL);

    /* store */
    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::MD5::add (const void* data, size_t size)
{
    const uint8_t* in = reinterpret_cast<const uint8_t*> (data);
    size_t n;

    if (_curlen >= sizeof(_buf)) {
        return false;
    }
    while (size > 0) {
        if (_curlen == 0 && size >= BLOCK_SIZE) {
            compress (in);
            _length += BLOCK_SIZE * 8;
            in += BLOCK_SIZE;
            size -= BLOCK_SIZE;
        }
        else {
            n = std::min (size, (BLOCK_SIZE - _curlen));
            ::memcpy(_buf + _curlen, in, n);
            _curlen += n;
            in += n;
            size -= n;
            if (_curlen == BLOCK_SIZE) {
                compress (_buf);
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

bool ts::MD5::getHash (void* hash, size_t bufsize, size_t* retsize)
{
    if (_curlen >= sizeof(_buf) || bufsize < HASH_SIZE) {
        return false;
    }

    /* increase the length of the message */
    _length += _curlen * 8;

    /* append the '1' bit */
    _buf[_curlen++] = 0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (_curlen > 56) {
        while (_curlen < 64) {
            _buf[_curlen++] = 0;
        }
        compress (_buf);
        _curlen = 0;
    }

    /* pad up to 56 bytes of zeroes */
    while (_curlen < 56) {
        _buf[_curlen++] = 0;
    }

    /* store length */
    PutUInt64LE (_buf + 56, _length);
    compress (_buf);

    /* copy output */
    uint8_t* out = reinterpret_cast<uint8_t*> (hash);
    for (size_t i = 0; i < 4; i++) {
        PutUInt32LE (out + 4*i, _state[i]);
    }

    if (retsize != nullptr) {
        *retsize = HASH_SIZE;
    }
    return true;
}


//----------------------------------------------------------------------------
// Implementation of Hash interface:
//----------------------------------------------------------------------------

ts::UString ts::MD5::name() const
{
    return u"MD-5";
}
size_t ts::MD5::hashSize() const
{
    return HASH_SIZE;
}
size_t ts::MD5::blockSize() const
{
    return BLOCK_SIZE;
}
