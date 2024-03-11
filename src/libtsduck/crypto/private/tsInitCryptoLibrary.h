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
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSingleton.h"
#include "tsCryptoLibrary.h"

// Private header, not accessible to applications.
//! @cond nodoxygen

namespace ts {

#if defined(TS_WINDOWS)

    // A class to open a BCrypt algorithm only once.
    // Store algorithm handle and subobject length in bytes.
    class FetchBCryptAlgorithm
    {
        TS_NOBUILD_NOCOPY(FetchBCryptAlgorithm);
    public:
        FetchBCryptAlgorithm(::LPCWSTR algo_id, ::LPCWSTR chain_mode = nullptr);
        ~FetchBCryptAlgorithm();
        void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) { algo = _algo; length = _objlength; }
    private:
        ::BCRYPT_ALG_HANDLE _algo = nullptr;
        size_t _objlength = 0;
    };

#else

    // A singleton which initializes the cryptographic library.
    class InitCryptoLibrary
    {
        TS_DECLARE_SINGLETON(InitCryptoLibrary);
    public:
        ~InitCryptoLibrary();
        bool debug() const { return _debug; }
    private:
        bool _debug = false;
    };

    // Common base for Fetch___ classes.
    class FetchBase
    {
    protected:
        static void loadProvider(const char* provider);
        static std::string providerProperties(const char* provider);
    };

    // A class to create a singleton with a preset hash context for OpenSSL.
    // This method speeds up the creation of hash context (the standard EVP scenario is too slow).
    class FetchHashAlgorithm : private FetchBase
    {
        TS_NOBUILD_NOCOPY(FetchHashAlgorithm);
    public:
        FetchHashAlgorithm(const char* algo, const char* provider = nullptr);
        ~FetchHashAlgorithm();
        const EVP_MD_CTX* referenceContext() const { return _context; }
    private:
        EVP_MD* _algo = nullptr;
        EVP_MD_CTX* _context = nullptr;
    };

    // A class to create a singleton with a preset cipher algorithm for OpenSSL.
    class FetchCipherAlgorithm : private FetchBase
    {
        TS_NOBUILD_NOCOPY(FetchCipherAlgorithm);
    public:
        FetchCipherAlgorithm(const char* algo, const char* provider = nullptr);
        ~FetchCipherAlgorithm();
        const EVP_CIPHER* algorithm() const { return _algo; }
    private:
        EVP_CIPHER* _algo = nullptr;
    };

#endif

    // Internal function to initialize the underlying cryptographic library.
    // Can be called many times, executed only once.
    inline void InitCryptographicLibrary()
    {
#if !defined(TS_WINDOWS)
        InitCryptoLibrary::Instance();
#endif
    }

    // Internal function to display errors from the underlying cryptographic library on standard error.
    inline void PrintCryptographicLibraryErrors()
    {
#if !defined(TS_WINDOWS)
        if (InitCryptoLibrary::Instance().debug()) {
            ERR_print_errors_fp(stderr);
        }
#endif
    }
}

//! @endcond
