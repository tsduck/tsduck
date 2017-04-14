//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Abstract base class for hash functions
//
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class TSDUCKDLL Hash
    {
    public:
        // Algorithm name.
        virtual std::string name() const = 0;

        // Size in bytes of the resulting hash.
        virtual size_t hashSize() const = 0;

        // Size in bytes of the block used by the algorithm.
        // Informational only. Can be zero if non significant.
        virtual size_t blockSize() const = 0;

        // Reinitialize the computation of the hash.
        // Return true on success, false on error.
        virtual bool init() = 0;

        // Add some part of the message to hash. Can be called several times.
        // Return true on success, false on error.
        virtual bool add (const void* data, size_t size) = 0;

        // Get the resulting hash value.
        // If retsize is non-zero, return the actual hash size.
        // Return true on success, false on error.
        virtual bool getHash (void* hash, size_t bufsize, size_t* retsize = 0) = 0;

        // Compute a hash in one operation (init + add + getHash)
        bool hash (const void* data, size_t data_size,
                   void* hash, size_t hash_maxsize, size_t* hash_retsize = 0)
        {
            return init() && add (data, data_size) && getHash (hash, hash_maxsize, hash_retsize);
        }

        // Virtual destructor
        virtual ~Hash() {}
    };
}
