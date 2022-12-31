//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Cipher Text Stealing (CTS) mode.
//  Template class using a BlockCipher subclass as template argument.
//
//  Several incompatible designs of CTS exist. This one implements the
//  description in http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/
//  ciphertext%20stealing%20proposal.pdf
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CTS2<CIPHER>::minMessageSize() const
{
    return this->block_size;
}

template<class CIPHER>
bool ts::CTS2<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::CTS2<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CTS2";
}


//----------------------------------------------------------------------------
// Encryption in CTS mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS2<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < this->block_size ||
        plain_length < this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Encrypt all blocks in CBC mode, except the last one if partial

    uint8_t* previous = this->iv.data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*> (plain);
    uint8_t* ct = reinterpret_cast<uint8_t*> (cipher);

    while (plain_length >= this->block_size) {
        // work = previous-cipher XOR plain-text
        for (size_t i = 0; i < this->block_size; ++i) {
            this->work[i] = previous[i] ^ pt[i];
        }
        // cipher-text = encrypt (work)
        if (!this->algo->encrypt (this->work.data(), this->block_size, ct, this->block_size)) {
            return false;
        }
        // previous-cipher = cipher-text
        previous = ct;
        // advance one block
        ct += this->block_size;
        pt += this->block_size;
        plain_length -= this->block_size;
    }

    // Process final block if incomplete

    if (plain_length > 0) {
        // work = Cn-1 XOR Pn, truncated
        for (size_t i = 0; i < plain_length; ++i) {
            this->work[i] = previous[i] ^ pt[i];
        }
        // pad with tail of Cn-1
        for (size_t i = plain_length; i < this->block_size; ++i) {
            this->work[i] = previous[i];
        }
        // Cn = encrypt (work), truncating Cn-1
        if (!this->algo->encrypt (this->work.data(), this->block_size, previous + plain_length, this->block_size)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS2<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < this->block_size ||
        cipher_length < this->block_size ||
        plain_maxsize < cipher_length)
    {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Decrypt blocks in CBC mode. If the last block is partial,
    // stop before the last two blocks (complete one + partial one).

    const uint8_t* previous = this->iv.data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*> (cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*> (plain);

    const size_t residue_size = cipher_length % this->block_size;
    const size_t trick_size = residue_size == 0 ? 0 : this->block_size + residue_size;

    while (cipher_length > trick_size) {
        // work = decrypt (cipher-text)
        if (!this->algo->decrypt (ct, this->block_size, this->work.data(), this->block_size)) {
            return false;
        }
        // plain-text = previous-cipher XOR work
        for (size_t i = 0; i < this->block_size; ++i) {
            pt[i] = previous[i] ^ this->work[i];
        }
        // previous-cipher = cipher-text
        previous = ct;
        // advance one block
        ct += this->block_size;
        pt += this->block_size;
        cipher_length -= this->block_size;
    }

    // Process final two blocks.

    if (cipher_length > 0) {
        assert (cipher_length == trick_size);
        // The remaining size is exactly one complete block plus a partial one.
        // Say that the last plaintext is Pn and the previous one is Pn-1.
        // Say that the last ciphertext is Cn and the previous one is Cn-1.

        // work = decrypt (Cn)
        if (!this->algo->decrypt(ct + residue_size, this->block_size, this->work.data(), this->block_size)) {
            return false;
        }
        // Pn(truncated) = work(truncated) xor Cn-1(truncated)
        for (size_t i = 0; i < residue_size; ++i) {
            pt[this->block_size + i] = ct[i] ^ this->work[i];
        }
        // Rebuild Cn-1 in work
        ::memcpy(this->work.data(), ct, residue_size);  // Flawfinder: ignore: memcpy()
        // Pn-1 = decrypt (Cn-1)
        if (!this->algo->decrypt(this->work.data(), this->block_size, pt, this->block_size)) {
            return false;
        }
        // Pn-1 = Pn-1 xor previous cipher
        for (size_t i = 0; i < this->block_size; ++i) {
            pt[i] ^= previous[i];
        }
    }
    return true;
}
