//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// Microsoft Windows BCrypt library support.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

// A class to open a BCrypt algorithm only once.
ts::FetchBCryptAlgorithm::FetchBCryptAlgorithm(::LPCWSTR algo_id, ::LPCWSTR chain_mode)
{
    if (::BCryptOpenAlgorithmProvider(&_algo, algo_id, nullptr, 0) >= 0) {
        bool success = chain_mode == nullptr || ::BCryptSetProperty(_algo, BCRYPT_CHAINING_MODE, ::PUCHAR(chain_mode), sizeof(chain_mode), 0) >= 0;
        ::DWORD length = 0;
        ::ULONG retsize = 0;
        if (success) {
            success = ::BCryptGetProperty(_algo, BCRYPT_OBJECT_LENGTH, ::PUCHAR(&length), sizeof(length), &retsize, 0) >= 0 && retsize == sizeof(length);
        }
        if (success) {
            _objlength = size_t(length);
        }
        else {
            ::BCryptCloseAlgorithmProvider(_algo, 0);
            _algo = nullptr;
        }
    }
}

// Cleanup BCrypt algorithm.
ts::FetchBCryptAlgorithm::~FetchBCryptAlgorithm()
{
    if (_algo != nullptr) {
        ::BCryptCloseAlgorithmProvider(_algo, 0);
        _algo = nullptr;
    }
}

#else

//----------------------------------------------------------------------------
// OpenSSL crypto library support (Unix systems only).
//----------------------------------------------------------------------------

// A singleton which initialize the cryptographic library.
TS_DEFINE_SINGLETON(ts::InitCryptoLibrary);

// Initialize OpenSSL.
ts::InitCryptoLibrary::InitCryptoLibrary()
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

// Cleanup OpenSSL.
ts::InitCryptoLibrary::~InitCryptoLibrary()
{
    EVP_cleanup();
    ERR_free_strings();
}

// A class to create a singleton with a preset hash context for OpenSSL.
ts::PresetHashContext::PresetHashContext(const char* algo)
{
    _md = EVP_MD_fetch(nullptr, algo, nullptr);
    if (_md != nullptr) {
        _context = EVP_MD_CTX_new();
        if (_context != nullptr && !EVP_DigestInit_ex(_context, _md, nullptr)) {
            EVP_MD_CTX_free(_context);
            _context = nullptr;
        }
    }
}

// Cleanup hash context for OpenSSL.
ts::PresetHashContext::~PresetHashContext()
{
    if (_context != nullptr) {
        EVP_MD_CTX_free(_context);
        _context = nullptr;
    }
    if (_md != nullptr) {
        EVP_MD_free(_md);
        _md = nullptr;
    }
}

#endif
