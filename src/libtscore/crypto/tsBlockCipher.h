//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsCryptoLibrary.h"
#include "tsBlockCipherProperties.h"

namespace ts {

    class BlockCipherAlertInterface;

    //!
    //! Base class for all block ciphers.
    //! A block cipher may be a base encryption algorithm (one block) or includes a chaining mode.
    //! @ingroup libtscore crypto
    //!
    class TSCOREDLL BlockCipher
    {
        TS_NOBUILD_NOCOPY(BlockCipher);
    public:
        //!
        //! Virtual destructor.
        //!
        virtual ~BlockCipher();

        //!
        //! Algorithm name (informational only).
        //! @return The algorithm name.
        //!
        UString name() const;

        //!
        //! Size in bytes of the block used by the algorithm.
        //! @return The size in bytes of the block used by the algorithm.
        //!
        size_t blockSize() const { return properties.block_size; }

        //!
        //! Minimum key sizes in bytes.
        //! @return The minimum key sizes in bytes.
        //!
        size_t minKeySize() const { return properties.min_key_size; }

        //!
        //! Maximum key sizes in bytes.
        //! @return The maximum key sizes in bytes.
        //!
        size_t maxKeySize() const { return properties.max_key_size; }

        //!
        //! Check if a size in bytes is a valid key size.
        //! @param [in] size Suggested key size in bytes.
        //! @return True if @a size is a valid key size for the algorithm.
        //!
        virtual bool isValidKeySize(size_t size) const;

        //!
        //! Check if a size in bytes is a valid initialization vector size.
        //! @param [in] size Suggested IV size in bytes.
        //! @return True if @a size is a valid IV size for the algorithm.
        //!
        virtual bool isValidIVSize(size_t size) const;

        //!
        //! Check if this object is a base encryption algorithm (one block) or includes a chaining mode.
        //! @return True if this object includes a chaining mode, false otherwise.
        //!
        bool hasChainingMode() const { return properties.chaining; }

        //!
        //! Get the minimum initialization vector sizes in bytes.
        //! @return The minimum IV sizes in bytes. Zero if no chaining mode is defined.
        //!
        size_t minIVSize() const { return properties.min_iv_size; }

        //!
        //! Get the maximum initialization vector sizes in bytes.
        //! @return The maximum IV sizes in bytes. Zero if no chaining mode is defined.
        //!
        size_t maxIVSize() const { return properties.max_iv_size; }

        //!
        //! Get the minimum message size.
        //! Shorter data cannot be ciphered in this mode.
        //! @return The minimum message size. Always equal to blockSize() if no chaining mode is defined.
        //!
        size_t minMessageSize() const { return properties.min_message_size; }

        //!
        //! Check if the chaining mode can process residue after the last multiple of the block size.
        //! @return True if the chaining mode can process residue after the last multiple of the block size.
        //!
        bool residueAllowed() const { return properties.residue_allowed; }

        //!
        //! Schedule a new key and optional initialization vector.
        //! @param [in] key Address of key value.
        //! @param [in] key_length Key length in bytes.
        //! @param [in] iv Address of IV value (for chaining mode only). If null and an IV is already set, continue to use it.
        //! @param [in] iv_length IV length in bytes (for chaining mode only).
        //! @return True on success, false on error.
        //!
        bool setKey(const void* key, size_t key_length, const void* iv = nullptr, size_t iv_length = 0);

        //!
        //! Schedule a new key.
        //! @param [in] key Key value.
        //! @return True on success, false on error.
        //!
        bool setKey(const ByteBlock& key) { return setKey(key.data(), key.size()); }

        //!
        //! Schedule a new key and initialization vector.
        //! @param [in] key Key value.
        //! @param [in] iv IV value (for chaining mode only).
        //! @return True on success, false on error.
        //!
        bool setKey(const ByteBlock& key, const ByteBlock& iv) { return setKey(key.data(), key.size(), iv.data(), iv.size()); }

        //!
        //! Set a new initialization vector without changing the key, or before setting the key.
        //! Note that if you need to set the key and IV, it is usually much more efficient to do
        //! it in one call instead of two (and not only because of te two calls).
        //! @param [in] iv Address of IV value (for chaining mode only).
        //! @param [in] iv_length IV length in bytes (for chaining mode only).
        //! @return True on success, false on error.
        //!
        bool setIV(const void* iv, size_t iv_length);

        //!
        //! Set a new initialization vector without changing the key, or before setting the key.
        //! Note that if you need to set the key and IV, it is usually much more efficient to do
        //! it in one call instead of two (and not only because of te two calls).
        //! @param [in] iv IV value (for chaining mode only).
        //! @return True on success, false on error.
        //!
        bool setIV(const ByteBlock& iv) { return setIV(iv.data(), iv.size()); }

        //!
        //! Check if a current key is present and valid.
        //! @return True if a current key is present and valid.
        //!
        bool hasKey() const { return _key_set; }

        //!
        //! Get the current key.
        //! @return A constant reference to the current key value.
        //!
        const ByteBlock& currentKey() const { return _current_key; }

        //!
        //! Get the current initialization vector.
        //! @return A constant reference to the current initialization vector value.
        //!
        const ByteBlock& currentIV() const { return _current_iv; }

