//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAES128.h"
#include "tsFetchAlgorithm.h"


//----------------------------------------------------------------------------
// Basic implementation (one block)
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::AES128::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(u"AES-128", ts::AES128::BLOCK_SIZE, ts::AES128::KEY_SIZE);
    return props;
}

ts::AES128::AES128() : BlockCipher(AES128::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES128::AES128(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(AES128::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::AES128::~AES128()
{
}


//----------------------------------------------------------------------------
// Implementation using external cryptographic libraries.
//----------------------------------------------------------------------------

#if !defined(TS_NO_CRYPTO_LIBRARY)

#if defined(TS_WINDOWS)

void ts::AES128::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::AES128::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-128-ECB");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::ECB<ts::AES128>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::AES128::Properties(), u"ECB", false, ts::AES128::BLOCK_SIZE, 0, 0);
    return props;
}

ts::ECB<ts::AES128>::ECB() : AES128(ts::ECB<ts::AES128>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::AES128>::ECB(const BlockCipherProperties& props) : AES128(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::AES128>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::AES128>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::ECB<ts::AES128>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-128-ECB");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::CBC<ts::AES128>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::AES128::Properties(), u"CBC", false, ts::AES128::BLOCK_SIZE, 0, ts::AES128::BLOCK_SIZE);
    return props;
}

ts::CBC<ts::AES128>::CBC() : AES128(ts::CBC<ts::AES128>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::AES128>::CBC(const BlockCipherProperties& props) : AES128(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::AES128>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::CBC<ts::AES128>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_AES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC);
    fetch.getAlgorithm(algo, length);
    ignore_iv = false;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::CBC<ts::AES128>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("AES-128-CBC");
    return fetch.algorithm();
}

#endif

#endif // TS_NO_CRYPTO_LIBRARY
