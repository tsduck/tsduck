//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBlockCipher.h"
#include "tsBlockCipherAlertInterface.h"
#include "tsInitCryptoLibrary.h"
#include "tsFatal.h"


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
#else
    if (_encrypt != nullptr) {
        EVP_CIPHER_CTX_free(_encrypt);
        _encrypt = nullptr;
    }
    if (_decrypt != nullptr) {
        EVP_CIPHER_CTX_free(_decrypt);
        _decrypt = nullptr;
    }
    _algo = nullptr;
#endif
}


//----------------------------------------------------------------------------
// Get the algorithm handle. This is the default implementation, for
// subclasses not using BCrypt or OpenSSL.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

void ts::BlockCipher::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length) const
{
    algo = nullptr;
    length = 0;
}

#else

const EVP_CIPHER* ts::BlockCipher::getAlgorithm() const
{
    return nullptr;
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

    // With Windows BCrypt and OpenSSL, overlapping is possible without extra copy.
#if defined(TS_WINDOWS)
    if (_hkey != nullptr) {
#else
    if (_encrypt != nullptr) {
#endif
        return encryptImpl(data, data_length, data, cipher_max_size, max_actual_length);
    }

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

    // With Windows BCrypt and OpenSSL, overlapping is possible without extra copy.
#if defined(TS_WINDOWS)
    if (_hkey != nullptr) {
#else
    if (_decrypt != nullptr) {
#endif
        return decryptImpl(data, data_length, data, plain_max_size, max_actual_length);
    }

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

    // Get a reference to algorithm EVP the first time.
    if (_algo == nullptr && (_algo = getAlgorithm()) == nullptr) {
        return false;
    }

    // With OpenSSL EVP, a context is used either for encrypt or decrypt, but not both.
    // We keep two contexts, one for encrypt, one for decrypt. We don't want one single
    // cantext which is initialized for encrypt or decrypt when necessary to avoid too
    // frequent reinit when the application constantly switches between encrypt and
    // decrypt. We delay the initialization until necessary to avoid useless init when
    // the application uses only one operation, encrypt or decrypt. At that time, we
    // will use the key data from _current_key.

    // Terminate previous key contexts if not yet done.
    if (_encrypt != nullptr) {
        EVP_CIPHER_CTX_free(_encrypt);
        _encrypt = nullptr;
    }
    if (_decrypt != nullptr) {
        EVP_CIPHER_CTX_free(_decrypt);
        _decrypt = nullptr;
    }

    return true;

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

    // Problem with OpenSSL: there is no way to limit the amount of written data during encryption or decryption.
    // The application shall provide a "large enough" output buffer. From OpenSSL man page for EVP_EncryptUpdate:
    //   << The amount of data written depends on the block alignment of the encrypted data. For most ciphers and
    //   modes, the amount of data written can be anything from zero bytes to (inl + cipher_block_size - 1) bytes.
    //   For wrap cipher modes, the amount of data written can be anything from zero bytes to (inl + cipher_block_size)
    //   bytes. For stream ciphers, the amount of data written can be anything from zero bytes to inl bytes. Thus,
    //   the buffer pointed to by out must contain sufficient room for the operation being performed. The actual
    //   number of bytes written is placed in outl. >>
    // It is not feasible to claim that the output buffer shall be at least "inl + cipher_block_size" because ECB
    // always needs "inl" only. So, we only check for "inl" and then verify after encryption that no buffer overflow
    // occurred.
    if (cipher_maxsize < plain_length) {
        return false;
    }

    // Initialize key encryption context the first time.
    if (_encrypt == nullptr) {
        if ((_encrypt = EVP_CIPHER_CTX_new()) == nullptr) {
            PrintCryptographicLibraryErrors();
            return false;
        }
        if (EVP_EncryptInit(_encrypt, _algo, _current_key.data(), nullptr) <= 0 ||
            EVP_CIPHER_CTX_set_padding(_encrypt, 0) <= 0)
        {
            EVP_CIPHER_CTX_free(_encrypt);
            _encrypt = nullptr;
            PrintCryptographicLibraryErrors();
            return false;
        }
    }

    // Perform complete encryption.
    const unsigned char* input = reinterpret_cast<const unsigned char*>(plain);
    unsigned char* output = reinterpret_cast<unsigned char*>(cipher);
    int output_len = 0;
    int final_len = 0;
    if (EVP_EncryptUpdate(_encrypt, output, &output_len, input, int(plain_length)) <= 0 ||
        EVP_EncryptFinal(_encrypt, output + output_len, &final_len) <= 0)
    {
        PrintCryptographicLibraryErrors();
        return false;
    }
    output_len += final_len;
    // Check for potential buffer overflow (see comment above).
    if (cipher_maxsize < size_t(output_len)) {
        TS_FATAL("Buffer overflow in OpenSSL encryption");
    }
    if (cipher_length != nullptr) {
        *cipher_length = size_t(output_len);
    }
    return true;

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

    // See comment in encryptImpl().
    if (plain_maxsize < cipher_length) {
        return false;
    }

    // Initialize key decryption context the first time.
    if (_decrypt == nullptr) {
        if ((_decrypt = EVP_CIPHER_CTX_new()) == nullptr) {
            PrintCryptographicLibraryErrors();
            return false;
        }
        if (EVP_DecryptInit(_decrypt, _algo, _current_key.data(), nullptr) <= 0 ||
            EVP_CIPHER_CTX_set_padding(_decrypt, 0) <= 0)
        {
            EVP_CIPHER_CTX_free(_decrypt);
            _decrypt = nullptr;
            PrintCryptographicLibraryErrors();
            return false;
        }
    }

    // Perform complete decryption.
    const unsigned char* input = reinterpret_cast<const unsigned char*>(cipher);
    unsigned char* output = reinterpret_cast<unsigned char*>(plain);
    int output_len = 0;
    int final_len = 0;
    if (EVP_DecryptUpdate(_decrypt, output, &output_len, input, int(cipher_length)) <= 0 ||
        EVP_DecryptFinal(_decrypt, output + output_len, &final_len) <= 0)
    {
        PrintCryptographicLibraryErrors();
        return false;
    }
    output_len += final_len;
    // Check for potential buffer overflow (see comment above).
    if (plain_maxsize < size_t(output_len)) {
        TS_FATAL("Buffer overflow in OpenSSL decryption");
    }
    if (plain_length != nullptr) {
        *plain_length = size_t(output_len);
    }
    return true;

#endif
}
