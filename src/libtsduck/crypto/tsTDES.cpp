//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//
//  Triple-DES (EDE) block cipher
//
//  Implementation based on LibTomCrypt (http://www.libtom.org/)
//  by Tom St Denis (tomstdenis@gmail.com)
//
//  Usage in TSDuck allowed based on LibTomCrypt licence:
//    << LibTomCrypt is public domain. The library is free for >>
//    << all purposes without any express guarantee it works.  >>
//
//----------------------------------------------------------------------------

#include "tsTDES.h"
#include "tsDES.h"


//----------------------------------------------------------------------------
// Schedule a new key. If rounds is zero, the default is used.
//----------------------------------------------------------------------------

bool ts::TDES::setKeyImpl(const void* key, size_t key_length, size_t rounds)
{
    if (key_length != KEY_SIZE || (rounds != 0 && rounds != ROUNDS)) {
        return false;
    }

    const uint8_t* k = reinterpret_cast<const uint8_t*>(key);

    DES::deskey(k,    DES::EN0, _ek[0]);
    DES::deskey(k+8,  DES::DE1, _ek[1]);
    DES::deskey(k+16, DES::EN0, _ek[2]);

    DES::deskey(k,    DES::DE1, _dk[2]);
    DES::deskey(k+8,  DES::EN0, _dk[1]);
    DES::deskey(k+16, DES::DE1, _dk[0]);

    return true;
}


//----------------------------------------------------------------------------
// Encryption in ECB mode.
//----------------------------------------------------------------------------

bool ts::TDES::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (plain_length != BLOCK_SIZE || cipher_maxsize < BLOCK_SIZE) {
        return false;
    }

    uint32_t work[2];
    work[0] = GetUInt32(plain);
    work[1] = GetUInt32(reinterpret_cast<const uint8_t*> (plain) + 4);

    DES::desfunc(work, _ek[0]);
    DES::desfunc(work, _ek[1]);
    DES::desfunc(work, _ek[2]);

    PutUInt32(cipher, work[0]);
    PutUInt32(reinterpret_cast<uint8_t*> (cipher) + 4, work[1]);

    if (cipher_length != nullptr) {
        *cipher_length = BLOCK_SIZE;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in ECB mode.
//----------------------------------------------------------------------------

bool ts::TDES::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (cipher_length != BLOCK_SIZE || plain_maxsize < BLOCK_SIZE) {
        return false;
    }

    uint32_t work[2];
    work[0] = GetUInt32(cipher);
    work[1] = GetUInt32(reinterpret_cast<const uint8_t*>(cipher) + 4);

    DES::desfunc(work, _dk[0]);
    DES::desfunc(work, _dk[1]);
    DES::desfunc(work, _dk[2]);

    PutUInt32(plain, work[0]);
    PutUInt32(reinterpret_cast<uint8_t*> (plain) + 4, work[1]);

    if (plain_length != nullptr) {
        *plain_length = BLOCK_SIZE;
    }

    return true;
}


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::UString ts::TDES::name() const
{
    return u"TDES";
}
size_t ts::TDES::blockSize() const
{
    return BLOCK_SIZE;
}
size_t ts::TDES::minKeySize() const
{
    return KEY_SIZE;
}
size_t ts::TDES::maxKeySize() const
{
    return KEY_SIZE;
}
bool ts::TDES::isValidKeySize (size_t size) const
{
    return size == KEY_SIZE;
}
size_t ts::TDES::minRounds() const
{
    return ROUNDS;
}
size_t ts::TDES::maxRounds() const
{
    return ROUNDS;
}
size_t ts::TDES::defaultRounds() const
{
    return ROUNDS;
}
