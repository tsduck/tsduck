//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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

#elif !defined(TS_NO_OPENSSL)

//----------------------------------------------------------------------------
// Base class for objects which must be terminated with OpenSSL.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::TerminateWithOpenSSL::Repo);

// The constructor registers the object into the repository of object to terminate.
ts::TerminateWithOpenSSL::TerminateWithOpenSSL()
{
    Repo::Instance().registerObject(this);
}

// The destructor deregisters the object to avoid being called later.
ts::TerminateWithOpenSSL::~TerminateWithOpenSSL()
{
    Repo::Instance().deregisterObject(this);
}

// Warning: the OpenSSL termination procedure can be called after the Repo destructor.
// This static boolean is set to true as long as the Repo singleton is alive.
bool ts::TerminateWithOpenSSL::Repo::active = false;

// The private constructor of the Repo singleton registers exitHandler() to OpenSSL_atexit().
ts::TerminateWithOpenSSL::Repo::Repo()
{
    active = true;
    OPENSSL_atexit(exitHandler);
}

// Destructor may be called before exitHandler().
// Make sure exitHandler() will not call a destructed object.
ts::TerminateWithOpenSSL::Repo::~Repo()
{
    terminate();
    active = false;
}

// Register an instance.
void ts::TerminateWithOpenSSL::Repo::registerObject(TerminateWithOpenSSL* obj)
{
    if (obj != nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        _list.push_back(obj);
    }
}

// Deregister an instance.
void ts::TerminateWithOpenSSL::Repo::deregisterObject(TerminateWithOpenSSL* obj)
{
    if (obj != nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it = _list.begin(); it != _list.end(); ) {
            if (*it == obj) {
                it = _list.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

// This method calls the terminate() method of all active instances of TerminateWithOpenSSL in
// reverse order of registration and deregisters them.
void ts::TerminateWithOpenSSL::Repo::terminate()
{
    for (;;) {
        TerminateWithOpenSSL* obj = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_list.empty()) {
                return;
            }
            obj = _list.back();
            _list.pop_back();
        }
        if (obj != nullptr) {
            obj->terminate();
        }
    }
}

// This static method is executed by OpenSSL termination procedure.
// Warning: may be called after Repo's destructor.
void ts::TerminateWithOpenSSL::Repo::exitHandler()
{
    if (active) {
        Instance().terminate();
    }
}


//----------------------------------------------------------------------------
// A singleton which initialize the OpenSSL cryptographic library.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::InitCryptoLibrary);

// Initialize OpenSSL.
ts::InitCryptoLibrary::InitCryptoLibrary()
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    _debug = !GetEnvironment(u"TS_DEBUG_OPENSSL").empty();
}

// Destructor is the same as terminate().
ts::InitCryptoLibrary::~InitCryptoLibrary()
{
    InitCryptoLibrary::terminate();
}

// Unload all providers. Must be idempotent.
void ts::InitCryptoLibrary::terminate()
{
#if defined(TS_OPENSSL_PROVIDERS)
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& prov : _providers) {
        OSSL_PROVIDER_unload(prov.second);
    }
    _providers.clear();
#endif
}

// Load an OpenSSL provider if not yet loaded.
void ts::InitCryptoLibrary::loadProvider(const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
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
#endif
}

// Get the properies string from an OpenSSL provider.
std::string ts::InitCryptoLibrary::providerProperties(const char* provider)
{
    return provider == nullptr || provider[0] == '\0' ? std::string() : std::string("provider=") + provider;
}


//----------------------------------------------------------------------------
// A class to create a singleton with a preset hash context for OpenSSL.
//----------------------------------------------------------------------------

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
    InitCryptoLibrary::Instance().loadProvider(provider);
    _algo = EVP_CIPHER_fetch(nullptr, algo, InitCryptoLibrary::providerProperties(provider).c_str());
#else
    // With OpenSSL v1, this is a predefined context which shall not be freeed.
    _algo = EVP_get_cipherbyname(algo);
#endif
    PrintCryptographicLibraryErrors();
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
bool tsInitCryptoLibraryIsEmpty = true; // Avoid warning about empty module.
#endif
