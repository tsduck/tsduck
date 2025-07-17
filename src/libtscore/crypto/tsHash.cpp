//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHash.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::Hash::Hash(const UChar* name, size_t hash_size) :
    _name(name),
    _hash_size(hash_size)
{
}

ts::Hash::~Hash()
{
    // Cleanup system-specific crypto library resources, if used.
#if defined(TS_WINDOWS)

    if (_hash != nullptr) {
        ::BCryptDestroyHash(_hash);
        _hash = nullptr;
    }
    _algo = nullptr;

#elif !defined(TS_NO_OPENSSL)

    if (_context != nullptr) {
        EVP_MD_CTX_free(_context);
        _context = nullptr;
    }

#endif
}


//----------------------------------------------------------------------------
// Get parameters for system-specific crypto library.
// Default implementation, when thislibrary is not used.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

void ts::Hash::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    algo = nullptr;
    length = 0;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_MD_CTX* ts::Hash::referenceContext() const
{
    return nullptr;
}

#endif


//----------------------------------------------------------------------------
// Reinitialize the computation of the hash.
// Default implementation, using the system-specific crypto library.
//----------------------------------------------------------------------------

bool ts::Hash::init()
{
#if defined(TS_WINDOWS)

    // Get a reference to algorithm provider the first time.
    if (_algo == nullptr) {
        size_t objlength = 0;
        getAlgorithm(_algo, objlength);
        if (_algo == nullptr) {
            return false;
        }
        // Allocate the "hash object" for the rest of the life of this Hash instance.
        _obj.resize(objlength);
    }
    // Terminate previous hash if not yet done.
    if (_hash != nullptr) {
        ::BCryptDestroyHash(_hash);
        _hash = nullptr;
    }
    // Create a new hash handle.
    if (::BCryptCreateHash(_algo, &_hash, _obj.data(), ::ULONG(_obj.size()), nullptr, 0, 0) < 0) {
        return false;
    }
    return true;

#elif !defined(TS_NO_OPENSSL)

    // Create the hash context the first time.
    if (_context == nullptr && (_context = EVP_MD_CTX_new()) == nullptr) {
        OpenSSL::DebugErrors();
        return false;
    }
    // Copy the content of the reference context. This is much faster than fetching the algo again.
    if (!EVP_MD_CTX_copy_ex(_context, referenceContext())) {
        OpenSSL::DebugErrors();
        return false;
    }
    return true;

#else

    // No cryptographic library.
    return false;

#endif
}


//----------------------------------------------------------------------------
// Add some part of the message to hash. Can be called several times.
// Default implementation, using the system-specific crypto library.
//----------------------------------------------------------------------------

bool ts::Hash::add(const void* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return true;
    }

#if defined(TS_WINDOWS)

    return _hash != nullptr && ::BCryptHashData(_hash, ::PUCHAR(data), ::ULONG(size), 0) >= 0;

#elif !defined(TS_NO_OPENSSL)

    const bool ok = _context != nullptr && EVP_DigestUpdate(_context, data, size);
    OpenSSL::DebugErrors();
    return ok;

#else

    // No cryptographic library.
    return false;

#endif
}


//----------------------------------------------------------------------------
// Get the resulting hash value.
// Default implementation, using the system-specific crypto library.
//----------------------------------------------------------------------------

bool ts::Hash::getHash(void* hash, size_t bufsize, size_t* retsize)
{
    const size_t hsize = hashSize();
    if (retsize != nullptr) {
        *retsize = hsize;
    }
    if (hash == nullptr || bufsize < hsize) {
        return false;
    }

#if defined(TS_WINDOWS)

    // We need to provide the expected size for the hash.
    if (_hash == nullptr || ::BCryptFinishHash(_hash, ::PUCHAR(hash), ::ULONG(hsize), 0) < 0) {
        return false;
    }
    // We cannot reuse the hash handler, destroy it.
    ::BCryptDestroyHash(_hash);
    _hash = nullptr;
    return true;

#elif !defined(TS_NO_OPENSSL)

    const bool ok = _context != nullptr && EVP_DigestFinal_ex(_context, reinterpret_cast<unsigned char*>(hash), nullptr);
    OpenSSL::DebugErrors();
    return ok;

#else

    // No cryptographic library.
    return false;

#endif
}


//----------------------------------------------------------------------------
// Compute a hash in one operation
//----------------------------------------------------------------------------

bool ts::Hash::hash(const void* data, size_t data_size, void* hash, size_t hash_maxsize, size_t* hash_retsize)
{
    return init() && add(data, data_size) && getHash(hash, hash_maxsize, hash_retsize);
}

bool ts::Hash::hash(const void* data, size_t data_size, ByteBlock& result)
{
    result.resize(hashSize());
    size_t retsize = 0;
    const bool ok = hash(data, data_size, &result[0], result.size(), &retsize);
    result.resize(ok ? retsize : 0);
    return ok;
}
