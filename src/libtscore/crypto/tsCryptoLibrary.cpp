//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCryptoLibrary.h"
#include "tsInitCryptoLibrary.h"
#include "tsVersionInfo.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS) && defined(TS_NO_OPENSSL)
#define SUPPORT UNSUPPORTED
#else
#define SUPPORT SUPPORTED
#endif

TS_REGISTER_FEATURE(u"crypto", u"Cryptographic library", SUPPORT, ts::GetCryptographicLibraryVersion);


//----------------------------------------------------------------------------
// Get the name and version of the underlying cryptographic library.
//----------------------------------------------------------------------------

ts::UString ts::GetCryptographicLibraryVersion()
{
    InitCryptographicLibrary();

#if defined(TS_NO_CRYPTO_LIBRARY)
    return u"This version of TSDuck was compiled without cryptographic library support";
#elif defined(TS_WINDOWS)
    // Don't know how to get the version of BCrypt library.
    return u"Microsoft BCrypt";
#elif defined(OPENSSL_FULL_VERSION_STRING)
    // OpenSSL v3
    return UString::Format(u"OpenSSL %s (%s)", OpenSSL_version(OPENSSL_FULL_VERSION_STRING), OpenSSL_version(OPENSSL_CPU_INFO));
#else
    // OpenSSL v1
    return UString::FromUTF8(OpenSSL_version(OPENSSL_VERSION));
#endif
}
