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
//!
//!  @file
//!  DVS 042 cipher block chaining mode.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCipherChaining.h"

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

#include "tsDVS042Template.h"
