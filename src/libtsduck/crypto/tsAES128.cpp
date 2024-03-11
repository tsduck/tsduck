//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAES128.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::AES128::AES128()
{
    InitCryptographicLibrary();
}

ts::UString ts::AES128::name() const
{
    return u"AES-128";
}

size_t ts::AES128::blockSize() const
{
    return BLOCK_SIZE;
}

size_t ts::AES128::minKeySize() const
{
    return KEY_SIZE;
}

size_t ts::AES128::maxKeySize() const
{
    return KEY_SIZE;
}

bool ts::AES128::isValidKeySize (size_t size) const
{
    return size == KEY_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::AES128::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::PresetCipherAlgorithm, ("AES-128-ECB"), Algo);

const EVP_CIPHER* ts::AES128::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
