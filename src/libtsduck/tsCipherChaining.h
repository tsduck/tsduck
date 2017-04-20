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
//!
//!  @file
//!  Declaraction of classes ts::CipherChaining and ts::CipherChainingTemplate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsByteBlock.h"

namespace ts {

    class TSDUCKDLL CipherChaining: public BlockCipher
    {
    public:
        // Implementation of BlockCipher interface:
        virtual size_t blockSize() const;
        virtual size_t minKeySize() const;
        virtual size_t maxKeySize() const;
        virtual bool isValidKeySize(size_t size) const;
        virtual size_t minRounds() const;
        virtual size_t maxRounds() const;
        virtual size_t defaultRounds() const;
        virtual bool setKey(const void* key, size_t key_length, size_t rounds = 0);

        // Set a new IV.
        virtual bool setIV(const void* iv, size_t iv_length);

        // Minimum and maximum IV sizes in bytes.
        virtual size_t minIVSize() const {return _iv_min_size;}
        virtual size_t maxIVSize() const {return _iv_max_size;}

        // Get minimum message size. Shorter data cannot be ciphered in this mode.
        virtual size_t minMessageSize() const = 0;

        // Check if the chaining mode can process residue after the last multiple of the block size.
        virtual bool residueAllowed() const = 0;

    protected:
        // Protected fields, for chaining mode subclass implementation.
        BlockCipher* algo;        // an instance of the block cipher
        size_t       block_size;  // shortcut for cipher_instance.blockSize()
        ByteBlock    iv;          // current initialization vector
        ByteBlock    work;        // temporary working buffer

        // Constructor for subclasses
        CipherChaining (BlockCipher* cipher = 0,    // an instance of block cipher
                        size_t iv_min_blocks = 1,   // min IV size in multiples of cipher block size
                        size_t iv_max_blocks = 1,   // max IV size in multiples of cipher block size
                        size_t work_blocks = 1);    // temp work buffer size in multiples of cipher block size

    private:
        // Private fields
        size_t _iv_min_size;  // IV min size in bytes
        size_t _iv_max_size;  // IV max size in bytes

        // Inaccesible operations
        CipherChaining(const CipherChaining&) = delete;
        CipherChaining& operator=(const CipherChaining&) = delete;
    };

    template <class CIPHER>
    class CipherChainingTemplate: public CipherChaining
    {
    protected:
        // Constructor for subclasses
        CipherChainingTemplate(size_t iv_min_blocks = 1, // min IV size in multiples of cipher block size
                               size_t iv_max_blocks = 1, // max IV size in multiples of cipher block size
                               size_t work_blocks = 1) : // temp work buffer size in multiples of cipher block size
            CipherChaining(new CIPHER, iv_min_blocks, iv_max_blocks, work_blocks)
        {
        }

        // Destructor
        virtual ~CipherChainingTemplate()
        {
            if (algo != 0) {
                delete algo;
                algo = 0;
            }
        }
    };
}
