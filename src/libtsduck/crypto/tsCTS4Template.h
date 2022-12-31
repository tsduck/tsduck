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
//  Cipher text Stealing (CTS) mode.
//  Template class using a BlockCipher subclass as template argument.
//
//  Several incompatible designs of CTS exist. This one implements the
//  weird ST 71xx ECB-CTS implementation.
//
//----------------------------------------------------------------------------

#pragma once


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CTS4<CIPHER>::minMessageSize() const
{
    return this->block_size + 1;
}

template<class CIPHER>
bool ts::CTS4<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::CTS4<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CTS4";
}


//----------------------------------------------------------------------------
// Encryption in CTS4 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS4<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->work.size() < this->block_size ||
        plain_length < this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* pt = reinterpret_cast<const uint8_t*> (plain);
    uint8_t* ct = reinterpret_cast<uint8_t*> (cipher);

    // Process in ECB mode, except the last 2 blocks

    while (plain_length > 2 * this->block_size) {
        if (!this->algo->encrypt(pt, this->block_size, ct, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        plain_length -= this->block_size;
    }

    // Process final two blocks.

    assert(plain_length > this->block_size);
    const size_t residue_size = plain_length - this->block_size;

    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data(), pt + residue_size, this->block_size - residue_size);
    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data() + this->block_size - residue_size, pt + this->block_size, residue_size);

    if (!this->algo->encrypt(this->work.data(), this->block_size, ct + residue_size, this->block_size)) {
        return false;
    }

    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data(), pt, residue_size);
    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data() + residue_size, ct + residue_size, this->block_size - residue_size);

    if (!this->algo->encrypt(this->work.data(), this->block_size, ct, this->block_size)) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CTS4 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CTS4<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->work.size() < this->block_size ||
        cipher_length < this->block_size ||
        plain_maxsize < cipher_length)
    {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*> (cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*> (plain);

    // Process in ECB mode, except the last block

    while (cipher_length > this->block_size) {
        if (!this->algo->decrypt(ct, this->block_size, pt, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        cipher_length -= this->block_size;
    }

    // Process final block

    assert(cipher_length <= this->block_size);

    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data(), pt - this->block_size + cipher_length, this->block_size - cipher_length);
    // Flawfinder: ignore: memcpy()
    ::memcpy(this->work.data() + this->block_size - cipher_length, ct, cipher_length);

    if (!this->algo->decrypt(this->work.data(), this->block_size, pt - this->block_size + cipher_length, this->block_size)) {
        return false;
    }

    return true;
}
