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
// Basic implementation (one block)
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::TDES, TDES, (u"TDES", ts::TDES::BLOCK_SIZE, ts::TDES::KEY_SIZE));

ts::TDES::TDES() : BlockCipher(TDES::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::TDES::TDES(const BlockCipherProperties& props) : BlockCipher(props)
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    props.assertCompatibleBase(TDES::PROPERTIES());
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB), FetchECB);
void ts::TDES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB::Instance().getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("DES-EDE3-ECB"), Algo);
const EVP_CIPHER* ts::TDES::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::ECB<ts::TDES>, ECB, (ts::TDES::PROPERTIES(), u"ECB", false, ts::TDES::BLOCK_SIZE, 0, 0));
ts::ECB<ts::TDES>::ECB() : TDES(ts::ECB<ts::TDES>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::TDES>::ECB(const BlockCipherProperties& props) : TDES(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::TDES>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::TDES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchECB::Instance().getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#else

const EVP_CIPHER* ts::ECB<ts::TDES>::getAlgorithm() const
{
    return Algo::Instance().algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::CBC<ts::TDES>, CBC, (ts::TDES::PROPERTIES(), u"CBC", false, ts::TDES::BLOCK_SIZE, 0, ts::TDES::BLOCK_SIZE));
ts::CBC<ts::TDES>::CBC() : TDES(ts::CBC<ts::TDES>::PROPERTIES())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::TDES>::CBC(const BlockCipherProperties& props) : TDES(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::TDES>::PROPERTIES());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

TS_STATIC_INSTANCE(ts::FetchBCryptAlgorithm, (BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC), FetchCBC);
void ts::CBC<ts::TDES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    FetchCBC::Instance().getAlgorithm(algo, length);
    ignore_iv = false;
}

#else

TS_STATIC_INSTANCE(ts::FetchCipherAlgorithm, ("DES-EDE3-CBC"), AlgoCBC);
const EVP_CIPHER* ts::CBC<ts::TDES>::getAlgorithm() const
{
    return AlgoCBC::Instance().algorithm();
}

#endif
