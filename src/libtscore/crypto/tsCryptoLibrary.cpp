//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCryptoLibrary.h"
#include "tsFeatures.h"
#if defined(TS_WINDOWS)
    #include "tsWinModuleInfo.h"
#endif


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if defined(TS_NO_CRYPTO_LIBRARY)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"crypto", u"Cryptographic library", SUPPORT, ts::GetCryptographicLibraryVersion);


//----------------------------------------------------------------------------
// Get the name and version of the underlying cryptographic library.
//----------------------------------------------------------------------------

ts::UString ts::GetCryptographicLibraryVersion()
{
#if defined(TS_WINDOWS)
    return WinModuleInfo(u"bcrypt.dll").summary();
#else
    return OpenSSL::Version();
#endif
}
