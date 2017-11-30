//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  CppUnit test suite for cryptographic classes.
//
//----------------------------------------------------------------------------

#include "tsAES.h"
#include "tsDES.h"
#include "tsTDES.h"
#include "tsSHA1.h"
#include "tsSHA256.h"
#include "tsSHA512.h"
#include "tsMD5.h"
#include "tsECB.h"
#include "tsCBC.h"
#include "tsCTS1.h"
#include "tsCTS2.h"
#include "tsCTS3.h"
#include "tsCTS4.h"
#include "tsDVS042.h"
#include "tsSystemRandomGenerator.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;

#include "crypto/tv_aes.h"
#include "crypto/tv_aes_chain.h"
#include "crypto/tv_des.h"
#include "crypto/tv_des_chain.h"
#include "crypto/tv_tdes.h"
#include "crypto/tv_tdes_cbc.h"
#include "crypto/tv_sha1.h"
#include "crypto/tv_sha256.h"
#include "crypto/tv_sha512.h"
#include "crypto/tv_md5.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CryptoTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testAES();
    void testAESECB();
    void testAES_CBC();
    void testAES_CTS1();
    void testAES_CTS2();
    void testAES_CTS3();
    void testAES_CTS4();
    void testAES_DVS042();
    void testDES();
    void testTDES();
    void testTDES_CBC();
    void testDES_DVS042();
    void testSHA1();
    void testSHA256();
    void testSHA512();
    void testMD5();

    CPPUNIT_TEST_SUITE(CryptoTest);
    CPPUNIT_TEST(testAES);
    CPPUNIT_TEST(testAESECB);
    CPPUNIT_TEST(testAES_CBC);
    CPPUNIT_TEST(testAES_CTS1);
    CPPUNIT_TEST(testAES_CTS2);
    CPPUNIT_TEST(testAES_CTS3);
    CPPUNIT_TEST(testAES_CTS4);
    CPPUNIT_TEST(testAES_DVS042);
    CPPUNIT_TEST(testDES);
    CPPUNIT_TEST(testTDES);
    CPPUNIT_TEST(testTDES_CBC);
    CPPUNIT_TEST(testDES_DVS042);
    CPPUNIT_TEST(testSHA1);
    CPPUNIT_TEST(testSHA256);
    CPPUNIT_TEST(testSHA512);
    CPPUNIT_TEST(testMD5);
    CPPUNIT_TEST_SUITE_END();

private:
    void testCipher(ts::BlockCipher& algo,
                    size_t tv_index,
                    size_t tv_count,
                    const void* key,
                    size_t key_size,
                    const void* plain,
                    size_t plain_size,
                    const void* cipher,
                    size_t cipher_size);

    void testChaining(ts::CipherChaining& algo,
                      size_t tv_index,
                      size_t tv_count,
                      const void* key,
                      size_t key_size,
                      const void* iv,
                      size_t iv_size,
                      const void* plain,
                      size_t plain_size,
                      const void* cipher,
                      size_t cipher_size);

    void testChainingSizes(ts::CipherChaining& algo, int sizes, ...);

    void testHash(ts::Hash& algo,
                  size_t tv_index,
                  size_t tv_count,
                  const char* message,
                  const void* hash,
                  size_t hash_size);
};

