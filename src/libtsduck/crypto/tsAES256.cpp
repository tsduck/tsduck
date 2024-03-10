//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAES256.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::AES256::AES256()
{
    InitCryptographicLibrary();
}

ts::UString ts::AES256::name() const
{
    return u"AES-256";
}

size_t ts::AES256::blockSize() const
{
    return BLOCK_SIZE;
}

size_t ts::AES256::minKeySize() const
{
    return KEY_SIZE;
}

size_t ts::AES256::maxKeySize() const
{
    return KEY_SIZE;
}

bool ts::AES256::isValidKeySize (size_t size) const
{
    return size == KEY_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::AES256::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

//----------------------------------------------------------------------------
// Schedule a new key. If rounds is zero, the default is used.
//----------------------------------------------------------------------------

bool ts::AES256::setKeyImpl(const void* key_data, size_t key_length)
{
    if (key_length != KEY_SIZE) {
        return false;
    }
    return _aes.setKey(key_data, key_length);
}


//----------------------------------------------------------------------------
// Encryption in ECB mode.
//----------------------------------------------------------------------------

bool ts::AES256::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (plain_length != BLOCK_SIZE || cipher_maxsize < BLOCK_SIZE) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = BLOCK_SIZE;
    }
    return _aes.encrypt(plain, plain_length, cipher, cipher_maxsize, cipher_length);
}


//----------------------------------------------------------------------------
// Decryption in ECB mode.
//----------------------------------------------------------------------------

bool ts::AES256::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (cipher_length != BLOCK_SIZE || plain_maxsize < BLOCK_SIZE) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = BLOCK_SIZE;
    }
    return _aes.decrypt(cipher, cipher_length, plain, plain_maxsize, plain_length);
}

#endif
