//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFetchAlgorithm.h"


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

#elif !defined(TS_NO_OPENSSL)

//----------------------------------------------------------------------------
// A class to create a singleton with a preset hash context for OpenSSL.
//----------------------------------------------------------------------------

ts::FetchHashAlgorithm::FetchHashAlgorithm(const char* algo, const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
    OpenSSL::Providers::Instance().load(provider);
    _algo = EVP_MD_fetch(nullptr, algo, OpenSSL::Providers::Properties(provider).c_str());
#else
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    _algo = EVP_get_digestbyname(algo);
#endif

    if (_algo != nullptr) {
        _context = EVP_MD_CTX_new();
        if (_context != nullptr && !EVP_DigestInit_ex(_context, _algo, nullptr)) {
            EVP_MD_CTX_free(_context);
            _context = nullptr;
        }
    }
    OpenSSL::DebugErrors();
}

ts::FetchHashAlgorithm::~FetchHashAlgorithm()
{
    FetchHashAlgorithm::terminate();
}

void ts::FetchHashAlgorithm::terminate()
{
    if (_context != nullptr) {
        EVP_MD_CTX_free(_context);
        _context = nullptr;
    }

#if defined(TS_OPENSSL_PROVIDERS)
    if (_algo != nullptr) {
        EVP_MD_free(const_cast<EVP_MD*>(_algo));
        _algo = nullptr;
    }
#endif
}


//----------------------------------------------------------------------------
// A class to create a singleton with a preset cipher algorithm for OpenSSL.
//----------------------------------------------------------------------------

ts::FetchCipherAlgorithm::FetchCipherAlgorithm(const char* algo, const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
    OpenSSL::Providers::Instance().load(provider);
    _algo = EVP_CIPHER_fetch(nullptr, algo, OpenSSL::Providers::Properties(provider).c_str());
#else
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    _algo = EVP_get_cipherbyname(algo);
#endif
    OpenSSL::DebugErrors();
}

ts::FetchCipherAlgorithm::~FetchCipherAlgorithm()
{
    FetchCipherAlgorithm::terminate();
}

void ts::FetchCipherAlgorithm::terminate()
{
#if defined(TS_OPENSSL_PROVIDERS)
    if (_algo != nullptr) {
        EVP_CIPHER_free(const_cast<EVP_CIPHER*>(_algo));
        _algo = nullptr;
    }
#endif
}

#else
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsFetchAlgorithmIsEmpty = true; // Avoid warning about empty module.
#endif
