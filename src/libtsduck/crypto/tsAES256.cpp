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
// Basic implementation (one block)
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::AES256, AES256, (u"AES-256", ts::AES256::BLOCK_SIZE, ts::AES256::KEY_SIZE));

ts::AES256::AES256() : BlockCipher(AES256::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES256::AES256(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES256::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchECB, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB));

void ts::AES256::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB->getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, Algo, ("AES-256-ECB"), OPENSSL_atexit);

const EVP_CIPHER* ts::AES256::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::ECB<ts::AES256>, ECB, (ts::AES256::PROPERTIES(), u"ECB", false, ts::AES256::BLOCK_SIZE, 0, 0));
ts::ECB<ts::AES256>::ECB() : AES256(ts::ECB<ts::AES256>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::AES256>::ECB(const BlockCipherProperties& props) : AES256(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::AES256>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::AES256>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB->getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

const EVP_CIPHER* ts::ECB<ts::AES256>::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

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

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchCBC, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC));

void ts::CBC<ts::AES256>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchCBC->getAlgorithm(algo, length);
    ignore_iv = false;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, AlgoCBC, ("AES-256-CBC"), OPENSSL_atexit);

const EVP_CIPHER* ts::CBC<ts::AES256>::getAlgorithm() const
{
    return AlgoCBC->algorithm();
}

#endif
