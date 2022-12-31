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
//  DVS 042 cipher block chaining mode.
//  Template class using a BlockCipher subclass as template argument.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsMemory.h"

TS_PUSH_WARNING()
TS_MSC_NOWARNING(4505) // warning C4505: 'ts::DVS042<ts::AES>::encrypt': unreferenced local function has been removed


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

template<class CIPHER>
ts::DVS042<CIPHER>::DVS042() :
    CipherChainingTemplate<CIPHER>(1, 1, 1),
    shortIV(this->block_size)
{
}


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::DVS042<CIPHER>::minMessageSize() const
{
    return 0;
}

template<class CIPHER>
bool ts::DVS042<CIPHER>::residueAllowed() const
{
    return true;
}

template<class CIPHER>
ts::UString ts::DVS042<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-DVS042";
}


//----------------------------------------------------------------------------
// Set initialization vectors.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::DVS042<CIPHER>::setIV(const void* iv_data, size_t iv_length)
{
    const bool ok1 = CipherChainingTemplate<CIPHER>::setIV(iv_data, iv_length);
    const bool ok2 = setShortIV(iv_data, iv_length);
    return ok1 && ok2;
}

template<class CIPHER>
bool ts::DVS042<CIPHER>::setShortIV(const void* iv_data, size_t iv_length)
{
    if (this->iv_min_size == 0 && iv_length == 0) {
        shortIV.clear();
        return true;
    }
    else if (iv_data == nullptr || iv_length < this->iv_min_size || iv_length > this->iv_max_size) {
        shortIV.clear();
        return false;
    }
    else {
        shortIV.copy(iv_data, iv_length);
        return true;
    }
}


//----------------------------------------------------------------------------
// Encryption in DVS 042 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::DVS042<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->shortIV.size() != this->block_size ||
        this->work.size() < this->block_size ||
        cipher_maxsize < plain_length)
    {
        return false;
    }

    // Cipher length is always the same as plain length.
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Select IV depending on block size.
    const uint8_t* previous = plain_length < this->block_size ? this->shortIV.data() : this->iv.data();

    // Encrypt all blocks in CBC mode, except the last one if partial.
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length >= this->block_size) {
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

    // Process final block if incomplete
    if (plain_length > 0) {
        // work = encrypt (Cn-1), which is encrypt (shortIV) for short packets
        if (!this->algo->encrypt(previous, this->block_size, this->work.data(), this->block_size)) {
            return false;
        }
        // Cn = work XOR Pn, truncated
        for (size_t i = 0; i < plain_length; ++i) {
            ct[i] = this->work[i] ^ pt[i];
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decryption in DVS 042 mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::DVS042<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->shortIV.size() != this->block_size ||
        this->work.size() < this->block_size ||
        plain_maxsize < cipher_length)
    {
        return false;
    }

    // Deciphered length is always the same as cipher length.
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Select IV depending on block size.
    const uint8_t* previous = cipher_length < this->block_size ? this->shortIV.data() : this->iv.data();

    // Decrypt all blocks in CBC mode, except the last one if partial
    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length >= this->block_size) {
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

    // Process final block if incomplete
    if (cipher_length > 0) {
        // work = encrypt (Cn-1), which is encrypt (shortIV) for short packets
        if (!this->algo->encrypt(previous, this->block_size, this->work.data(), this->block_size)) {
            return false;
        }
        // Pn = work XOR Cn, truncated
        for (size_t i = 0; i < cipher_length; ++i) {
            pt[i] = this->work[i] ^ ct[i];
        }
    }
    return true;
}

TS_POP_WARNING()
