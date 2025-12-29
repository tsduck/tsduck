//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for hash functions
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsCryptoLibrary.h"

namespace ts {
    //!
    //! Base class for all hash functions.
    //! @ingroup libtscore crypto
    //!
    class TSCOREDLL Hash
    {
        TS_NOBUILD_NOCOPY(Hash);
    public:
        //!
        //! Algorithm name (informational only).
        //! @return The algorithm name.
        //!
        UString name() const { return _name; }

        //!
        //! Size in bytes of the resulting hash.
        //! @return The size in bytes of the resulting hash.
        //!
        size_t hashSize() const { return _hash_size; }

        //!
        //! Reinitialize the computation of the hash.
        //! @return True on success, false on error.
        //!
        virtual bool init();

        //!
        //! Add some part of the message to hash.
        //! Can be called several times.
        //! @param [in] data Address of message part.
        //! @param [in] size Size in bytes of message part.
        //! @return True on success, false on error.
        //!
        virtual bool add(const void* data, size_t size);

        //!
        //! Get the resulting hash value.
        //! @param [out] hash Address of returned hash buffer.
        //! @param [in] bufsize Size in bytes of hash buffer.
        //! @param [out] retsize Address of an integer receiving
        //! the actual returned hash size. Can be a null pointer (ignored).
        //! @return True on success, false on error.
        //!
        virtual bool getHash(void* hash, size_t bufsize, size_t* retsize = nullptr);

        //!
        //! Compute a hash in one operation.
        //! Same in init() + add() + getHash().
        //! @param [in] data Address of message to hash.
        //! @param [in] data_size Size in bytes of message to hash.
        //! @param [out] hash Address of returned hash buffer.
        //! @param [in] hash_maxsize Size in bytes of hash buffer.
        //! @param [out] hash_retsize Address of an integer receiving
        //! the actual returned hash size. Can be a null pointer (ignored).
        //! @return True on success, false on error.
        //!
        bool hash(const void* data, size_t data_size, void* hash, size_t hash_maxsize, size_t* hash_retsize = nullptr);

        //!
        //! Compute a hash in one operation.
        //! Same in init() + add() + getHash().
        //! @param [in] data Address of message to hash.
        //! @param [in] data_size Size in bytes of message to hash.
        //! @param [out] hash Returned hash buffer.
        //! @return True on success, false on error.
        //!
        bool hash(const void* data, size_t data_size, ByteBlock& hash);

        //!
        //! Virtual destructor.
        //!
        virtual ~Hash();

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] name Algorithm name.
        //! @param [in] hash_size Size in bytes of the hash value.
        //!
        Hash(const UChar* name, size_t hash_size);

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! Get the algorithm handle and subobject size, when the subclass uses Microsoft BCrypt library.
        //! @param [out] algo Handle to hash algorithm.
        //! @param [out] length Length in bytes of the subobject to allocate.
        //!
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const;
#endif

#if (!defined(TS_WINDOWS) && !defined(TS_NO_OPENSSL)) || defined(DOXYGEN)
        //!
        //! Get reference hash context, when the subclass uses OpenSSL.
        //! @return Reference EVP hash context. To be copied into each new hash object.
        //!
        virtual const EVP_MD_CTX* referenceContext() const;
#endif

    private:
        const UChar* const _name;
        const size_t _hash_size;
#if defined(TS_WINDOWS)
        ::BCRYPT_ALG_HANDLE _algo = nullptr;
        ::BCRYPT_HASH_HANDLE _hash = nullptr;
        ByteBlock _obj {};
#elif !defined(TS_NO_OPENSSL)
        EVP_MD_CTX* _context = nullptr;
#endif
    };
}
