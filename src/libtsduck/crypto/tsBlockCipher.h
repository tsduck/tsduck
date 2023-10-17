//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract interface of block ciphers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsByteBlock.h"

namespace ts {

    class BlockCipherAlertInterface;

    //!
    //! Abstract interface of block ciphers.
    //! @ingroup crypto
    //!
    class TSDUCKDLL BlockCipher
    {
        TS_NOCOPY(BlockCipher);
    public:
        //!
        //! Constructor.
        //!
        BlockCipher() = default;

        //!
        //! Virtual destructor.
        //!
        virtual ~BlockCipher();

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
        bool setKey(const void* key, size_t key_length, size_t rounds = 0);

        //!
        //! Get the current key.
        //! @param [out] key Current key value.
        //! @return True on success, false if the key is unset or invalid.
        //!
        bool getKey(ByteBlock& key) const;

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
        bool encrypt(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length = nullptr);

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
        bool decrypt(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length = nullptr);

        //!
        //! Encrypt one block of data in place.
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
        bool encryptInPlace(void* data, size_t data_length, size_t* max_actual_length = nullptr);

        //!
        //! Decrypt one block of data in place.
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
        bool decryptInPlace(void* data, size_t data_length, size_t* max_actual_length = nullptr);

        //!
        //! Get the number of times the current key was used for encryption.
        //! @return The number of times the current key was used for encryption.
        //!
        size_t encryptionCount() const { return _key_encrypt_count; }

        //!
        //! Get the number of times the current key was used for decryption.
        //! @return The number of times the current key was used for decryption.
        //!
        size_t decryptionCount() const { return _key_decrypt_count; }

        //!
        //! A constant meaning "may use a key an unlimited number of times".
        //!
        static constexpr size_t UNLIMITED = std::numeric_limits<size_t>::max();

        //!
        //! Set the maximum number of times a key should be used for encryption.
        //! The default initial value is UNLIMITED.
        //! @param [in] count The maximum number of times a key should be used for encryption.
        //!
        void setEncryptionMax(size_t count) { _key_encrypt_max = count; }

        //!
        //! Set the maximum number of times a key should be used for decryption.
        //! The default initial value is UNLIMITED.
        //! @param [in] count The maximum number of times a key should be used for decryption.
        //!
        void setDecryptionMax(size_t count) { _key_decrypt_max = count; }

        //!
        //! Get the maximum number of times a key should be used for encryption.
        //! @return The maximum number of times a key should be used for encryption.
        //!
        size_t encryptionMax() const { return _key_encrypt_max; }

        //!
        //! Get the maximum number of times a key should be used for decryption.
        //! @return The maximum number of times a key should be used for decryption.
        //!
        size_t decryptionMax() const { return _key_decrypt_max; }

        //!
        //! Set the handler to be notified on alert.
        //! Only one handler can be set at a time.
        //! @param [in] handler Handler to set. Use a null pointer to remove the handler.
        //!
        void setAlertHandler(BlockCipherAlertInterface* handler) { _alert = handler; }

        //!
        //! Set some arbitrary "cipher id" value.
        //! This value is chosen and set by the application and can be retrieved later.
        //! The cipher id is not interpreted by the block cipher engine, it is only stored for the application.
        //! The initial value of a cipher id is zero.
        //! @param [in] id Application-defined cipher id to assign.
        //!
        void setCipherId(int id) { _cipher_id = id; }

        //!
        //! Get the "cipher id" value, as previously stored by the application.
        //! @return The application-defined cipher id.
        //!
        int cipherId() const { return _cipher_id; }

    protected:
        //!
        //! Schedule a new key (implementation of algorithm-specific part).
        //! @param [in] key Address of key value.
        //! @param [in] key_length Key length in bytes.
        //! @param [in] rounds Requested number of rounds. If zero, the default is used.
        //! @return True on success, false on error.
        //!
        virtual bool setKeyImpl(const void* key, size_t key_length, size_t rounds) = 0;

        //!
        //! Encrypt one block of data (implementation of algorithm-specific part).
        //! @param [in] plain Address of plain text.
        //! @param [in] plain_length Plain text length in bytes.
        //! @param [out] cipher Address of buffer for cipher text.
        //! @param [in] cipher_maxsize Size of @a cipher buffer.
        //! @param [out] cipher_length Returned actual size of cipher text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length) = 0;

        //!
        //! Decrypt one block of data (implementation of algorithm-specific part).
        //! @param [in] cipher Address of cipher text.
        //! @param [in] cipher_length Cipher text length in bytes.
        //! @param [out] plain Address of buffer for plain text.
        //! @param [in] plain_maxsize Size of @a plain buffer.
        //! @param [out] plain_length Returned actual size of plain text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length) = 0;

        //!
        //! Encrypt one block of data in place (implementation of algorithm-specific part).
        //! The default implementation is to call encryptImpl() and copy the data.
        //! A subclass may provide a more efficient implementation.
        //! @param [in,out] data Address of data buffer to encrypt.
        //! @param [in] data_length Input plain text length in bytes.
        //! @param [in,out] max_actual_length Optional, ignored if zero.
        //! On input, contain the maximum size of the data buffer, which can be larger than @a data_length.
        //! On output, receive the actual size of the encrypted data. For pure block ciphers, this is the
        //! same as @a data_length. For cipher chainings with padding, this can be larger.
        //! @return True on success, false on error.
        //!
        virtual bool encryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length);

        //!
        //! Decrypt one block of data in place (implementation of algorithm-specific part).
        //! The default implementation is to call decryptImpl() and copy the data.
        //! A subclass may provide a more efficient implementation.
        //! @param [in,out] data Address of data buffer to decrypt.
        //! @param [in] data_length Input cipher text length in bytes.
        //! @param [in,out] max_actual_length Optional, ignored if zero.
        //! On input, contain the maximum size of the data buffer, which can be larger than @a data_length.
        //! On output, receive the actual size of the decrypted data. For pure block ciphers, this is the
        //! same as @a data_length. For cipher chainings with padding, this can be smaller.
        //! @return True on success, false on error.
        //!
        virtual bool decryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length);

    private:
        bool      _key_set = false;                   // Current key successfully set.
        int       _cipher_id = 0;                     // Cipher identity (from application).
        size_t    _key_encrypt_count = 0;             // Number of times the current key was used for decryption.
        size_t    _key_decrypt_count = 0;             // Number of times the current key was used for decryption.
        size_t    _key_encrypt_max {UNLIMITED};       // Maximum number of times a key should be used for encryption.
        size_t    _key_decrypt_max {UNLIMITED};       // Maximum number of times a key should be used for decryption.
        ByteBlock _current_key{};                     // Current unscheduled key.
        BlockCipherAlertInterface* _alert = nullptr;  // Alert handler.

        // Check if encryption or decryption is allowed. Increment counters.
        bool allowEncrypt();
        bool allowDecrypt();
    };
}
