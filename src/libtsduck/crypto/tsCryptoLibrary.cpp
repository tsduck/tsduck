//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCryptoLibrary.h"
#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Get the name and version of the underlying cryptographic library.
//----------------------------------------------------------------------------

ts::UString ts::GetCryptographicLibraryVersion()
{
    InitCryptographicLibrary();

#if defined(TS_WINDOWS)
    // Don't know how to get the version of BCrypt library.
    return u"Microsoft BCrypt";
#elif defined(OPENSSL_FULL_VERSION_STRING)
    // OpenSSL v3
    return UString::Format(u"OpenSSL %s (%s)", {OpenSSL_version(OPENSSL_FULL_VERSION_STRING), OpenSSL_version(OPENSSL_CPU_INFO)});
#else
    // OpenSSL v1
    return UString::FromUTF8(OpenSSL_version(OPENSSL_VERSION));
#endif
}
