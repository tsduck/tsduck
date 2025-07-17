//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBlockCipher.h"
#include "tsBlockCipherAlertInterface.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::BlockCipher::BlockCipher(const BlockCipherProperties& props) :
    properties(props),
    work(props.work_blocks * props.block_size)
{
    if (props.fixed_iv != nullptr) {
        _current_iv.copy(props.fixed_iv, props.fixed_iv_size);
#if defined(TS_WINDOWS)
        _work_iv.resize(_current_iv.size());
#endif
    }
}

ts::BlockCipher::~BlockCipher()
{
    // Cleanup system-specific crypto library resources, if used.
#if defined(TS_WINDOWS)

    if (_hkey != nullptr) {
        ::BCryptDestroyKey(_hkey);
        _hkey = nullptr;
    }
    _algo = nullptr;

#elif !defined(TS_NO_OPENSSL)

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
// Algorithm name (informational only).
//----------------------------------------------------------------------------

ts::UString ts::BlockCipher::name() const
{
    UString n(properties.name);
    if (properties.chaining && properties.chaining_name != nullptr && properties.chaining_name[0] != '\0') {
        if (!n.empty()) {
            n.append(u"-");
        }
        n.append(properties.chaining_name);
    }
    return n;
}


//----------------------------------------------------------------------------
// Check if a size in bytes is a valid key size.
// Default implementation, when all key sizes are allowed in min..max.
//----------------------------------------------------------------------------

bool ts::BlockCipher::isValidKeySize(size_t size) const
{
    return size >= properties.min_key_size && size <= properties.max_key_size;
}

bool ts::BlockCipher::isValidIVSize(size_t size) const
{
    if (!properties.chaining || properties.fixed_iv != nullptr) {
        // No explicit IV is allowed.
        return size == 0;
    }
    else {
        return size >= properties.min_iv_size && size <= properties.max_iv_size;
    }
}


//----------------------------------------------------------------------------
// Get the algorithm handle. This is the default implementation, for
// subclasses not using OpenSSL or Windows BCrypt.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)

void ts::BlockCipher::getAlgorithm(::BCRYPT_ALG_HANDLE& algo, size_t& length, bool& ignore_iv) const
{
    algo = nullptr;
    length = 0;
    ignore_iv = false;
}

#elif !defined(TS_NO_OPENSSL)

const EVP_CIPHER* ts::BlockCipher::getAlgorithm() const
{
    return nullptr;
}

#endif


//----------------------------------------------------------------------------
// Schedule a new key.
//----------------------------------------------------------------------------

bool ts::BlockCipher::setKey(const void* key, size_t key_length, const void* iv, size_t iv_length)
{
    // Setting the key is mandatory.
    if (key == nullptr || !isValidKeySize(key_length)) {
        return false;
    }

    // Setting the IV is optional.
    const bool valid_iv = isValidIVSize(iv_length) && (iv != nullptr || iv_length == 0);
    if (iv != nullptr && !valid_iv) {
        return false;
    }

    _key_encrypt_count = _key_decrypt_count = 0;
    _current_key.copy(key, key_length);

    if (valid_iv || !_current_iv.empty()) {
        // Schedule the key now, the IV is either valid or unused or previously set.
        if (valid_iv && properties.fixed_iv == nullptr) {
            _current_iv.copy(iv, iv_length);
#if defined(TS_WINDOWS)
            _work_iv.resize(iv_length);
#endif
        }
        _key_set = setKeyImpl();
        return _key_set;
    }
    else {
        // Schedule the key later, when an IV is set
        return true;
    }
}


//----------------------------------------------------------------------------
// Set a new initialization vector without changing the key
//----------------------------------------------------------------------------

bool ts::BlockCipher::setIV(const void* iv, size_t iv_length)
{
    if ((iv == nullptr && iv_length > 0) || !isValidIVSize(iv_length)) {
        return false;
    }
    _current_iv.copy(iv, iv_length);
#if defined(TS_WINDOWS)
    _work_iv.resize(iv_length);
#endif
    if (_current_key.empty()) {
        // No key set, nothing else to do.
        return true;
    }
    else {
        // A key was already set, set it again, in case IV is used here.
        _key_set = setKeyImpl();
        return _key_set;
    }
}


//----------------------------------------------------------------------------
// Check if encryption or decryption is allowed. Increment counters.
//----------------------------------------------------------------------------

bool ts::BlockCipher::allowEncrypt()
{
    // Check that a key and IV were successfully set.
    if (!_key_set || _current_iv.size() < properties.min_iv_size || _current_iv.size() > properties.max_iv_size) {
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
    // Check that a key and IV were successfully set.
    if (!_key_set || _current_iv.size() < properties.min_iv_size || _current_iv.size() > properties.max_iv_size) {
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
    if (!allowEncrypt()) {
        return false;
    }
    // With Windows BCrypt and OpenSSL, overlapping is possible without extra copy.
    if (plain == cipher && !_can_process_in_place) {
        // Disjoint overlapping buffers.
        const ByteBlock plain2(plain, plain_length);
        return encryptImpl(plain2.data(), plain2.size(), cipher, cipher_maxsize, cipher_length);
    }
    else {
        return encryptImpl(plain, plain_length, cipher, cipher_maxsize, cipher_length);
    }
}


//----------------------------------------------------------------------------
// Decrypt one block of data.
//----------------------------------------------------------------------------

bool ts::BlockCipher::decrypt(const void* cipher, size_t cipher_length, void* plain, size_t plain_maxsize, size_t* plain_length)
{
    if (!allowDecrypt()) {
        return false;
    }
    // With Windows BCrypt and OpenSSL, overlapping is possible without extra copy.
    if (plain == cipher && !_can_process_in_place) {
        // Disjoint overlapping buffers.
        const ByteBlock cipher2(cipher, cipher_length);
        return decryptImpl(cipher2.data(), cipher2.size(), plain, plain_maxsize, plain_length);
    }
    else {
        return decryptImpl(cipher, cipher_length, plain, plain_maxsize, plain_length);
    }
}


//----------------------------------------------------------------------------
// Schedule a new key (implementation of algorithm-specific part).
// Default implementation for the system-provided cryptographic library.
//----------------------------------------------------------------------------

bool ts::BlockCipher::setKeyImpl()
{
#if defined(TS_WINDOWS)

    // Get a reference to algorithm provider the first time.
    if (_algo == nullptr) {
        size_t objlength = 0;
        getAlgorithm(_algo, objlength, _ignore_iv);
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
    ByteBlock key_data(sizeof(::BCRYPT_KEY_DATA_BLOB_HEADER) + _current_key.size());
    ::BCRYPT_KEY_DATA_BLOB_HEADER* header = reinterpret_cast<::BCRYPT_KEY_DATA_BLOB_HEADER*>(key_data.data());
    header->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    header->cbKeyData = ::ULONG(_current_key.size());
    MemCopy(key_data.data() + sizeof(::BCRYPT_KEY_DATA_BLOB_HEADER), _current_key.data(), _current_key.size());

    // Create a new key handle.
    if (::BCryptImportKey(_algo, nullptr, BCRYPT_KEY_DATA_BLOB, &_hkey, _obj.data(), ::ULONG(_obj.size()), ::PUCHAR(key_data.data()), ::ULONG(key_data.size()), 0) < 0) {
        return false;
    }
    return true;

#elif !defined(TS_NO_OPENSSL)

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

#else

    // No cryptographic library.
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
    const ::PUCHAR iv = _ignore_iv || _current_iv.empty() ? nullptr : ::PUCHAR(_work_iv.data());
    const ::ULONG  ivlen = ::ULONG(_current_iv.size());
    if (iv != nullptr) {
        // Preserve _current_iv, the IV is updated by BCryptEncrypt
        assert(_current_iv.size() == _work_iv.size());
        MemCopy(_work_iv.data(), _current_iv.data(), _work_iv.size());
    }
    if (_hkey == nullptr || ::BCryptEncrypt(_hkey, ::PUCHAR(plain), ::ULONG(plain_length), nullptr, iv, ivlen, ::PUCHAR(cipher), ::ULONG(cipher_maxsize), &retsize, 0) < 0) {
        return false;
    }
    if (cipher_length != nullptr) {
        *cipher_length = size_t(retsize);
    }
    return true;

#elif !defined(TS_NO_OPENSSL)

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
            OpenSSL::DebugErrors();
            return false;
        }
        if (EVP_EncryptInit_ex(_encrypt, _algo, nullptr, _current_key.data(), nullptr) <= 0 ||
            EVP_CIPHER_CTX_set_padding(_encrypt, 0) <= 0)
        {
            EVP_CIPHER_CTX_free(_encrypt);
            _encrypt = nullptr;
            OpenSSL::DebugErrors();
            return false;
        }
    }

    // Set the IV before each encryption.
    if (!_current_iv.empty() && EVP_EncryptInit_ex(_encrypt, nullptr, nullptr, nullptr, _current_iv.data()) <= 0) {
        return false;
    }
    // Perform complete encryption.
    const unsigned char* input = reinterpret_cast<const unsigned char*>(plain);
    unsigned char* output = reinterpret_cast<unsigned char*>(cipher);
    int output_len = 0;
    int final_len = 0;
    if (EVP_EncryptUpdate(_encrypt, output, &output_len, input, int(plain_length)) <= 0 ||
        EVP_EncryptFinal_ex(_encrypt, output + output_len, &final_len) <= 0)
    {
        OpenSSL::DebugErrors();
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

#else

    // No cryptographic library.
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
    const ::PUCHAR iv = _ignore_iv || _current_iv.empty() ? nullptr : ::PUCHAR(_work_iv.data());
    const ::ULONG  ivlen = ::ULONG(_current_iv.size());
    if (iv != nullptr) {
        // Preserve _current_iv, the IV is updated by BCryptEncrypt
        assert(_current_iv.size() == _work_iv.size());
        MemCopy(_work_iv.data(), _current_iv.data(), _work_iv.size());
    }
    if (_hkey == nullptr || ::BCryptDecrypt(_hkey, ::PUCHAR(cipher), ::ULONG(cipher_length), nullptr, iv, ivlen, ::PUCHAR(plain), ::ULONG(plain_maxsize), &retsize, 0) < 0) {
        return false;
    }
    if (plain_length != nullptr) {
        *plain_length = size_t(retsize);
    }
    return true;

#elif !defined(TS_NO_OPENSSL)

    // See comment in encryptImpl().
    if (plain_maxsize < cipher_length) {
        return false;
    }

    // Initialize key decryption context the first time.
    if (_decrypt == nullptr) {
        if ((_decrypt = EVP_CIPHER_CTX_new()) == nullptr) {
            OpenSSL::DebugErrors();
            return false;
        }
        if (EVP_DecryptInit_ex(_decrypt, _algo, nullptr, _current_key.data(), nullptr) <= 0 ||
            EVP_CIPHER_CTX_set_padding(_decrypt, 0) <= 0)
        {
            EVP_CIPHER_CTX_free(_decrypt);
            _decrypt = nullptr;
            OpenSSL::DebugErrors();
            return false;
        }
    }

    // Set the IV before each decryption.
    if (!_current_iv.empty() && EVP_DecryptInit_ex(_decrypt, nullptr, nullptr, nullptr, _current_iv.data()) <= 0) {
        return false;
    }
    // Perform complete decryption.
    const unsigned char* input = reinterpret_cast<const unsigned char*>(cipher);
    unsigned char* output = reinterpret_cast<unsigned char*>(plain);
    int output_len = 0;
    int final_len = 0;
    if (EVP_DecryptUpdate(_decrypt, output, &output_len, input, int(cipher_length)) <= 0 ||
        EVP_DecryptFinal_ex(_decrypt, output + output_len, &final_len) <= 0)
    {
        OpenSSL::DebugErrors();
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

#else

    // No cryptographic library.
    return false;

#endif
}
