//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInitCryptoLibrary.h"
#include "tsEnvironment.h"
#include "tsAlgorithm.h"


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
// The singleton needs to be destroyed no later that OpenSSL cleanup.
// OpenSSL is supposed to cleanup all its resources on exit, using atexit() handlers.
// However, providers are not unloaded in this context. This is probably a bug in
// OpenSSL because the result is inconsistent: Providers are still loaded and use
// resources but they are unusable because OpenSSL is closed. The only safe way to
// deallocate providers is to call OSSL_PROVIDER_unload() from a OPENSSL_atexit()
// handler. This is explained here: https://github.com/lelegard/openssl-prov-leak
TS_DEFINE_SINGLETON_ATEXIT(ts::InitCryptoLibrary, OPENSSL_atexit);

// Providers management in OpenSSL 3.0 onwards..
#if defined(TS_OPENSSL_PROVIDERS)

// Load an OpenSSL provider if not yet loaded.
void ts::InitCryptoLibrary::loadProvider(const char* provider)
{
    const std::string name(provider != nullptr ? provider : "");
    if (!name.empty()) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_providers.contains(name)) {
            OSSL_PROVIDER* prov = OSSL_PROVIDER_load(nullptr, provider);
            if (prov != nullptr) {
                _providers[name] = prov;
            }
            else {
                PrintCryptographicLibraryErrors();
            }
        }
    }
}

// Unload all providers.
ts::InitCryptoLibrary::~InitCryptoLibrary()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& prov : _providers) {
        OSSL_PROVIDER_unload(prov.second);
    }
    _providers.clear();
}

// Get the properies string from an OpenSSL provider.
std::string ts::InitCryptoLibrary::providerProperties(const char* provider)
{
    return provider == nullptr || provider[0] == '\0' ? std::string() : std::string("provider=") + provider;
}

#endif // TS_OPENSSL_PROVIDERS

// Initialize OpenSSL.
ts::InitCryptoLibrary::InitCryptoLibrary()
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    _debug = !GetEnvironment(u"TS_DEBUG_OPENSSL").empty();
}

// A class to create a singleton with a preset hash context for OpenSSL.
ts::FetchHashAlgorithm::FetchHashAlgorithm(const char* algo, const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
    InitCryptoLibrary::Instance().loadProvider(provider);
    _algo = EVP_MD_fetch(nullptr, algo, InitCryptoLibrary::providerProperties(provider).c_str());
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
    PrintCryptographicLibraryErrors();
}

// Cleanup hash context for OpenSSL.
ts::FetchHashAlgorithm::~FetchHashAlgorithm()
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

// A class to create a singleton with a preset cipher algorithm for OpenSSL.
ts::FetchCipherAlgorithm::FetchCipherAlgorithm(const char* algo, const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
    InitCryptoLibrary::Instance().loadProvider(provider);
    _algo = EVP_CIPHER_fetch(nullptr, algo, InitCryptoLibrary::providerProperties(provider).c_str());
#else
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    _algo = EVP_get_cipherbyname(algo);
#endif
    PrintCryptographicLibraryErrors();
}

// Cleanup cipher algorithm for OpenSSL.
ts::FetchCipherAlgorithm::~FetchCipherAlgorithm()
{
#if defined(TS_OPENSSL_PROVIDERS)
    if (_algo != nullptr) {
        EVP_CIPHER_free(const_cast<EVP_CIPHER*>(_algo));
        _algo = nullptr;
    }
#endif
}

#endif
