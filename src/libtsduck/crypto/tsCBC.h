//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Cipher Block Chaining (CBC) mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //! Cipher Block Chaining (CBC) mode.
    //!
    //! No padding is performed. The plain text and cipher text sizes must be
    //! multiples of the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //! @ingroup crypto
    //!
    template <class CIPHER>
    class CBC: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(CBC);
    public:
        //!
        //! Constructor.
        //!
        CBC() : CipherChainingTemplate<CIPHER>(1, 1, 1) {}

        // Implementation of BlockCipher and CipherChaining interfaces.
        // For some reason, doxygen is unable to automatically inherit the
        // documentation of *some* methods when a non-template class derives
        // from our template class. We need explicit copydoc directives.

        //! @copydoc ts::CipherChaining::minMessageSize()
        virtual size_t minMessageSize() const override;

        //! @copydoc ts::CipherChaining::residueAllowed()
        virtual bool residueAllowed() const override;

        //! @copydoc ts::BlockCipher::name()
        virtual UString name() const override;

    protected:
        //! @copydoc ts::BlockCipher::encryptImpl()
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;

        //! @copydoc ts::BlockCipher::decryptImpl()
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template<class CIPHER>
size_t ts::CBC<CIPHER>::minMessageSize() const
{
    return this->block_size;
}

template<class CIPHER>
bool ts::CBC<CIPHER>::residueAllowed() const
{
    return false;
}

template<class CIPHER>
ts::UString ts::CBC<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-CBC";
}


//----------------------------------------------------------------------------
// Encryption in CBC mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CBC<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < this->block_size ||
        plain_length % this->block_size != 0 ||
        cipher_maxsize < plain_length)
    {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* previous = this->iv.data();
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
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

    return true;
}


//----------------------------------------------------------------------------
// Decryption in CBC mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::CBC<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr ||
        this->iv.size() != this->block_size ||
        this->work.size() < this->block_size ||
        cipher_length % this->block_size != 0 ||
        plain_maxsize < cipher_length)
    {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* previous = this->iv.data();
    const uint8_t* ct = reinterpret_cast<const uint8_t*> (cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*> (plain);

    while (cipher_length > 0) {
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

    return true;
}
