//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBlockCipher.h"
#include "tsBlockCipherAlertInterface.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::BlockCipher::~BlockCipher()
{
#if defined(TS_WINDOWS)
    if (_hkey != nullptr) {
        ::BCryptDestroyKey(_hkey);
        _hkey = nullptr;
    }
    _algo = nullptr;
#endif
}


//----------------------------------------------------------------------------
// Get the algorithm handle and subobject size with Windows BCrypt library.
// This is the default implementation, for subclasses not using BCrypt.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
void ts::BlockCipher::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    algo = nullptr;
    length = 0;
}
#endif


//----------------------------------------------------------------------------
// Get the current key.
//----------------------------------------------------------------------------

bool ts::BlockCipher::getKey(ByteBlock& key) const
{
    key = _current_key;
    return _key_set && isValidKeySize(key.size());
}


//----------------------------------------------------------------------------
// Schedule a new key.
//----------------------------------------------------------------------------

bool ts::BlockCipher::setKey(const void* key, size_t key_length)
{
    if (key == nullptr || !isValidKeySize(key_length)) {
        return false;
    }
    else {
        _key_encrypt_count = _key_decrypt_count = 0;
        _current_key.copy(key, key_length);
        _key_set = setKeyImpl(key, key_length);
        return _key_set;
    }
}


//----------------------------------------------------------------------------
// Check if encryption or decryption is allowed. Increment counters.
//----------------------------------------------------------------------------

bool ts::BlockCipher::allowEncrypt()
{
    // Check that a key was successfully set.
    if (!_key_set) {
        return false;
    }

    // Check encryption limitations.
    if (_key_encrypt_count >= _key_encrypt_max &&
        (_alert == nullptr || _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::ENCRYPTION_EXCEEDED)))
    {
        // Disallow encryption if no handler present or handler did not cancel the alert.
        return false;
    }

    // Notify first encryption.
    if (_key_encrypt_count == 0 && _alert != nullptr) {
        // Informational only.
        _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::FIRST_ENCRYPTION);
    }

    // Encryption allowed.
    _key_encrypt_count++;
    return true;
}

bool ts::BlockCipher::allowDecrypt()
{
    // Check that a key was successfully set.
    if (!_key_set) {
        return false;
    }

    // Check decryption limitations.
    if (_key_decrypt_count >= _key_decrypt_max &&
        (_alert == nullptr || _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::DECRYPTION_EXCEEDED)))
    {
        // Disallow decryption if no handler present or handler did not cancel the alert.
        return false;
    }

    // Notify first decryption.
    if (_key_decrypt_count == 0 && _alert != nullptr) {
        // Informational only.
        _alert->handleBlockCipherAlert(*this, BlockCipherAlertInterface::FIRST_DECRYPTION);
    }

    // Decryption allowed.
    _key_decrypt_count++;
    return true;
}


//----------------------------------------------------------------------------
// Encrypt one block of data.
//----------------------------------------------------------------------------

bool ts::BlockCipher::encrypt(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
    return allowEncrypt() && encryptImpl(plain, plain_length, cipher, cipher_maxsize, cipher_length);
}


//----------------------------------------------------------------------------
// Decrypt one block of data.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decrypt(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    return allowDecrypt() && decryptImpl(cipher, cipher_length, plain, plain_maxsize, plain_length);
}


//----------------------------------------------------------------------------
// Encrypt one block of data in place.
//----------------------------------------------------------------------------

bool ts::BlockCipher::encryptInPlace(void* data, size_t data_length, size_t* max_actual_length)
{
    return allowEncrypt() && encryptInPlaceImpl(data, data_length, max_actual_length);
}

bool ts::BlockCipher::encryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length)
{
    const size_t cipher_max_size = max_actual_length != nullptr ? *max_actual_length : data_length;

#if defined(TS_WINDOWS)
    // With Windows BCrypt, overlapping is possible without extra copy.
    if (_hkey != nullptr) {
        return encryptImpl(data, data_length, data, cipher_max_size, max_actual_length);
    }
#endif

    const ByteBlock plain(data, data_length);
    return encryptImpl(plain.data(), plain.size(), data, cipher_max_size, max_actual_length);
}


//----------------------------------------------------------------------------
// Decrypt one block of data in place.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decryptInPlace(void* data, size_t data_length, size_t* max_actual_length)
{
    return allowDecrypt() && decryptInPlaceImpl(data, data_length, max_actual_length);
}

bool ts::BlockCipher::decryptInPlaceImpl(void* data, size_t data_length, size_t* max_actual_length)
{
    const size_t plain_max_size = max_actual_length != nullptr ? *max_actual_length : data_length;

#if defined(TS_WINDOWS)
    // With Windows BCrypt, overlapping is possible without extra copy.
    if (_hkey != nullptr) {
        return decryptImpl(data, data_length, data, plain_max_size, max_actual_length);
    }
#endif

    const ByteBlock cipher(data, data_length);
    return decryptImpl(cipher.data(), cipher.size(), data, plain_max_size, max_actual_length);
}


//----------------------------------------------------------------------------
// Schedule a new key (implementation of algorithm-specific part).
// Default implementation for the system-provided cryptographic library.
//----------------------------------------------------------------------------

bool ts::BlockCipher::setKeyImpl(const void* key, size_t key_length)
{
#if defined(TS_WINDOWS)

    // Get a reference to algorithm provider the first time.
    if (_algo == nullptr) {
        size_t objlength = 0;
        getAlgorithm(_algo, objlength);
        if (_algo == nullptr) {
            return false;
        }
        // Allocate the "key object" for the rest of the life of this BlockCipher instance.
        _obj.resize(objlength);
    }
    // Terminate previous key session if not yet done.
    if (_hkey != nullptr) {
        ::BCryptDestroyKey(_hkey);
        _hkey = nullptr;
    }
    // Build a key data blob (header, followed by key).
    ByteBlock key_data(sizeof(::BCRYPT_KEY_DATA_BLOB_HEADER) + key_length);
    ::BCRYPT_KEY_DATA_BLOB_HEADER* header = reinterpret_cast<::BCRYPT_KEY_DATA_BLOB_HEADER*>(key_data.data());
    header->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    header->cbKeyData = ::ULONG(key_length);
    MemCopy(key_data.data() + sizeof(::BCRYPT_KEY_DATA_BLOB_HEADER), key, key_length);
    // Create a new key handle.
    if (::BCryptImportKey(_algo, nullptr, BCRYPT_KEY_DATA_BLOB, &_hkey, _obj.data(), ::ULONG(_obj.size()), ::PUCHAR(key_data.data()), ::ULONG(key_data.size()), 0) < 0) {
        return false;
    }
    return true;

#else
    return false;
#endif
}


//----------------------------------------------------------------------------
// Encrypt one block of data (implementation of algorithm-specific part).
// Default implementation for the system-provided cryptographic library.
//----------------------------------------------------------------------------

bool ts::BlockCipher::encryptImpl(const void* plain, size_t plain_length, void* cipher, size_t cipher_maxsize, size_t* cipher_length)
{
#if defined(TS_WINDOWS)

    ::ULONG retsize = 0;
    if (_hkey == nullptr || ::BCryptEncrypt(_hkey, ::PUCHAR(plain), ::ULONG(plain_length), nullptr, nullptr, 0, ::PUCHAR(cipher), ::ULONG(cipher_maxsize), &retsize, 0) < 0) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = size_t(retsize);
    }
    return true;

#else
    return false;
#endif
}


//----------------------------------------------------------------------------
// Decrypt one block of data (implementation of algorithm-specific part).
// Default implementation for the system-provided cryptographic library.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decryptImpl(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
#if defined(TS_WINDOWS)

    ::ULONG retsize = 0;
    if (_hkey == nullptr || ::BCryptDecrypt(_hkey, ::PUCHAR(cipher), ::ULONG(cipher_length), nullptr, nullptr, 0, ::PUCHAR(plain), ::ULONG(plain_maxsize), &retsize, 0) < 0) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = size_t(retsize);
    }
    return true;

#else
    return false;
#endif
}
