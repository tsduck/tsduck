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
// Basic implementation (one block)
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::AES128, AES128, (u"AES-128", ts::AES128::BLOCK_SIZE, ts::AES128::KEY_SIZE));

ts::AES128::AES128() : BlockCipher(AES128::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES128::AES128(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES128::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchECB, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB));

void ts::AES128::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB->getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, Algo, ("AES-128-ECB"), OPENSSL_atexit);

const EVP_CIPHER* ts::AES128::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::ECB<ts::AES128>, ECB, (ts::AES128::PROPERTIES(), u"ECB", false, ts::AES128::BLOCK_SIZE, 0, 0));
ts::ECB<ts::AES128>::ECB() : AES128(ts::ECB<ts::AES128>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::AES128>::ECB(const BlockCipherProperties& props) : AES128(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::AES128>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::AES128>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB::Instance().getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

const EVP_CIPHER* ts::ECB<ts::AES128>::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::CBC<ts::AES128>, CBC, (ts::AES128::PROPERTIES(), u"CBC", false, ts::AES128::BLOCK_SIZE, 0, ts::AES128::BLOCK_SIZE));
ts::CBC<ts::AES128>::CBC() : AES128(ts::CBC<ts::AES128>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::AES128>::CBC(const BlockCipherProperties& props) : AES128(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::AES128>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchCBC, (BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC));

void ts::CBC<ts::AES128>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchCBC->getAlgorithm(algo, length);
    ignore_iv = false;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, AlgoCBC, ("AES-128-CBC"), OPENSSL_atexit);

const EVP_CIPHER* ts::CBC<ts::AES128>::getAlgorithm() const
{
    return AlgoCBC->algorithm();
}

#endif
