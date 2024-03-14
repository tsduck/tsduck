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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::DES, DES, (u"DES", ts::DES::BLOCK_SIZE, ts::DES::KEY_SIZE));

ts::DES::DES() : BlockCipher(DES::PROPERTIES())
{
}

ts::DES::DES(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(DES::PROPERTIES());
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::DES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("DES-ECB", "legacy"), Algo);

const EVP_CIPHER* ts::DES::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
