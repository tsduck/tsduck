//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTDES.h"
#include "tsFetchAlgorithm.h"


//----------------------------------------------------------------------------
// Basic implementation (one block)
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::TDES::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(u"TDES", ts::TDES::BLOCK_SIZE, ts::TDES::KEY_SIZE);
    return props;
}

ts::TDES::TDES() : BlockCipher(TDES::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::TDES::TDES(const BlockCipherProperties& props) : BlockCipher(props)
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    props.assertCompatibleBase(TDES::Properties());
    canProcessInPlace(true);
}

ts::TDES::~TDES()
{
}


//----------------------------------------------------------------------------
// Implementation using external cryptographic libraries.
//----------------------------------------------------------------------------

#if !defined(TS_NO_CRYPTO_LIBRARY)

#if defined(TS_WINDOWS)

void ts::TDES::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::TDES::getAlgorithm() const
{
    // Unlike other OpenSSL algorithms, TDES in ECB mode is named DES-EDE3, without -ECB suffix.
    // Some implementations have an alias from DES-EDE3-ECB to DES-EDE3, but no all.
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-EDE3");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::ECB<ts::TDES>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::TDES::Properties(), u"ECB", false, ts::TDES::BLOCK_SIZE, 0, 0);
    return props;
}

ts::ECB<ts::TDES>::ECB() : TDES(ts::ECB<ts::TDES>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::TDES>::ECB(const BlockCipherProperties& props) : TDES(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::TDES>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::ECB<ts::TDES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_ECB);
    fetch.getAlgorithm(algo, length);
    // This is ECB mode, ignore IV which may be used by a upper chaining mode.
    ignore_iv = true;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::ECB<ts::TDES>::getAlgorithm() const
{
    // Unlike other OpenSSL algorithms, TDES in ECB mode is named DES-EDE3, without -ECB suffix.
    // Some implementations have an alias from DES-EDE3-ECB to DES-EDE3, but no all.
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-EDE3");
    return fetch.algorithm();
}

#endif


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::CBC<ts::TDES>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::TDES::Properties(), u"CBC", false, ts::TDES::BLOCK_SIZE, 0, ts::TDES::BLOCK_SIZE);
    return props;
}

ts::CBC<ts::TDES>::CBC() : TDES(ts::CBC<ts::TDES>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::TDES>::CBC(const BlockCipherProperties& props) : TDES(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::TDES>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if defined(TS_WINDOWS)

void ts::CBC<ts::TDES>::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    // Thread-safe init-safe static data pattern:
    static const FetchBCryptAlgorithm fetch(BCRYPT_3DES_ALGORITHM, BCRYPT_CHAIN_MODE_CBC);
    fetch.getAlgorithm(algo, length);
    ignore_iv = false;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::CBC<ts::TDES>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("DES-EDE3-CBC");
    return fetch.algorithm();
}

#endif

#endif // TS_NO_CRYPTO_LIBRARY
