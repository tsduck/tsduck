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

    // Starting with OpenSSL 3.0, algorithms are stored in providers.
    // Providers must be separately managed (see comments in .cpp file).
    #if OPENSSL_VERSION_MAJOR >= 3
        #define TS_OPENSSL_PROVIDERS 1
    #endif

    // A singleton which initializes the cryptographic library.
    class InitCryptoLibrary
    {
        TS_DECLARE_SINGLETON(InitCryptoLibrary);
    public:
        // Check if environment variable TS_DEBUG_OPENSSL was defined.
        bool debug() const { return _debug; }

    private:
        bool _debug = false;

    #if defined(TS_OPENSSL_PROVIDERS)

    public:
        // Load an OpenSSL provider if not yet loaded.
        void loadProvider(const char* provider);

        // Unload all providers.
        ~InitCryptoLibrary();

        // Get the properies string from an OpenSSL provider.
        static std::string providerProperties(const char* provider);

    private:
        std::mutex _mutex {};
        std::map<std::string,OSSL_PROVIDER*> _providers {};

    #endif // TS_OPENSSL_PROVIDERS
    };

    // A class to create a singleton with a preset hash context for OpenSSL.
    // This method speeds up the creation of hash context (the standard EVP scenario is too slow).
    class FetchHashAlgorithm
    {
        TS_NOBUILD_NOCOPY(FetchHashAlgorithm);
    public:
        FetchHashAlgorithm(const char* algo, const char* provider = nullptr);
        ~FetchHashAlgorithm();
        const EVP_MD_CTX* referenceContext() const { return _context; }
    private:
        // With OpenSSL 3, this should not be const because of EVP_MD_free.
        const EVP_MD* _algo = nullptr;
        EVP_MD_CTX* _context = nullptr;
    };

    // A class to create a singleton with a preset cipher algorithm for OpenSSL.
    class FetchCipherAlgorithm
    {
        TS_NOBUILD_NOCOPY(FetchCipherAlgorithm);
    public:
        FetchCipherAlgorithm(const char* algo, const char* provider = nullptr);
        ~FetchCipherAlgorithm();
        const EVP_CIPHER* algorithm() const { return _algo; }
    private:
        // With OpenSSL 3, this should not be const because of EVP_CIPHER_free.
        const EVP_CIPHER* _algo = nullptr;
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