        //!
        //! Encrypt data.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text must have the block size
        //! of the algorithm. For cipher chainings, the acceptable message sizes depend on the chaining mode.
        //!
        //! Plain and cipher buffers may be identical (start at the same location). If they don't start at the
        //! same address, they may not overlap.
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
        //! Decrypt data.
        //!
        //! For pure block ciphers such as AES or DES, the plain text and cipher text must have the block size
        //! of the algorithm. For cipher chainings, the acceptable message sizes depend on the chaining mode.
        //!
        //! Plain and cipher buffers may be identical (start at the same location). If they don't start at the
        //! same address, they may not overlap.
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
        //! Properties for this block cipher object instance.
        //! Accessible to subclasses, but constant.
        //!
        const BlockCipherProperties& properties;

        //!
        //! Constructor for subclasses.
        //! @param [in] properties Constant reference to a block of properties of this block cipher.
        //! The reference is kept all along the life of the object instance. The referenced object
        //! is typically static.
        //!
        BlockCipher(const BlockCipherProperties& properties);

        //!
        //! Schedule a new key and optional initialization vector (implementation of algorithm-specific part).
        //! Must be implemented by the subclass if it does not use the system-provided cryptographic library.
        //! @return True on success, false on error.
        //!
        virtual bool setKeyImpl();

        //!
        //! Encrypt one block of data (implementation of algorithm-specific part).
        //! Must be implemented by the subclass if it does not use the system-provided cryptographic library.
        //! @param [in] plain Address of plain text.
        //! @param [in] plain_length Plain text length in bytes.
        //! @param [out] cipher Address of buffer for cipher text.
        //! @param [in] cipher_maxsize Size of @a cipher buffer.
        //! @param [out] cipher_length Returned actual size of cipher text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length);

        //!
        //! Decrypt one block of data (implementation of algorithm-specific part).
        //! Must be implemented by the subclass if it does not use the system-provided cryptographic library.
        //! @param [in] cipher Address of cipher text.
        //! @param [in] cipher_length Cipher text length in bytes.
        //! @param [out] plain Address of buffer for plain text.
        //! @param [in] plain_maxsize Size of @a plain buffer.
        //! @param [out] plain_length Returned actual size of plain text. Ignored if zero.
        //! @return True on success, false on error.
        //!
        virtual bool decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length);

        //!
        //! Inform the superclass that the subclass can encrypt and decrypt in place (identical in/out buffers).
        //! Typically called by a subclass in constructor.
        //! @param [in] can_do If true, encrypt and decrypt in place is possible.
        //!
        void canProcessInPlace(bool can_do) { _can_process_in_place = can_do; }

#if defined(TS_WINDOWS) || defined(DOXYGEN)
        //!
        //! Get the algorithm handle and subobject size, when the subclass uses Microsoft BCrypt library.
        //! @param [out] algo Handle to hash algorithm.
        //! @param [out] length Length in bytes of the subobject to allocate.
        //! @param [out] ignore_iv The IV shall not be passed to BCrypt.
        //!
        virtual void getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const;
#endif

#if (!defined(TS_WINDOWS) && !defined(TS_NO_OPENSSL)) || defined(DOXYGEN)
        //!
        //! Get the EVP for the cipher algorithm, when the subclass uses OpenSSL.
        //! @return EVP cipher.
        //!
        virtual const EVP_CIPHER* getAlgorithm() const;
#endif

    protected:
        ByteBlock work {}; //!< Temporary working buffer.

    private:
        bool      _can_process_in_place = false;      // The subclass can encrypt and decrypt in place (identical in/out buffers).
        bool      _key_set = false;                   // Current key successfully set.
        int       _cipher_id = 0;                     // Cipher identity (from application).
        size_t    _key_encrypt_count = 0;             // Number of times the current key was used for decryption.
        size_t    _key_decrypt_count = 0;             // Number of times the current key was used for decryption.
        size_t    _key_encrypt_max = UNLIMITED;       // Maximum number of times a key should be used for encryption.
        size_t    _key_decrypt_max = UNLIMITED;       // Maximum number of times a key should be used for decryption.
        ByteBlock _current_key {};                    // Current unscheduled key.
        ByteBlock _current_iv {};                     // Current initialization vector.
        BlockCipherAlertInterface* _alert = nullptr;  // Alert handler.

        // Check if encryption or decryption is allowed. Increment counters when allowed.
        bool allowEncrypt();
        bool allowDecrypt();

        // System-specific cryptographic library.
#if defined(TS_WINDOWS)
        ::BCRYPT_ALG_HANDLE _algo = nullptr;
        ::BCRYPT_KEY_HANDLE _hkey = nullptr;
        ByteBlock _obj {};
        ByteBlock _work_iv {}; // BCrypt update the IV for reuse, use this one preserve _current_iv
        bool _ignore_iv = false;
#elif !defined(TS_NO_OPENSSL)
        const EVP_CIPHER* _algo = nullptr;
        EVP_CIPHER_CTX* _encrypt = nullptr;
        EVP_CIPHER_CTX* _decrypt = nullptr;
#endif
    };
}
