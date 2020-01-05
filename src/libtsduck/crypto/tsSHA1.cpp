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
TSDUCK_SOURCE;

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SHA1::SHA1() :
    _length(0),
    _curlen(0)
{
    init();
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
    uint32_t a,b,c,d,e,W[80],i;

    // Copy the state into 512-bits into W[0..15]
    for (i = 0; i < 16; i++) {
        W[i] = GetUInt32 (buf + 4*i);
    }

    // Copy state
    a = _state[0];
    b = _state[1];
    c = _state[2];
    d = _state[3];
    e = _state[4];

    // Expand it
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }

    // Compress
    // Round one
    #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30)
    #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30)
    #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30)
    #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30)

    for (i = 0; i < 20; ) {
        FF0(a,b,c,d,e,i++);
        FF0(e,a,b,c,d,i++);
        FF0(d,e,a,b,c,i++);
        FF0(c,d,e,a,b,i++);
        FF0(b,c,d,e,a,i++);
    }

    // Round two
    for (; i < 40; )  {
        FF1(a,b,c,d,e,i++);
        FF1(e,a,b,c,d,i++);
        FF1(d,e,a,b,c,i++);
        FF1(c,d,e,a,b,i++);
        FF1(b,c,d,e,a,i++);
    }

    // Round three
    for (; i < 60; )  {
        FF2(a,b,c,d,e,i++);
        FF2(e,a,b,c,d,i++);
        FF2(d,e,a,b,c,i++);
        FF2(c,d,e,a,b,i++);
        FF2(b,c,d,e,a,i++);
    }

    // Round four
    for (; i < 80; )  {
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
    const uint8_t* in = reinterpret_cast<const uint8_t*>(data);
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
            n = std::min(size, (BLOCK_SIZE - _curlen));
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

bool ts::SHA1::getHash (void* hash, size_t bufsize, size_t* retsize)
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

    /* pad upto 56 bytes of zeroes */
    while (_curlen < 56) {
        _buf[_curlen++] = 0;
    }

    /* store length */
    PutUInt64 (_buf + 56, _length);
    compress (_buf);

    /* copy output */
    uint8_t* out = reinterpret_cast<uint8_t*> (hash);
    for (size_t i = 0; i < 5; i++) {
        PutUInt32 (out + 4*i, _state[i]);
    }

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
