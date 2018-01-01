//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
    //! used for messages shorter than the block size. To avoid confusion, this
    //! implementation does no allow the handling of messages shorter than the
    //! block size.
    //!
    //! DVS042 can process a residue. The plain text and cipher text sizes must be
    //! equal to or greater than the block size of the underlying block cipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class DVS042: public CipherChainingTemplate<CIPHER>
    {
    public:
        //!
        //! Constructor.
        //!
        DVS042() : CipherChainingTemplate<CIPHER>(1, 1, 1) {}

        // Implementation of CipherChaining interface.
        virtual size_t minMessageSize() const override {return this->block_size;}
        virtual bool residueAllowed() const override {return true;}

        // Implementation of BlockCipher interface.
        virtual UString name() const override {return this->algo == 0 ? UString() : this->algo->name() + u"-DVS042";}
        virtual bool encrypt(const void* plain, size_t plain_length,
                             void* cipher, size_t cipher_maxsize,
                             size_t* cipher_length = 0) override;
        virtual bool decrypt(const void* cipher, size_t cipher_length,
                             void* plain, size_t plain_maxsize,
                             size_t* plain_length = 0) override;
    };
}

#include "tsDVS042Template.h"
