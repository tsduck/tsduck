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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::TDES, TDES, (u"TDES", ts::TDES::BLOCK_SIZE, ts::TDES::KEY_SIZE));

ts::TDES::TDES() : BlockCipher(TDES::PROPERTIES())
{
}

ts::TDES::TDES(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(TDES::PROPERTIES());
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), Fetch);

void ts::TDES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    Fetch::Instance().getAlgorithm(algo, length);
}

#else

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("DES-EDE3-ECB"), Algo);

const EVP_CIPHER* ts::TDES::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif
