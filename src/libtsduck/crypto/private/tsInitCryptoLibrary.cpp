//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInitCryptoLibrary.h"
#include "tsEnvironment.h"


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
    _debug = !GetEnvironment(u"TS_DEBUG_OPENSSL").empty();
}

// Cleanup OpenSSL.
ts::InitCryptoLibrary::~InitCryptoLibrary()
{
    EVP_cleanup();
    ERR_free_strings();
}

// Load an OpenSSL provider if not yet loaded.
void ts::FetchBase::loadProvider(const char* provider)
{
#if OPENSSL_VERSION_MAJOR >= 3
    if (provider != nullptr &&
        provider[0] != '\0' &&
        !OSSL_PROVIDER_available(nullptr, provider) &&
        OSSL_PROVIDER_load(nullptr, provider) == nullptr)
    {
        PrintCryptographicLibraryErrors();
    }
#endif
}

// Get the properies string from an OpenSSL provider.
std::string ts::FetchBase::providerProperties(const char* provider)
{
    return provider == nullptr || provider[0] == '\0' ? std::string() : std::string("provider=") + provider;
}

// A class to create a singleton with a preset hash context for OpenSSL.
ts::FetchHashAlgorithm::FetchHashAlgorithm(const char* algo, const char* provider)
{
#if OPENSSL_VERSION_MAJOR >= 3
    loadProvider(provider);
    _algo = EVP_MD_fetch(nullptr, algo, providerProperties(provider).c_str());
#else
    _algo = EVP_get_digestbyname(algo);
#endif
    if (_algo != nullptr) {
        _context = EVP_MD_CTX_new();
        if (_context != nullptr && !EVP_DigestInit_ex(_context, _algo, nullptr)) {
            EVP_MD_CTX_free(_context);
            _context = nullptr;
        }
    }
    PrintCryptographicLibraryErrors();
}

// Cleanup hash context for OpenSSL.
ts::FetchHashAlgorithm::~FetchHashAlgorithm()
{
    if (_context != nullptr) {
        EVP_MD_CTX_free(_context);
        _context = nullptr;
    }
#if OPENSSL_VERSION_MAJOR >= 3
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    if (_algo != nullptr) {
        EVP_MD_free(const_cast<EVP_MD*>(_algo));
        _algo = nullptr;
    }
#endif
}

// A class to create a singleton with a preset cipher algorithm for OpenSSL.
ts::FetchCipherAlgorithm::FetchCipherAlgorithm(const char* algo, const char* provider)
{
#if OPENSSL_VERSION_MAJOR >= 3
    loadProvider(provider);
    _algo = EVP_CIPHER_fetch(nullptr, algo, providerProperties(provider).c_str());
#else
    _algo = EVP_get_cipherbyname(algo);
#endif
    PrintCryptographicLibraryErrors();
}

// Cleanup cipher algorithm for OpenSSL.
ts::FetchCipherAlgorithm::~FetchCipherAlgorithm()
{
#if OPENSSL_VERSION_MAJOR >= 3
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    if (_algo != nullptr) {
        EVP_CIPHER_free(const_cast<EVP_CIPHER*>(_algo));
        _algo = nullptr;
    }
#endif
}

#endif
