//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTDES.h"
#include "tsSingleton.h"
#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Implementation of BlockCipher interface:
//----------------------------------------------------------------------------

ts::TDES::TDES()
{
    InitCryptographicLibrary();
}

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

bool ts::TDES::isValidKeySize(size_t size) const
{
    return size == KEY_SIZE;
}


//----------------------------------------------------------------------------
// System-specific implementation.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::TDES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::PresetCipherAlgorithm, ("DES-EDE3-ECB"), Algo);

const EVP_CIPHER* ts::TDES::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
