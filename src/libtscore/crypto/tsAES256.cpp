//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAES256.h"
#include "tsFetchAlgorithm.h"


//----------------------------------------------------------------------------
// Basic implementation (one block)
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::AES256::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(u"AES-256", ts::AES256::BLOCK_SIZE, ts::AES256::KEY_SIZE);
    return props;
}

ts::AES256::AES256() : BlockCipher(AES256::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES256::AES256(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES256::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES256::~AES256()
{
}


//----------------------------------------------------------------------------
// Implementation using external cryptographic libraries.
//----------------------------------------------------------------------------

#if !defined(TS_NO_CRYPTO_LIBRARY)

#if defined(TS_WINDOWS)

void ts::AES256::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::AES256::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-256-ECB");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::ECB<ts::AES256>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::AES256::Properties(), u"ECB", false, ts::AES256::BLOCK_SIZE, 0, 0);
    return props;
}

ts::ECB<ts::AES256>::ECB() : AES256(ts::ECB<ts::AES256>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::AES256>::ECB(const BlockCipherProperties& props) : AES256(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::AES256>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::AES256>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::ECB<ts::AES256>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-256-ECB");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::CBC<ts::AES256>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::AES256::Properties(), u"CBC", false, ts::AES256::BLOCK_SIZE, 0, ts::AES256::BLOCK_SIZE);
    return props;
}

ts::CBC<ts::AES256>::CBC() : AES256(ts::CBC<ts::AES256>::Properties())
{
    canProcessInPlace(true);
}

ts::CBC<ts::AES256>::CBC(const BlockCipherProperties& props) : AES256(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::AES256>::Properties());
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::CBC<ts::AES256>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC);
    fetch.getAlgorithm(algo, length);
    ignore_iv = false;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::CBC<ts::AES256>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-256-CBC");
    return fetch.algorithm();
}

#endif

#endif // TS_NO_CRYPTO_LIBRARY
