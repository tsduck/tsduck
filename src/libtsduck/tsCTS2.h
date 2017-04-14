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
//  Cipher text Stealing (CTS) mode.
//  Template class using a BlockCipher subclass as template argument.
//
//
//  Several incompatible designs of CTS exist. This one implements the
//  description in http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/
//  ciphertext%20stealing%20proposal.pdf
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {

    template <class CIPHER>
    class CTS2: public CipherChainingTemplate<CIPHER>
    {
    public:
        // Constructor / destructor
        CTS2() : CipherChainingTemplate<CIPHER>(1, 1, 1) {}

        // Algorithm name.
        virtual std::string name() const {return this->algo == 0 ? "" : this->algo->name() + "-CTS2";}

        // Get minimum message size. Shorter data cannot be ciphered in this mode.
        virtual size_t minMessageSize() const {return this->block_size;}

        // Check if the chaining mode can process residue after the last multiple of the block size.
        virtual bool residueAllowed() const {return true;}

        // Encryption in CTS mode.
        // Plain size must be greater than the block size.
        // Return true on success, false on error.
        virtual bool encrypt (const void* plain, size_t plain_length,
                              void* cipher, size_t cipher_maxsize,
                              size_t* cipher_length = 0);

        // Decryption in CTS mode.
        // Cipher size must be greater than the block size.
        // Return true on success, false on error.
        virtual bool decrypt (const void* cipher, size_t cipher_length,
                              void* plain, size_t plain_maxsize,
                              size_t* plain_length = 0);
    };
}

#include "tsCTS2Template.h"
