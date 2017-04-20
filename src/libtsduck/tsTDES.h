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
//!  Triple-DES (EDE) block cipher
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"

namespace ts {

    class TSDUCKDLL TDES: public BlockCipher
    {
    public:
        // Sizes in bytes
        static const size_t BLOCK_SIZE = 8;
        static const size_t KEY_SIZE = 24;
        static const size_t ROUNDS = 16;

        // Implementation of BlockCipher interface:
        virtual std::string name() const {return "TDES";}
        virtual size_t blockSize() const {return BLOCK_SIZE;}
        virtual size_t minKeySize() const {return KEY_SIZE;}
        virtual size_t maxKeySize() const {return KEY_SIZE;}
        virtual bool isValidKeySize (size_t size) const {return size == KEY_SIZE;}
        virtual size_t minRounds() const {return ROUNDS;}
        virtual size_t maxRounds() const {return ROUNDS;}
        virtual size_t defaultRounds() const {return ROUNDS;}

        virtual bool setKey (const void* key, size_t key_length, size_t rounds = 0);
        virtual bool encrypt (const void* plain, size_t plain_length,
                              void* cipher, size_t cipher_maxsize,
                              size_t* cipher_length = 0);
        virtual bool decrypt (const void* cipher, size_t cipher_length,
                              void* plain, size_t plain_maxsize,
                              size_t* plain_length = 0);

    private:
        uint32_t _ek[3][32];  // Encryption keys
        uint32_t _dk[3][32];  // Decryption keys
    };
}
