//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsInitCryptoLibrary.h"


//----------------------------------------------------------------------------
// A singleton which initialize the cryptographic library.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS)

TS_DEFINE_SINGLETON(ts::InitCryptoLibrary);

ts::InitCryptoLibrary::InitCryptoLibrary()
{
    // Initialize OpenSSL.
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

ts::InitCryptoLibrary::~InitCryptoLibrary()
{
    // Cleanup OpenSSL.
    EVP_cleanup();
    ERR_free_strings();
}

#endif


//----------------------------------------------------------------------------
// A class to create a singleton with a preset hash context for OpenSSL.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS)

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
