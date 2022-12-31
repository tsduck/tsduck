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
//  description in:
//  1) Bruce Schneier, Applied Cryptography (2nd, Ed.), pp 191, 195
//  2) RFC 2040, The RC5, RC5-CBC, RC5-CBC-Pad, and RC5-CTS Algorithms
//  3) "CBC ciphertext stealing" in
//     http://en.wikipedia.org/wiki/Ciphertext_stealing
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CTS1<CIPHER>::minMessageSize() const
{
    return this->block_size + 1;
}

template<class CIPHER>
bool ts::CTS1<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::CTS1<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CTS1";
}


//----------------------------------------------------------------------------
// Encryption in CTS mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS1<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < 2 * this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Data shorter than block size cannot be processed, keep it clear
    // Note that CTS mode requires at least TWO blocks (the last one may
    // be incomplete). As a consequence, exactly one block is not enough.

    if (plain_length <= this->block_size) {
        return false;
    }

    // Encrypt all blocks in CBC mode, except the last one

    uint8_t* previous = this->iv.data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*> (plain);
    uint8_t* ct = reinterpret_cast<uint8_t*> (cipher);

    while (plain_length > this->block_size) {
        // work = previous-cipher XOR plain-text
        for (size_t i = 0; i < this->block_size; ++i) {
            this->work[i] = previous[i] ^ pt[i];
        }
        // cipher-text = encrypt (work)
        if (!this->algo->encrypt(this->work.data(), this->block_size, ct, this->block_size)) {
            return false;
        }
        // previous-cipher = cipher-text
        previous = ct;
        // advance one block
        ct += this->block_size;
        pt += this->block_size;
        plain_length -= this->block_size;
    }

    // Process final block (possibly incomplete)

    // padded = last partial block, zero-padded
    uint8_t* const padded = this->work.data() + this->block_size;
    Zero(padded, this->block_size);
    ::memcpy(padded, pt, plain_length);  // Flawfinder: ignore: memcpy()
    // work = previous-cipher XOR padded
    for (size_t i = 0; i < this->block_size; ++i) {
        this->work[i] = previous[i] ^ padded[i];
    }
    // padded = encrypt (work)
    if (!this->algo->encrypt(this->work.data(), this->block_size, padded, this->block_size)) {
        return false;
    }
    // Swap last two blocks and truncate last one
    ::memcpy(ct, previous, plain_length);          // Flawfinder: ignore: memcpy()
    ::memcpy(previous, padded, this->block_size);  // Flawfinder: ignore: memcpy()
    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS1<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < 2 * this->block_size ||
        plain_maxsize < cipher_length)
    {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Data shorter than block size cannot be encrypted.
    // Note that CTS mode requires at least TWO blocks (the last one may
    // be incomplete). As a consequence, exactly one block is not enough.

    if (cipher_length <= this->block_size) {
        return false;
    }

    // Decrypt blocks in CBC mode.
    // Stop before the last two blocks (complete one + possibly-partial one).

    const uint8_t* previous = this->iv.data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*> (cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*> (plain);

    while (cipher_length > 2 * this->block_size) {
        // work = decrypt (cipher-text)
        if (!this->algo->decrypt(ct, this->block_size, this->work.data(), this->block_size)) {
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
    // The remaining size is exactly one complete block plus a partial one.
    // Say that the last plaintext is Pn and the previous one is Pn-1.
    // Say that the last ciphertext is Cn and the previous one is Cn-1.
    // The content of the last two blocks is: Cn, Cn-1(truncated)

    // Size of last partial block
    size_t last_size = cipher_length - this->block_size;
    // Get Cn-1(truncated) unpadded in padded
    uint8_t* const padded = this->work.data() + this->block_size;
    ::memcpy(padded, ct + this->block_size, last_size);  // Flawfinder: ignore: memcpy()
    // Decrypt Cn -> get work = Cn-1 xor Pn(zero-padded)
    if (!this->algo->decrypt(ct, this->block_size, this->work.data(), this->block_size)) {
        return false;
    }
    // Obtain Pn = work(truncated) xor Cn-1(truncated),
    // move it to final place, in last partial block.
    for (size_t i = 0; i < last_size; ++i) {
        pt[this->block_size + i] = this->work[i] ^ padded[i];
    }
    // Build complete Cn-1 in padded:
    // First part already in padded, last part comes from work
    if (last_size < this->block_size) {
        ::memcpy(padded + last_size, this->work.data() + last_size, this->block_size - last_size);  // Flawfinder: ignore: memcpy()
    }
    // Decrypt Cn-1 -> get curblock = Cn-2 xor Pn-1
    if (!this->algo->decrypt(padded, this->block_size, pt, this->block_size)) {
        return false;
    }
    // Get Pn-1
    for (size_t i = 0; i < this->block_size; ++i) {
        pt[i] = pt[i] ^ previous[i];
    }

    return true;
}
