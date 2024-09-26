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
// Basic implementation (one block)
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::DES, DES, (u"DES", ts::DES::BLOCK_SIZE, ts::DES::KEY_SIZE));

ts::DES::DES() : BlockCipher(DES::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::DES::DES(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(DES::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchECB, (BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB));

void ts::DES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB->getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, Algo, ("DES-ECB", "legacy"), OPENSSL_atexit);

const EVP_CIPHER* ts::DES::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::ECB<ts::DES>, ECB, (ts::DES::PROPERTIES(), u"ECB", false, ts::DES::BLOCK_SIZE, 0, 0));
ts::ECB<ts::DES>::ECB() : DES(ts::ECB<ts::DES>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::DES>::ECB(const BlockCipherProperties& props) : DES(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::DES>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::DES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB->getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

const EVP_CIPHER* ts::ECB<ts::DES>::getAlgorithm() const
{
    return Algo->algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::CBC<ts::DES>, CBC, (ts::DES::PROPERTIES(), u"CBC", false, ts::DES::BLOCK_SIZE, 0, ts::DES::BLOCK_SIZE));
ts::CBC<ts::DES>::CBC() : DES(ts::CBC<ts::DES>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::DES>::CBC(const BlockCipherProperties& props) : DES(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::DES>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(, ts::FetchBCryptAlgorithm, FetchCBC, (BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC));

void ts::CBC<ts::DES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchCBC->getAlgorithm(algo, length);
    ignore_iv = false;
}

#else

// The singleton needs to be destroyed no later that OpenSSL cleanup.
TS_STATIC_INSTANCE_ATEXIT(const, ts::FetchCipherAlgorithm, AlgoCBC, ("DES-CBC", "legacy"), OPENSSL_atexit);

const EVP_CIPHER* ts::CBC<ts::DES>::getAlgorithm() const
{
    return AlgoCBC->algorithm();
}

#endif
