//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Initialization of the system-specific cryptographic library.
//!  Private header, not accessible to applications.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"
#include "tsCryptoLibrary.h"

namespace ts {

#if !defined(TS_WINDOWS) && !defined(DOXYGEN)
    // A singleton which initialize the cryptographic library.
    class InitCryptoLibrary
    {
        TS_DECLARE_SINGLETON(InitCryptoLibrary);
    public:
        ~InitCryptoLibrary();
    };
#endif

    //!
    //! Internal function to initialize the underlying cryptographic library.
    //! Call be called many times, executed only once.
    //! @ingroup crypto
    //!
    inline void InitCryptographicLibrary()
    {
#if !defined(TS_WINDOWS)
        InitCryptoLibrary::Instance();
#endif
    }

#if !defined(TS_WINDOWS) && !defined(DOXYGEN)
    // A class to create a singleton with a preset hash context for OpenSSL.
    // This method speeds up the creation of hash context (the standard EVP scenario is too slow).
    class PresetHashContext
    {
        TS_NOBUILD_NOCOPY(PresetHashContext);
    public:
        PresetHashContext(const char* algo);
        ~PresetHashContext();
        const EVP_MD_CTX* context() const { return _context; }
    private:
        EVP_MD* _md = nullptr;
        EVP_MD_CTX* _context = nullptr;
    };
#endif
}
