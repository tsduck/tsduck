//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVS 042 cipher block chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! DVS 042 cipher block chaining mode.
    //! @ingroup crypto
    //!
    //! DVS 042 has been renamed as "ANSI/SCTE 52 2003". It used to be available
    //! at http://www.scte.org/documents/pdf/ANSISCTE522003DVS042.pdf
    //! This file is no longer online. The next iteration of this standard
    //! is now "ANSI/SCTE 52 2008", available at
    //! http://www.scte.org/documents/pdf/Standards/ANSI_SCTE%2052%202008.pdf
    //!
    //! The only noticeable difference between the two versions is the handling
    //! of messages shorter than the block size. In the 2003 (DVS 042) version,
    //! the same IV (called "whitener" in the standard) is used for long and
    //! short messages. In the 2008 version, a different "whitener2" must be
    //! used for messages shorter than the block size.
    //!
    //! The ATIS-0800006 standard (IDSA) uses the same chaining mode and residue
    //! processing as DVS-042 but is based on AES instead of DES.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class DVS042: public CipherChainingTemplate<CIPHER>
    {
        TS_NOCOPY(DVS042);
    public:
        //!
        //! Constructor.
        //!
        DVS042();

        //!
        //! Set a new initialization vector for short blocks.
        //! The method setIV() sets the IV for @e long blocks (longer than the block size)
        //! and @e short blocks (shorter than the block size). The latter can then
        //! be overwritten using setShortIV().
        //! @param [in] iv_data Address of IV.
        //! @param [in] iv_length IV length in bytes.
        //! @return True on success, false on error.
        //!
        virtual bool setShortIV(const void* iv_data, size_t iv_length);

        // Implementation of BlockCipher and CipherChaining interfaces.
        // For some reason, doxygen is unable to automatically inherit the
        // documentation of *some* methods when a non-template class derives
        // from our template class. We need explicit copydoc directives.

        //! @copydoc ts::CipherChaining::minMessageSize()
        virtual size_t minMessageSize() const override;

        //! @copydoc ts::CipherChaining::residueAllowed()
        virtual bool residueAllowed() const override;

        //! @copydoc ts::CipherChaining::setIV()
        virtual bool setIV(const void* iv_data, size_t iv_length) override;

        //! @copydoc ts::BlockCipher::name()
        virtual UString name() const override;

    protected:
        //! @copydoc ts::BlockCipher::encryptImpl()
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;

        //! @copydoc ts::BlockCipher::decryptImpl()
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize,                              size_t* plain_length) override;

    protected:
        ByteBlock shortIV;  //!< Current initialization vector for short blocks.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

TS_PUSH_WARNING()
TS_MSC_NOWARNING(4505) // warning C4505: 'ts::DVS042<ts::AES>::encrypt': unreferenced local function has been removed

template<class CIPHER>
ts::DVS042<CIPHER>::DVS042() :
    CipherChainingTemplate<CIPHER>(1, 1, 1),
    shortIV(this->block_size)
{
}

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
