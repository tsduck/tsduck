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

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("AES-256-ECB"), Algo);

const EVP_CIPHER* ts::AES256::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
