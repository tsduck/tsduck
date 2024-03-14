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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::AES256, AES256, (u"AES-256", ts::AES256::BLOCK_SIZE, ts::AES256::KEY_SIZE));

ts::AES256::AES256() : BlockCipher(AES256::PROPERTIES())
{
}

ts::AES256::AES256(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES256::PROPERTIES());
}

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
