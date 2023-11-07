//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 4.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 4.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the
    //!  weird STMicroelectronics STi71xx ECB-CTS implementation.
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CTS4: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CTS4);
    public:
        //!
        //! Constructor.
        //!
        CTS4() : CipherChainingTemplate<CIPHER>(0, 0, 1) {}

        // Implementation of CipherChaining interface.
        virtual size_t minMessageSize() const override;
        virtual bool residueAllowed() const override;

        // Implementation of BlockCipher interface.
        virtual UString name() const override;

    protected:
        // Implementation of BlockCipher interface.
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
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
    std::memcpy(this->work.data(), pt + residue_size, this->block_size - residue_size);
    // Flawfinder: ignore: memcpy()
    std::memcpy(this->work.data() + this->block_size - residue_size, pt + this->block_size, residue_size);

    if (!this->algo->encrypt(this->work.data(), this->block_size, ct + residue_size, this->block_size)) {
        return false;
    }

    // Flawfinder: ignore: memcpy()
    std::memcpy(this->work.data(), pt, residue_size);
    // Flawfinder: ignore: memcpy()
    std::memcpy(this->work.data() + residue_size, ct + residue_size, this->block_size - residue_size);

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
    std::memcpy(this->work.data(), pt - this->block_size + cipher_length, this->block_size - cipher_length);
    // Flawfinder: ignore: memcpy()
    std::memcpy(this->work.data() + this->block_size - cipher_length, ct, cipher_length);

    if (!this->algo->decrypt(this->work.data(), this->block_size, pt - this->block_size + cipher_length, this->block_size)) {
        return false;
    }

    return true;
}
