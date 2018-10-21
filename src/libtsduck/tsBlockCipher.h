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
//!  Abstract interface of block ciphers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Abstract interface of block ciphers.
    //! @ingroup crypto
    //!
    class TSDUCKDLL BlockCipher
    {
    public:
        //!
        //! Algorithm name (informational only).
        //! @return The algorithm name.
        //!
        virtual UString name() const = 0;

        //!
        //! Size in bytes of the block used by the algorithm.
        //! @return The size in bytes of the block used by the algorithm.
        //!
        virtual size_t blockSize() const = 0;

        //!
        //! Minimum key sizes in bytes.
        //! @return The minimum key sizes in bytes.
        //!
        virtual size_t minKeySize() const = 0;

        //!
        //! Maximum key sizes in bytes.
        //! @return The maximum key sizes in bytes.
        //!
        virtual size_t maxKeySize() const = 0;

        //!
        //! Check if a size in bytes is a valid key size.
        //! @param [in] size Suggested key size in bytes.
        //! @return True if @a size is a valid key size for the algorithm.
        //!
        virtual bool isValidKeySize(size_t size) const = 0;

        //!
        //! Minimum number of rounds for the algorithm.
        //! @return The minimum number of rounds for the algorithm.
        //!
        virtual size_t minRounds() const = 0;

        //!
        //! Maximum number of rounds for the algorithm.
        //! @return The maximum number of rounds for the algorithm.
        //!
        virtual size_t maxRounds() const = 0;

        //!
        //! Default number of rounds for the algorithm.
        //! @return The default number of rounds for the algorithm.
        //!
        virtual size_t defaultRounds() const = 0;

        //!
        //! Schedule a new key.
        //! @param [in] key Address of key value.
        //! @param [in] key_length Key length in bytes.
        //! @param [in] rounds Requested number of rounds. If zero, the default is used.
        //! @return True on success, false on error.
        //!
        virtual bool setKey(const void* key, size_t key_length, size_t rounds = 0) = 0;

        //!
        //! Encrypt one block of data.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text
        //! must have the block size of the algorithm. For cipher chainings, the
        //! acceptable message sizes depend on the chaining mode.
        //!
        //! @param [in] plain Address of plain text.
        //! @param [in] plain_length Plain text length in bytes.
        //! @param [out] cipher Address of buffer for cipher text.
        //! @param [in] cipher_maxsize Size of @a cipher buffer.
        //! @param [out] cipher_length Returned actual size of cipher text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool encrypt(const void* plain, size_t plain_length,
                             void* cipher, size_t cipher_maxsize,
                             size_t* cipher_length = nullptr) = 0;

        //!
        //! Decrypt one block of data.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text
        //! must have the block size of the algorithm. For cipher chainings, the
        //! acceptable message sizes depend on the chaining mode.
        //!
        //! @param [in] cipher Address of cipher text.
        //! @param [in] cipher_length Cipher text length in bytes.
        //! @param [out] plain Address of buffer for plain text.
        //! @param [in] plain_maxsize Size of @a plain buffer.
        //! @param [out] plain_length Returned actual size of plain text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool decrypt(const void* cipher, size_t cipher_length,
                             void* plain, size_t plain_maxsize,
                             size_t* plain_length = nullptr) = 0;

        //!
        //! Encrypt one block of data in place.
        //!
        //! The default implementation is to call encrypt() and copy the data.
        //! A subclass may provide a more efficient implementation.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text
        //! must have the block size of the algorithm. For cipher chainings, the
        //! acceptable message sizes depend on the chaining mode.
        //!
        //! @param [in,out] data Address of data buffer to encrypt.
        //! @param [in] data_length Input plain text length in bytes.
        //! @param [in,out] max_actual_length Optional, ignored if zero.
        //! On input, contain the maximum size of the data buffer, which can be larger than @a data_length.
        //! On output, receive the actual size of the encrypted data. For pure block ciphers, this is the
        //! same as @a data_length. For cipher chainings with padding, this can be larger.
        //! @return True on success, false on error.
        //!
        virtual bool encryptInPlace(void* data, size_t data_length, size_t* max_actual_length = nullptr);

        //!
        //! Decrypt one block of data in place.
        //!
        //! The default implementation is to call decrypt() and copy the data.
        //! A subclass may provide a more efficient implementation.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text
        //! must have the block size of the algorithm. For cipher chainings, the
        //! acceptable message sizes depend on the chaining mode.
        //!
        //! @param [in,out] data Address of data buffer to decrypt.
        //! @param [in] data_length Input cipher text length in bytes.
        //! @param [in,out] max_actual_length Optional, ignored if zero.
        //! On input, contain the maximum size of the data buffer, which can be larger than @a data_length.
        //! On output, receive the actual size of the decrypted data. For pure block ciphers, this is the
        //! same as @a data_length. For cipher chainings with padding, this can be smaller.
        //! @return True on success, false on error.
        //!
        virtual bool decryptInPlace(void* data, size_t data_length, size_t* max_actual_length = nullptr);

        //!
        //! Virtual destructor.
        //!
        virtual ~BlockCipher() {}
    };
}
