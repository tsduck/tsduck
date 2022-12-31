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
//!  Declaraction of classes ts::CipherChaining and ts::CipherChainingTemplate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBlockCipher.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base class of all cipher chaining modes.
    //! @ingroup crypto
    //!
    //! Each instance uses a specific block cipher.
    //! The combination of a block cipher and a chaining mode also implements
    //! the same interface as ts::BlockCipher.
    //!
    class TSDUCKDLL CipherChaining: public BlockCipher
    {
        TS_NOCOPY(CipherChaining);
    public:
        // Implementation of BlockCipher interface:
        virtual size_t blockSize() const override;
        virtual size_t minKeySize() const override;
        virtual size_t maxKeySize() const override;
        virtual bool isValidKeySize(size_t size) const override;
        virtual size_t minRounds() const override;
        virtual size_t maxRounds() const override;
        virtual size_t defaultRounds() const override;

        //!
        //! Set a new initialization vector.
        //! @param [in] iv_data Address of IV.
        //! @param [in] iv_length IV length in bytes.
        //! @return True on success, false on error.
        //!
        virtual bool setIV(const void* iv_data, size_t iv_length);

        //!
        //! Get the minimum IV sizes in bytes.
        //! @return The minimum IV sizes in bytes.
        //!
        virtual size_t minIVSize() const;

        //!
        //! Get the maximum IV sizes in bytes.
        //! @return The maximum IV sizes in bytes.
        //!
        virtual size_t maxIVSize() const;

        //!
        //! Get the minimum message size.
        //! Shorter data cannot be ciphered in this mode.
        //! @return The minimum message size.
        //!
        virtual size_t minMessageSize() const = 0;

        //!
        //! Check if the chaining mode can process residue after the last multiple of the block size.
        //! @return True if the chaining mode can process residue after the last multiple of the block size.
        //!
        virtual bool residueAllowed() const = 0;

    protected:
        // Protected fields, for chaining mode subclass implementation.
        BlockCipher* algo;        //!< An instance of the block cipher.
        const size_t block_size;  //!< Shortcut for algo->blockSize().
        const size_t iv_min_size; //!< IV min size in bytes.
        const size_t iv_max_size; //!< IV max size in bytes.
        ByteBlock    iv;          //!< Current initialization vector.
        ByteBlock    work;        //!< Temporary working buffer.

        //!
        //! Constructor for subclasses.
        //! @param [in,out] cipher An instance of block cipher.
        //! @param [in] iv_min_blocks Minimum IV size in multiples of cipher block size (default: 1).
        //! @param [in] iv_max_blocks Maximum IV size in multiples of cipher block size (default: 1).
        //! @param [in] work_blocks Temporary work buffer size in multiples of cipher block size (default: 1).
        //!
        CipherChaining(BlockCipher* cipher = nullptr,
                       size_t iv_min_blocks = 1,
                       size_t iv_max_blocks = 1,
                       size_t work_blocks = 1);

        // Implementation of BlockCipher interface:
        virtual bool setKeyImpl(const void* key, size_t key_length, size_t rounds) override;
    };

    //!
    //! Base class of all cipher chaining modes, template version.
    //! @ingroup crypto
    //!
    //! Each instance uses a specific block cipher.
    //! The combination of a block cipher and a chaining mode also implements
    //! the same interface as ts::BlockCipher.
    //!
    //! @tparam CIPHER A subclass of ts::BlockCipher, the underlying block cipher.
    //!
    template <class CIPHER>
    class CipherChainingTemplate: public CipherChaining
    {
        TS_NOCOPY(CipherChainingTemplate);
    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] iv_min_blocks Minimum IV size in multiples of cipher block size (default: 1).
        //! @param [in] iv_max_blocks Maximum IV size in multiples of cipher block size (default: 1).
        //! @param [in] work_blocks Temporary work buffer size in multiples of cipher block size (default: 1).
        //!
        CipherChainingTemplate(size_t iv_min_blocks = 1,
                               size_t iv_max_blocks = 1,
                               size_t work_blocks = 1);

        //!
        //! Destructor.
        //!
        virtual ~CipherChainingTemplate() override;
    };
}

#include "tsCipherChainingTemplate.h"
