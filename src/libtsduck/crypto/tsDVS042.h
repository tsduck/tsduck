//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVS 042 cipher block chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsMemory.h"

namespace ts {
    //!
    //! DVS 042 cipher block chaining mode.
    //! @ingroup libtsduck crypto
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
    template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
    class DVS042: public CIPHER
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
        //! @param [in] iv_data Address of initialization vector for short blocks.
        //! @param [in] iv_length IV length in bytes.
        //! @return True on success, false on error.
        //!
        bool setShortIV(const void* iv_data, size_t iv_length);

        //!
        //! Set a new initialization vector for short blocks.
        //! The method setIV() sets the IV for @e long blocks (longer than the block size)
        //! and @e short blocks (shorter than the block size). The latter can then
        //! be overwritten using setShortIV().
        //! @param [in] iv Initialization vector for short blocks.
        //! @return True on success, false on error.
        //!
        bool setShortIV(const ByteBlock& iv) { return setShortIV(iv.data(), iv.size()); }

    protected:
        //! Properties of this algorithm.
        //! @return A constant reference to the properties.
        static const BlockCipherProperties& Properties();

        //! Constructor for subclasses which add some properties, such as fixed IV.
        //! @param [in] props Constant reference to a block of properties of this block cipher.
        //! @param [in] ignore_short_iv Ingnore short IV, use the standard IV for short blocks.
        DVS042(const BlockCipherProperties& props, bool ignore_short_iv = false);

        // Implementation of BlockCipher interface.
        //! @cond nodoxygen
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) override;
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) override;
        //! @endcond

    private:
        bool _ignore_short_iv = false;
        ByteBlock _short_iv {};
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

TS_PUSH_WARNING()
TS_MSC_NOWARNING(4505) // warning C4505: 'ts::DVS042<ts::AES>::encrypt': unreferenced local function has been removed

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
const ts::BlockCipherProperties& ts::DVS042<CIPHER>::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(CIPHER::Properties(), u"DVS042", true, 0, 3, CIPHER::BLOCK_SIZE);
    return props;
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::DVS042<CIPHER>::DVS042() : CIPHER(DVS042::Properties())
{
}

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
ts::DVS042<CIPHER>::DVS042(const BlockCipherProperties& props, bool ignore_short_iv) :
    CIPHER(props),
    _ignore_short_iv(ignore_short_iv)
{
    props.assertCompatibleChaining(DVS042::Properties());
}



//----------------------------------------------------------------------------
// Set initialization vectors.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::DVS042<CIPHER>::setShortIV(const void* iv_data, size_t iv_length)
{
    if (this->properties.min_iv_size == 0 && iv_length == 0) {
        _short_iv.clear();
        return true;
    }
    else if (_ignore_short_iv || iv_data == nullptr || iv_length < this->properties.min_iv_size || iv_length > this->properties.max_iv_size) {
        _short_iv.clear();
        return false;
    }
    else {
        _short_iv.copy(iv_data, iv_length);
        return true;
    }
}


//----------------------------------------------------------------------------
// Encryption in DVS 042 mode.
// The algorithm is safe with overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::DVS042<CIPHER>::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();

    if (this->currentIV().size() != bsize || (!_ignore_short_iv && _short_iv.size() != 0 && _short_iv.size() != bsize) || cipher_maxsize < plain_length) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = plain_length;
    }

    // Select IV depending on block size. Short IV, if unset, is equal to IV.
    const uint8_t* previous = plain_length < bsize && !_ignore_short_iv && _short_iv.size() != 0 ? _short_iv.data() : this->currentIV().data();

    // Encrypt all blocks in CBC mode, except the last one if partial.
    const uint8_t* pt = reinterpret_cast<const uint8_t*>(plain);
    uint8_t* ct = reinterpret_cast<uint8_t*>(cipher);

    while (plain_length >= bsize) {
        // work = previous-cipher XOR plain-text
        MemXor(work1, previous, pt, bsize);
        // cipher-text = encrypt (work)
        if (!CIPHER::encryptImpl(work1, bsize, ct, bsize, nullptr)) {
            return false;
        }
        // previous-cipher = cipher-text
        previous = ct;
        // advance one block
        ct += bsize;
        pt += bsize;
        plain_length -= bsize;
    }

    // Process final block if incomplete
    if (plain_length > 0) {
        // work = encrypt (Cn-1), which is encrypt (shortIV) for short packets
        if (!CIPHER::encryptImpl(previous, bsize, work1, bsize, nullptr)) {
            return false;
        }
        // Cn = work XOR Pn, truncated
        MemXor(ct, work1, pt, plain_length);
    }
    return true;
}


//----------------------------------------------------------------------------
// Decryption in DVS 042 mode.
// The algorithm needs to specifically process overlapping buffers.
//----------------------------------------------------------------------------

template<class CIPHER> requires std::derived_from<CIPHER, ts::BlockCipher>
bool ts::DVS042<CIPHER>::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    const size_t bsize = this->properties.block_size;
    uint8_t* work1 = this->work.data();
    uint8_t* work2 = this->work.data() + bsize;
    uint8_t* work3 = this->work.data() + 2 * bsize;

    if (this->currentIV().size() != bsize || (!_ignore_short_iv && _short_iv.size() != 0 && _short_iv.size() != bsize) || plain_maxsize < cipher_length) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = cipher_length;
    }

    // Select IV depending on block size. Short IV, if unset, is equal to IV.
    const uint8_t* previous = cipher_length < bsize && !_ignore_short_iv && _short_iv.size() != 0 ? _short_iv.data() : this->currentIV().data();

    // Decrypt all blocks in CBC mode, except the last one if partial
    const uint8_t* ct = reinterpret_cast<const uint8_t*>(cipher);
    uint8_t* pt = reinterpret_cast<uint8_t*>(plain);

    while (cipher_length >= bsize) {
        // work = decrypt (cipher-text)
        if (!CIPHER::decryptImpl(ct, bsize, work1, bsize, nullptr)) {
            return false;
        }
        // plain-text = previous-cipher XOR work.
        // previous-cipher = cipher-text
        if (pt == ct) {
            // With overlapping buffer, need to save current cipher.
            MemCopy(work2, ct, bsize);
            MemXor(pt, previous, work1, bsize);
            previous = work2;
            std::swap(work2, work3);
        }
        else {
            MemXor(pt, previous, work1, bsize);
            previous = ct;
        }
        // advance one block
        ct += bsize;
        pt += bsize;
        cipher_length -= bsize;
    }

    // Process final block if incomplete
    if (cipher_length > 0) {
        // work = encrypt (Cn-1), which is encrypt (shortIV) for short packets
        if (!CIPHER::encryptImpl(previous, bsize, work1, bsize, nullptr)) {
            return false;
        }
        // Pn = work XOR Cn, truncated
        MemXor(pt, work1, ct, cipher_length);
    }
    return true;
}

TS_POP_WARNING()

#endif
