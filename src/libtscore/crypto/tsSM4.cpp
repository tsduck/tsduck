//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSM4.h"
#include "tsFetchAlgorithm.h"


//----------------------------------------------------------------------------
// Basic implementation (one block)
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::SM4::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(u"SM4", ts::SM4::BLOCK_SIZE, ts::SM4::KEY_SIZE);
    return props;
}

ts::SM4::SM4() : BlockCipher(SM4::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::SM4::SM4(const BlockCipherProperties& props) : BlockCipher(props)
{
    props.assertCompatibleBase(SM4::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::SM4::~SM4()
{
}


//----------------------------------------------------------------------------
// Implementation using external cryptographic libraries.
//----------------------------------------------------------------------------

#if !defined(TS_NO_CRYPTO_LIBRARY)

#if !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::SM4::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("SM4-ECB");
    return fetch.algorithm();
}

#endif // TS_NO_OPENSSL


//----------------------------------------------------------------------------
// Template specialization for ECB mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::ECB<ts::SM4>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::SM4::Properties(), u"ECB", false, ts::SM4::BLOCK_SIZE, 0, 0);
    return props;
}

ts::ECB<ts::SM4>::ECB() : SM4(ts::ECB<ts::SM4>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::ECB<ts::SM4>::ECB(const BlockCipherProperties& props) : SM4(props)
{
    props.assertCompatibleChaining(ts::ECB<ts::SM4>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}


#if !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::ECB<ts::SM4>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("SM4-ECB");
    return fetch.algorithm();
}

#endif // TS_NO_OPENSSL


//----------------------------------------------------------------------------
// Template specialization for CBC mode.
//----------------------------------------------------------------------------

const ts::BlockCipherProperties& ts::CBC<ts::SM4>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(ts::SM4::Properties(), u"CBC", false, ts::SM4::BLOCK_SIZE, 0, ts::SM4::BLOCK_SIZE);
    return props;
}

ts::CBC<ts::SM4>::CBC() : SM4(ts::CBC<ts::SM4>::Properties())
{
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

ts::CBC<ts::SM4>::CBC(const BlockCipherProperties& props) : SM4(props)
{
    props.assertCompatibleChaining(ts::CBC<ts::SM4>::Properties());
    // OpenSSL and Windows BCrypt can encrypt/decrypt in place.
    canProcessInPlace(true);
}

#if !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::CBC<ts::SM4>::getAlgorithm() const
{
    // Thread-safe init-safe static data pattern:
    static const FetchCipherAlgorithm fetch("SM4-CBC");
    return fetch.algorithm();
}

#endif // TS_NO_OPENSSL

#endif // TS_NO_CRYPTO_LIBRARY
