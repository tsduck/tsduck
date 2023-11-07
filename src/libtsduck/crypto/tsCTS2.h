//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher text Stealing (CTS) mode, alternative 2.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //!  Cipher text Stealing (CTS) mode, alternative 2.
    //!  @ingroup crypto
    //!
    //!  Several incompatible designs of CTS exist. This one implements the description in
    //!  http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/ciphertext%20stealing%20proposal.pdf
    //!
    //!  CTS can process a residue. The plain text and cipher text sizes must be
    //!  equal to or greater than the block size of the underlying block cipher.
    //!
    //!  @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CTS2: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CTS2);
    public:
        //!
        //! Constructor.
        //!
        CTS2() : CipherChainingTemplate<CIPHER>(1, 1, 1) {}

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
        std::memcpy(this->work.data(), ct, residue_size);  // Flawfinder: ignore: memcpy()
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
