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
    //! A class to create a singleton with a preset hash context for OpenSSL.
    //! This method speeds up the creation of hash context (the standard EVP scenario is too slow).
    //!
    class FetchHashAlgorithm : public OpenSSL::Controlled
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
    class FetchCipherAlgorithm : public OpenSSL::Controlled
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
}
