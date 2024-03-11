//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDES.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::DES::DES()
{
    InitCryptographicLibrary();
}

ts::UString ts::DES::name() const
{
    return u"DES";
}

size_t ts::DES::blockSize() const
{
    return BLOCK_SIZE;
}

size_t ts::DES::minKeySize() const
{
    return KEY_SIZE;
}

size_t ts::DES::maxKeySize() const
{
    return KEY_SIZE;
}

bool ts::DES::isValidKeySize(size_t size) const
{
    return size == KEY_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::DES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::PresetCipherAlgorithm, ("DES-ECB", "provider=legacy"), Algo);

const EVP_CIPHER* ts::DES::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
