//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDES.h"
#include "tsFetchAlgorithm.h"


//----------------------------------------------------------------------------
// Basic implementation (one block)
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::DES::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(u"DES", ts::DES::BLOCK_SIZE, ts::DES::KEY_SIZE);
    return props;
}

ts::DES::DES() : BlockCipher(DES::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::DES::DES(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(DES::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::DES::~DES()
{
}


//----------------------------------------------------------------------------
// Implementation using external cryptographic libraries.
//----------------------------------------------------------------------------

#if !defined(TS_NO_CRYPTO_LIBRARY)

#if defined(TS_WINDOWS)

void ts::DES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::DES::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-ECB", "legacy");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::ECB<ts::DES>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::DES::Properties(), u"ECB", false, ts::DES::BLOCK_SIZE, 0, 0);
    return props;
}

ts::ECB<ts::DES>::ECB() : DES(ts::ECB<ts::DES>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::DES>::ECB(const BlockCipherProperties& props) : DES(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::DES>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::DES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::ECB<ts::DES>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-ECB", "legacy");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::CBC<ts::DES>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::DES::Properties(), u"CBC", false, ts::DES::BLOCK_SIZE, 0, ts::DES::BLOCK_SIZE);
    return props;
}

ts::CBC<ts::DES>::CBC() : DES(ts::CBC<ts::DES>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::DES>::CBC(const BlockCipherProperties& props) : DES(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::DES>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::CBC<ts::DES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC);
    fetch.getAlgorithm(algo, length);
    ignore_iv = false;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::CBC<ts::DES>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-CBC", "legacy");
    return fetch.algorithm();
}

#endif

#endif // TS_NO_CRYPTO_LIBRARY
