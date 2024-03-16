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
    canProcessInPlace(true);
}

ts::AES256::AES256(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES256::PROPERTIES());
    canProcessInPlace(true);
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

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::ECB<ts::AES256>, ECB, (ts::AES256::PROPERTIES(), u"ECB", false, ts::AES256::BLOCK_SIZE, 0, 0));
ts::ECB<ts::AES256>::ECB() : AES256(ts::ECB<ts::AES256>::PROPERTIES())
{
    canProcessInPlace(true);
}
ts::ECB<ts::AES256>::ECB(const BlockCipherProperties& props) : AES256(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::AES256>::PROPERTIES());
    canProcessInPlace(true);
}
const EVP_CIPHER* ts::ECB<ts::AES256>::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}
#endif

// Specialization for AES-128-CBC is currently disabled, see:
// https://stackoverflow.com/questions/78172656/openssl-how-to-encrypt-new-message-with-same-key-without-evp-encryptinit-ex-a
#if 0
TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::CBC<ts::AES256>, CBC, (ts::AES256::PROPERTIES(), u"CBC", false, ts::AES256::BLOCK_SIZE, 0, ts::AES256::BLOCK_SIZE));
ts::CBC<ts::AES256>::CBC() : AES256(ts::CBC<ts::AES256>::PROPERTIES())
{
    canProcessInPlace(true);
}
ts::CBC<ts::AES256>::CBC(const BlockCipherProperties& props) : AES256(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::AES256>::PROPERTIES());
    canProcessInPlace(true);
}
TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("AES-256-CBC"), AlgoCBC);
const EVP_CIPHER* ts::CBC<ts::AES256>::getAlgorithm() const
{
    return AlgoCBC::Instance().algorithm();
}

#endif
