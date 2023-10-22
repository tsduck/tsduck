//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Electronic Code Book (ECB) mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

namespace ts {
    //!
    //! Electronic Code Book (ECB) mode.
    //! @ingroup crypto
    //!
    //! No padding is performed. The plain text and cipher text sizes must be
    //! multiples of the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class ECB: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(ECB);
    public:
        //!
        //! Constructor.
        //!
        ECB() : CipherChainingTemplate<CIPHER>(0, 0, 0) {}

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
size_t ts::ECB<CIPHER>::minMessageSize() const
{
    return this->block_size;
}

template<class CIPHER>
bool ts::ECB<CIPHER>::residueAllowed() const
{
    return false;
}

template<class CIPHER>
ts::UString ts::ECB<CIPHER>::name() const
{
    return this->algo == nullptr ? UString() : this->algo->name() + u"-ECB";
}


//----------------------------------------------------------------------------
// Encryption in ECB mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::ECB<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    if (this->algo == nullptr || plain_length % this->block_size != 0 || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length > 0) {
        if (!this->algo->encrypt(pt, this->block_size, ct, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        plain_length -= this->block_size;
    }

    return true;
}


//----------------------------------------------------------------------------
// Decryption in ECB mode.
//----------------------------------------------------------------------------

template<class CIPHER>
bool ts::ECB<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (this->algo == nullptr || cipher_length % this->block_size != 0 || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length > 0) {
        if (!this->algo->decrypt(ct, this->block_size, pt, this->block_size)) {
            return false;
        }
        ct += this->block_size;
        pt += this->block_size;
        cipher_length -= this->block_size;
    }

    return true;
}
