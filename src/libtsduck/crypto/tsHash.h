//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

namespace ts {
    //!
    //! Abstract interface of hash functions.
    //! @ingroup crypto
    //!
    class TSDUCKDLL Hash
    {
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
        //! Size in bytes of the block used by the algorithm.
        //! @return The size in bytes of the block used by the algorithm.
        //! Informational only. Can be zero if non significant.
        //!
        virtual size_t blockSize() const = 0;

        //!
        //! Reinitialize the computation of the hash.
        //! @return True on success, false on error.
        //!
        virtual bool init() = 0;

        //!
        //! Add some part of the message to hash.
        //! Can be called several times.
        //! @param [in] data Address of message part.
        //! @param [in] size Size in bytes of message part.
        //! @return True on success, false on error.
        //!
        virtual bool add(const void* data, size_t size) = 0;

        //!
        //! Get the resulting hash value.
        //! @param [out] hash Address of returned hash buffer.
        //! @param [in] bufsize Size in bytes of hash buffer.
        //! @param [out] retsize Address of an integer receiving
        //! the actual returned hash size. Can be a null pointer (ignored).
        //! @return True on success, false on error.
        //!
        virtual bool getHash(void* hash, size_t bufsize, size_t* retsize = nullptr) = 0;

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
    };
}