CPPUNIT_TEST_SUITE_REGISTRATION(CryptoTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CryptoTest::setUp()
{
}

// Test suite cleanup method.
void CryptoTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void CryptoTest::testCipher(ts::BlockCipher& algo,
                            size_t tv_index,
                            size_t tv_count,
                            const void* key,
                            size_t key_size,
                            const void* plain,
                            size_t plain_size,
                            const void* cipher,
                            size_t cipher_size)
{
    const ts::UString name(algo.name() + " test vector " + ts::UString::Decimal(tv_index + 1) + "/" + ts::UString::Decimal(tv_count));
    std::vector<uint8_t> tmp(std::max(plain_size, cipher_size));
    size_t retsize;

    CPPUNIT_ASSERT(algo.setKey(key, key_size));

    CPPUNIT_ASSERT(algo.encrypt(plain, plain_size, &tmp[0], tmp.size(), &retsize));
    CPPUNIT_ASSERT_EQUAL(cipher_size, retsize);

    if (::memcmp(cipher, &tmp[0], cipher_size) != 0) {
        utest::Out()
            << "CryptoTest: " << name << ": encryption failed" << std::endl
            << "  Expected cipher: " << ts::UString::Dump(cipher, cipher_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned cipher: " << ts::UString::Dump(&tmp[0], retsize, ts::UString::SINGLE_LINE) << std::endl;
        CPPUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": encryption failed");
    }

    CPPUNIT_ASSERT(algo.decrypt(cipher, cipher_size, &tmp[0], tmp.size(), &retsize));
    CPPUNIT_ASSERT_EQUAL(plain_size, retsize);

    if (::memcmp(plain, &tmp[0], plain_size) != 0) {
        utest::Out()
            << "CryptoTest: " << name << ": decryption failed" << std::endl
            << "  Expected plain: " << ts::UString::Dump(plain, plain_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned plain: " << ts::UString::Dump(&tmp[0], retsize, ts::UString::SINGLE_LINE) << std::endl;
        CPPUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": decryption failed");
    }
}

void CryptoTest::testChaining(ts::CipherChaining& algo,
                              size_t tv_index,
                              size_t tv_count,
                              const void* key,
                              size_t key_size,
                              const void* iv,
                              size_t iv_size,
                              const void* plain,
                              size_t plain_size,
                              const void* cipher,
                              size_t cipher_size)
{
    CPPUNIT_ASSERT(algo.setIV(iv, iv_size));
    testCipher(algo, tv_index, tv_count, key, key_size, plain, plain_size, cipher, cipher_size);
}

void CryptoTest::testChainingSizes(ts::CipherChaining& algo, int sizes, ...)
{
    ts::SystemRandomGenerator prng;
    ts::ByteBlock key(algo.maxKeySize());
    ts::ByteBlock iv(algo.maxIVSize());

    int size = sizes;
    va_list ap;
    va_start(ap, sizes);
    while (size > 0) {

        const ts::UString name(algo.name() + " on " + ts::UString::Decimal(size) + " bytes");

        size_t retsize = 0;
        ts::ByteBlock plain(size);
        ts::ByteBlock cipher(size);
        ts::ByteBlock decipher(size);

        CPPUNIT_ASSERT(prng.read(key.data(), key.size()));
        CPPUNIT_ASSERT(prng.read(iv.data(), iv.size()));
        CPPUNIT_ASSERT(prng.read(plain.data(), plain.size()));
        CPPUNIT_ASSERT(algo.setKey(key.data(), key.size()));
        CPPUNIT_ASSERT(algo.setIV(iv.data(), iv.size()));

        CPPUNIT_ASSERT(algo.encrypt(&plain[0], plain.size(), &cipher[0], cipher.size(), &retsize));
        CPPUNIT_ASSERT_EQUAL(plain.size(), retsize);

        CPPUNIT_ASSERT(algo.decrypt(&cipher[0], retsize, &decipher[0], decipher.size(), &retsize));
        CPPUNIT_ASSERT_EQUAL(cipher.size(), retsize);

        if (::memcmp(&plain[0], &decipher[0], size) != 0) {
            utest::Out()
                << "CryptoTest: " << name << " failed" << std::endl
                << "  Initial plain: " << ts::UString::Dump(&plain[0], size, ts::UString::SINGLE_LINE) << std::endl
                << "  Returned plain: " << ts::UString::Dump(&decipher[0], size, ts::UString::SINGLE_LINE) << std::endl;
            CPPUNIT_FAIL("CryptoTest: " + name.toUTF8() + " failed");
        }

        size = va_arg(ap, int);
    }
    va_end(ap);
}

void CryptoTest::testHash(ts::Hash& algo,
                          size_t tv_index,
                          size_t tv_count,
                          const char* message,
                          const void* hash,
                          size_t hash_size)
{
    const ts::UString name(algo.name() + " test vector " + ts::UString::Decimal(tv_index + 1) + "/" + ts::UString::Decimal(tv_count));
    std::vector<uint8_t> tmp(2 * hash_size);
    size_t retsize;

    CPPUNIT_ASSERT(algo.init());
    CPPUNIT_ASSERT(algo.add(message, ::strlen(message)));
    CPPUNIT_ASSERT(algo.getHash(&tmp[0], tmp.size(), &retsize));
    CPPUNIT_ASSERT_EQUAL(hash_size, retsize);

    if (::memcmp(hash, &tmp[0], hash_size) != 0) {
        utest::Out()
            << "CryptoTest: " << name << " failed" << std::endl
            << "  Expected hash: " << ts::UString::Dump(hash, hash_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned hash: " << ts::UString::Dump(&tmp[0], retsize, ts::UString::SINGLE_LINE) << std::endl;
        CPPUNIT_FAIL("CryptoTest: " + name.toUTF8() + " failed");
    }
}

void CryptoTest::testAES()
{
    ts::AES aes;

    CPPUNIT_ASSERT(aes.blockSize()  == 16);
    CPPUNIT_ASSERT(aes.minKeySize() == 16);
    CPPUNIT_ASSERT(aes.maxKeySize() == 32);
    CPPUNIT_ASSERT(!aes.isValidKeySize(0));
    CPPUNIT_ASSERT(!aes.isValidKeySize(8));
    CPPUNIT_ASSERT(aes.isValidKeySize(16));
    CPPUNIT_ASSERT(aes.isValidKeySize(24));
    CPPUNIT_ASSERT(aes.isValidKeySize(32));
    CPPUNIT_ASSERT(!aes.isValidKeySize(64));
    CPPUNIT_ASSERT(aes.minRounds() == 10);
    CPPUNIT_ASSERT(aes.maxRounds() == 14);
    CPPUNIT_ASSERT(aes.defaultRounds() == 10);

    const size_t tv_count = sizeof(tv_aes) / sizeof(TV_AES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES* tv = tv_aes + tvi;
        testCipher(aes, tvi, tv_count, tv->key, tv->key_size, tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
    }
}

void CryptoTest::testAESECB()
{
    ts::ECB<ts::AES> ecb_aes;
    const size_t tv_count = sizeof(tv_ecb_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_ecb_aes + tvi;
        testChaining(ecb_aes, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }
}

void CryptoTest::testAES_CBC()
{
    ts::CBC<ts::AES> cbc_aes;
    const size_t tv_count = sizeof(tv_cbc_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_cbc_aes + tvi;
        testChaining(cbc_aes, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }
}

void CryptoTest::testAES_CTS1()
{
    ts::CTS1<ts::AES> cts1_aes;
    const size_t tv_count = sizeof(tv_cts_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_cts_aes + tvi;
        testChaining(cts1_aes, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }
}

void CryptoTest::testAES_CTS2()
{
    ts::CTS2<ts::AES> cts2_aes;
    testChainingSizes(cts2_aes, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_CTS3()
{
    ts::CTS3<ts::AES> cts3_aes;
    testChainingSizes(cts3_aes, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_CTS4()
{
    ts::CTS4<ts::AES> cts4_aes;
    testChainingSizes(cts4_aes, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_DVS042()
{
    ts::DVS042<ts::AES> dvs042_aes;
    testChainingSizes(dvs042_aes, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testDES()
{
    ts::DES des;

    CPPUNIT_ASSERT(des.blockSize()  == 8);
    CPPUNIT_ASSERT(des.minKeySize() == 8);
    CPPUNIT_ASSERT(des.maxKeySize() == 8);
    CPPUNIT_ASSERT(des.isValidKeySize(8));
    CPPUNIT_ASSERT(!des.isValidKeySize(0));
    CPPUNIT_ASSERT(!des.isValidKeySize(16));
    CPPUNIT_ASSERT(des.minRounds() == 16);
    CPPUNIT_ASSERT(des.maxRounds() == 16);
    CPPUNIT_ASSERT(des.defaultRounds() == 16);

    const size_t tv_count = sizeof(tv_des) / sizeof(TV_DES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_DES* tv = tv_des + tvi;
        testCipher(des, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
    }
}

void CryptoTest::testTDES()
{
    ts::TDES tdes;

    CPPUNIT_ASSERT(tdes.blockSize()  == 8);
    CPPUNIT_ASSERT(tdes.minKeySize() == 24);
    CPPUNIT_ASSERT(tdes.maxKeySize() == 24);
    CPPUNIT_ASSERT(tdes.isValidKeySize(24));
    CPPUNIT_ASSERT(!tdes.isValidKeySize(0));
    CPPUNIT_ASSERT(!tdes.isValidKeySize(8));
    CPPUNIT_ASSERT(!tdes.isValidKeySize(16));
    CPPUNIT_ASSERT(tdes.minRounds() == 16);
    CPPUNIT_ASSERT(tdes.maxRounds() == 16);
    CPPUNIT_ASSERT(tdes.defaultRounds() == 16);

    const size_t tv_count = sizeof(tv_tdes) / sizeof(TV_TDES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_TDES* tv = tv_tdes + tvi;
        testCipher(tdes, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
    }
}

void CryptoTest::testTDES_CBC()
{

    ts::CBC<ts::TDES> cbc_tdes;

    CPPUNIT_ASSERT(cbc_tdes.blockSize()  == 8);
    CPPUNIT_ASSERT(cbc_tdes.minKeySize() == 24);
    CPPUNIT_ASSERT(cbc_tdes.maxKeySize() == 24);
    CPPUNIT_ASSERT(cbc_tdes.isValidKeySize(24));
    CPPUNIT_ASSERT(!cbc_tdes.isValidKeySize(0));
    CPPUNIT_ASSERT(!cbc_tdes.isValidKeySize(8));
    CPPUNIT_ASSERT(!cbc_tdes.isValidKeySize(16));
    CPPUNIT_ASSERT(cbc_tdes.minRounds() == 16);
    CPPUNIT_ASSERT(cbc_tdes.maxRounds() == 16);
    CPPUNIT_ASSERT(cbc_tdes.defaultRounds() == 16);

    const size_t tv_count = sizeof(tv_tdes_cbc) / sizeof(TV_TDES_CBC);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_TDES_CBC* tv = tv_tdes_cbc + tvi;
        CPPUNIT_ASSERT(cbc_tdes.setIV(tv->iv, sizeof(tv->iv)));
        testCipher(cbc_tdes, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, tv->size, tv->cipher, tv->size);
    }
}

void CryptoTest::testDES_DVS042()
{
    ts::DVS042<ts::DES> dvs042_des;
    const size_t tv_count = sizeof(tv_dvs042_des) / sizeof(TV_DES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_DES_CHAIN* tv = tv_dvs042_des + tvi;
        CPPUNIT_ASSERT(dvs042_des.setIV(tv->iv, sizeof(tv->iv)));
        testCipher(dvs042_des, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }
}

void CryptoTest::testSHA1()
{
    ts::SHA1 sha1;
    CPPUNIT_ASSERT(sha1.hashSize() == 20);
    CPPUNIT_ASSERT(sha1.blockSize() == 64);
    const size_t tv_count = sizeof(tv_sha1) / sizeof(TV_SHA1);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA1* tv = tv_sha1 + tvi;
        testHash(sha1, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }
}

void CryptoTest::testSHA256()
{
    ts::SHA256 sha256;
    CPPUNIT_ASSERT(sha256.hashSize() == 32);
    CPPUNIT_ASSERT(sha256.blockSize() == 64);
    const size_t tv_count = sizeof(tv_sha256) / sizeof(TV_SHA256);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA256* tv = tv_sha256 + tvi;
        testHash(sha256, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }
}

void CryptoTest::testSHA512()
{
    ts::SHA512 sha512;
    CPPUNIT_ASSERT(sha512.hashSize() == 64);
    CPPUNIT_ASSERT(sha512.blockSize() == 128);
    const size_t tv_count = sizeof(tv_sha512) / sizeof(TV_SHA512);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA512* tv = tv_sha512 + tvi;
        testHash(sha512, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }
}

void CryptoTest::testMD5()
{
    ts::MD5 md5;

    CPPUNIT_ASSERT(md5.hashSize() == 16);
    CPPUNIT_ASSERT(md5.blockSize() == 64);

    const size_t tv_count = sizeof(tv_md5) / sizeof(TV_MD5);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_MD5* tv = tv_md5 + tvi;
        testHash(md5, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }
}
