//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
    //! Abstract interface of hash functions.
    //! @ingroup crypto
    //!
    class TSDUCKDLL Hash
    {
        TS_NOCOPY(Hash);
    public:
        //!
        //! Algorithm name (informational only).
        //! @return The algorithm name.
        //!
        virtual UString name() const = 0;

        //!
        //! Size in bytes of the resulting hash.
        //! @return The size in bytes of the resulting hash.
        //!
        virtual size_t hashSize() const = 0;

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
        //! Default constructor
        //!
        Hash() = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~Hash();

    protected:
        //! @cond nodoxygen
#if defined(TS_WINDOWS) && !defined(DOXYGEN)
        // Get the algorithm id.
        virtual ::LPCWSTR algorithmId() const = 0;
#else
        // Get reference hash context.
        virtual const EVP_MD_CTX* referenceContext() const = 0;
#endif
        //! @endcond

    private:
#if defined(TS_WINDOWS)
        ::BCRYPT_ALG_HANDLE _algo = nullptr;
        ::BCRYPT_HASH_HANDLE _hash = nullptr;
        ByteBlock _obj {};
#else
        EVP_MD_CTX* _context = nullptr;
#endif
    };
}
