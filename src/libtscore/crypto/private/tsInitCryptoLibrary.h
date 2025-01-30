//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Initialization of the system-specific cryptographic library.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCryptoLibrary.h"

// Private header, not accessible to applications.
//! @cond nodoxygen

namespace ts {

#if defined(TS_WINDOWS)

    //!
    //! Windows: A class to open a BCrypt algorithm only once.
    //! Store algorithm handle and subobject length in bytes.
    //!
    class FetchBCryptAlgorithm
    {
        TS_NOBUILD_NOCOPY(FetchBCryptAlgorithm);
    public:
        FetchBCryptAlgorithm(::LPCWSTR algo_id, ::LPCWSTR chain_mode = nullptr);
        ~FetchBCryptAlgorithm();
        void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const { algo = _algo; length = _objlength; }
    private:
        ::BCRYPT_ALG_HANDLE _algo = nullptr;
        size_t _objlength = 0;
    };

#elif !defined(TS_NO_OPENSSL)

    //!
    //! Base class for objects which must be terminated with OpenSSL.
    //!
    class TerminateWithOpenSSL
    {
        TS_NOCOPY(TerminateWithOpenSSL);
    public:
        TerminateWithOpenSSL();
        virtual ~TerminateWithOpenSSL();

        // This method terminates anything about OpenSSL which must be cleaned up.
        // This method must be idempotent and must be called in the subclass destructor.
        virtual void terminate() = 0;

    private:
        // This internal class is a repository of all active instances of TerminateWithOpenSSL.
        // The constructor and destructor of TerminateWithOpenSSL perform the registration / deregistration.
        class Repo
        {
            TS_SINGLETON(Repo);
        public:
            // Register / deregister an instance.
            void registerObject(TerminateWithOpenSSL*);
            void deregisterObject(TerminateWithOpenSSL*);

        private:
            // List of registered instances, under protection of a mutex.
            std::mutex _mutex {};
            std::list<TerminateWithOpenSSL*> _list {};

            // This method calls the terminate() method of all active instances of TerminateWithOpenSSL in
            // reverse order of registration and deregisters them.
            void terminate();

            // This static method is executed by OpenSSL termination procedure.
            // It calls the instance's terminate().
            static void exitHandler();

            // Warning: the OpenSSL termination procedure can be called after the Repo destructor.
            // This static boolean is set to true as long as the Repo singleton is alive.
            static bool active;
            ~Repo();
        };
    };

    //!
    //! A singleton which initializes the OpenSSL cryptographic library.
    //!
    class InitCryptoLibrary : public TerminateWithOpenSSL
    {
        TS_SINGLETON(InitCryptoLibrary);
    public:
        // Check if environment variable TS_DEBUG_OPENSSL was defined.
        bool debug() const { return _debug; }

        // Implementation of TerminateWithOpenSSL.
        virtual void terminate() override;
        virtual ~InitCryptoLibrary() override;

        // Load an OpenSSL provider if not yet loaded.
        void loadProvider(const char* provider);

        // Get the properies string from an OpenSSL provider.
        static std::string providerProperties(const char* provider);

    private:
        bool _debug = false;
    #if defined(TS_OPENSSL_PROVIDERS)
        std::mutex _mutex {};
        std::map<std::string,OSSL_PROVIDER*> _providers {};
    #endif
    };

    //!
    //! A class to create a singleton with a preset hash context for OpenSSL.
    //! This method speeds up the creation of hash context (the standard EVP scenario is too slow).
    //!
    class FetchHashAlgorithm : public TerminateWithOpenSSL
    {
        TS_NOBUILD_NOCOPY(FetchHashAlgorithm);
    public:
        FetchHashAlgorithm(const char* algo, const char* provider = nullptr);
        virtual ~FetchHashAlgorithm() override;
        virtual void terminate() override;
        const EVP_MD_CTX* referenceContext() const { return _context; }
    private:
        // With OpenSSL 3, this should not be const because of EVP_MD_free.
        const EVP_MD* _algo = nullptr;
        EVP_MD_CTX* _context = nullptr;
    };

    //!
    //! A class to create a singleton with a preset cipher algorithm for OpenSSL.
    //!
    class FetchCipherAlgorithm : public TerminateWithOpenSSL
    {
        TS_NOBUILD_NOCOPY(FetchCipherAlgorithm);
    public:
        FetchCipherAlgorithm(const char* algo, const char* provider = nullptr);
        virtual ~FetchCipherAlgorithm() override;
        virtual void terminate() override;
        const EVP_CIPHER* algorithm() const { return _algo; }
    private:
        // With OpenSSL 3, this should not be const because of EVP_CIPHER_free.
        const EVP_CIPHER* _algo = nullptr;
    };

#endif

    //!
    //! Internal function to initialize the underlying cryptographic library.
    //! Can be called many times, executed only once.
    //!
    inline void InitCryptographicLibrary()
    {
    #if !defined(TS_WINDOWS) && !defined(TS_NO_OPENSSL)
        InitCryptoLibrary::Instance();
    #endif
    }

    //!
    //! Internal function to display errors from the underlying cryptographic library on standard error.
    //!
    inline void PrintCryptographicLibraryErrors()
    {
    #if !defined(TS_WINDOWS) && !defined(TS_NO_OPENSSL)
        if (InitCryptoLibrary::Instance().debug()) {
            ERR_print_errors_fp(stderr);
        }
    #endif
    }
}

//! @endcond
