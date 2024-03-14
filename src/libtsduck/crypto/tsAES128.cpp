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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::AES128, AES128, (u"AES-128", ts::AES128::BLOCK_SIZE, ts::AES128::KEY_SIZE));

ts::AES128::AES128() : BlockCipher(AES128::PROPERTIES())
{
}

ts::AES128::AES128(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES128::PROPERTIES());
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::AES128::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("AES-128-ECB"), Algo);

const EVP_CIPHER* ts::AES128::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
