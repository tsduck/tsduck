//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for cryptographic classes.
//
//----------------------------------------------------------------------------

#include "tsAES128.h"
#include "tsAES256.h"
#include "tsDES.h"
#include "tsTDES.h"
#include "tsSHA1.h"
#include "tsSHA256.h"
#include "tsSHA512.h"
#include "tsECB.h"
#include "tsCBC.h"
#include "tsCTR.h"
#include "tsCTS1.h"
#include "tsCTS2.h"
#include "tsCTS3.h"
#include "tsCTS4.h"
#include "tsSCTE52.h"
#include "tsDVBCSA2.h"
#include "tsDVBCISSA.h"
#include "tsIDSA.h"
#include "tsTSPacket.h"
#include "tsSystemRandomGenerator.h"
#include "tsunit.h"
#include "utestTSUnitBenchmark.h"

#include "crypto/tv_aes.h"
#include "crypto/tv_aes_chain.h"
#include "crypto/tv_des.h"
#include "crypto/tv_des_chain.h"
#include "crypto/tv_tdes.h"
#include "crypto/tv_tdes_cbc.h"
#include "crypto/tv_dvb_csa2.h"
#include "crypto/tv_dvb_cissa.h"
#include "crypto/tv_atis_idsa.h"
#include "crypto/tv_sha1.h"
#include "crypto/tv_sha256.h"
#include "crypto/tv_sha512.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CryptoTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testAES();
    void testAES_ECB();
    void testAES_CBC();
    void testAES_CTR();
    void testAES_CTS1();
    void testAES_CTS2();
    void testAES_CTS3();
    void testAES_CTS4();
    void testAES_DVS042();
    void testDES();
    void testTDES();
    void testTDES_CBC();
    void testDVBCSA2();
    void testDVBCISSA();
    void testIDSA();
    void testSCTE52_2003();
    void testSCTE52_2008();
    void testSHA1();
    void testSHA256();
    void testSHA512();

    TSUNIT_TEST_BEGIN(CryptoTest);
    TSUNIT_TEST(testAES);
    TSUNIT_TEST(testAES_ECB);
    TSUNIT_TEST(testAES_CBC);
    TSUNIT_TEST(testAES_CTR);
    TSUNIT_TEST(testAES_CTS1);
    TSUNIT_TEST(testAES_CTS2);
    TSUNIT_TEST(testAES_CTS3);
    TSUNIT_TEST(testAES_CTS4);
    TSUNIT_TEST(testAES_DVS042);
    TSUNIT_TEST(testDES);
    TSUNIT_TEST(testTDES);
    TSUNIT_TEST(testTDES_CBC);
    TSUNIT_TEST(testDVBCSA2);
    TSUNIT_TEST(testDVBCISSA);
    TSUNIT_TEST(testIDSA);
    TSUNIT_TEST(testSCTE52_2003);
    TSUNIT_TEST(testSCTE52_2008);
    TSUNIT_TEST(testSHA1);
    TSUNIT_TEST(testSHA256);
    TSUNIT_TEST(testSHA512);
    TSUNIT_TEST_END();

private:
    void testCipher(utest::TSUnitBenchmark& bench,
                    ts::BlockCipher& algo,
                    size_t tv_index,
                    size_t tv_count,
                    const void* key,
                    size_t key_size,
                    const void* plain,
                    size_t plain_size,
                    const void* cipher,
                    size_t cipher_size);

    void testChaining(utest::TSUnitBenchmark& bench,
                      ts::BlockCipher& algo,
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

    void testChainingSizes(ts::BlockCipher& algo, int sizes, ...);

    void testHash(utest::TSUnitBenchmark& bench,
                  ts::Hash& algo,
                  size_t tv_index,
                  size_t tv_count,
                  const char* message,
                  const void* hash,
                  size_t hash_size);
};

TSUNIT_REGISTER(CryptoTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CryptoTest::beforeTest()
{
}

// Test suite cleanup method.
void CryptoTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void CryptoTest::testCipher(utest::TSUnitBenchmark& bench,
                            ts::BlockCipher& algo,
                            size_t tv_index,
                            size_t tv_count,
                            const void* key,
                            size_t key_size,
                            const void* plain,
                            size_t plain_size,
                            const void* cipher,
                            size_t cipher_size)
{
    const ts::UString name(ts::UString::Format(u"%s test vector %d/%d", {algo.name(), tv_index + 1, tv_count}));
    ts::ByteBlock tmp(std::max(plain_size, cipher_size));
    size_t retsize = 0;

    TSUNIT_ASSERT(algo.setKey(key, key_size));

    bool encrypt_ok = true;
    bench.start();
    for (size_t iter = 0; iter < bench.iterations; ++iter) {
        encrypt_ok = algo.encrypt(plain, plain_size, tmp.data(), tmp.size(), &retsize) && encrypt_ok;
    }
    bench.stop();
    TSUNIT_ASSERT(encrypt_ok);
    TSUNIT_EQUAL(cipher_size, retsize);

    if (!ts::MemEqual(cipher, tmp.data(), cipher_size)) {
        debug()
            << "CryptoTest: " << name << ": encryption failed" << std::endl
            << "  Expected cipher: " << ts::UString::Dump(cipher, cipher_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned cipher: " << ts::UString::Dump(tmp.data(), retsize, ts::UString::SINGLE_LINE) << std::endl;
        TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": encryption failed");
    }

    bool decrypt_ok = true;
    bench.start();
    for (size_t iter = 0; iter < bench.iterations; ++iter) {
        decrypt_ok = algo.decrypt(cipher, cipher_size, tmp.data(), tmp.size(), &retsize) && decrypt_ok;
    }
    bench.stop();
    TSUNIT_ASSERT(decrypt_ok);
    TSUNIT_EQUAL(plain_size, retsize);

    if (!ts::MemEqual(plain, tmp.data(), plain_size)) {
        debug()
            << "CryptoTest: " << name << ": decryption failed" << std::endl
            << "  Expected plain: " << ts::UString::Dump(plain, plain_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned plain: " << ts::UString::Dump(tmp.data(), retsize, ts::UString::SINGLE_LINE) << std::endl;
        TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": decryption failed");
    }

    // Test encrypt "in place" (same input and output buffer).
    ts::MemCopy(tmp.data(), plain, plain_size);
    retsize = 0;
    TSUNIT_ASSERT(algo.encrypt(tmp.data(), plain_size, tmp.data(), tmp.size(), &retsize));
    TSUNIT_EQUAL(cipher_size, retsize);

    if (!ts::MemEqual(cipher, tmp.data(), cipher_size)) {
        debug()
            << "CryptoTest: " << name << ": encrypt 'in place' failed" << std::endl
            << "  Expected cipher: " << ts::UString::Dump(cipher, cipher_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned cipher: " << ts::UString::Dump(tmp.data(), retsize, ts::UString::SINGLE_LINE) << std::endl;
        TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": encrypt 'in place' failed");
    }

    // Test decrypt "in place" (same input and output buffer).
    ts::MemCopy(tmp.data(), cipher, cipher_size);
    retsize = 0;
    TSUNIT_ASSERT(algo.decrypt(tmp.data(), cipher_size, tmp.data(), tmp.size(), &retsize));
    TSUNIT_EQUAL(plain_size, retsize);

    if (!ts::MemEqual(plain, tmp.data(), plain_size)) {
        debug()
            << "CryptoTest: " << name << ": decrypt 'in place' failed" << std::endl
            << "  Expected plain: " << ts::UString::Dump(plain, plain_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned plain: " << ts::UString::Dump(tmp.data(), retsize, ts::UString::SINGLE_LINE) << std::endl;
        TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + ": decrypt 'in place' failed");
    }
}

void CryptoTest::testChaining(utest::TSUnitBenchmark& bench,
                              ts::BlockCipher& algo,
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
    TSUNIT_ASSERT(algo.setIV(iv, iv_size));
    testCipher(bench, algo, tv_index, tv_count, key, key_size, plain, plain_size, cipher, cipher_size);
}

void CryptoTest::testChainingSizes(ts::BlockCipher& algo, int sizes, ...)
{
    ts::SystemRandomGenerator prng;
    ts::ByteBlock key(algo.maxKeySize());
    ts::ByteBlock iv(algo.maxIVSize());

    int size = sizes;
    va_list ap;
    va_start(ap, sizes);
    while (size > 0) {

        const ts::UString name(ts::UString::Format(u"%s on %d bytes", {algo.name(), size}));

        size_t retsize = 0;
        ts::ByteBlock plain(size);
        ts::ByteBlock cipher(size);
        ts::ByteBlock decipher(size);

        TSUNIT_ASSERT(prng.read(key.data(), key.size()));
        TSUNIT_ASSERT(prng.read(iv.data(), iv.size()));
        TSUNIT_ASSERT(prng.read(plain.data(), plain.size()));
        TSUNIT_ASSERT(algo.setKey(key.data(), key.size()));
        TSUNIT_ASSERT(algo.setIV(iv.data(), iv.size()));

        TSUNIT_ASSERT(algo.encrypt(plain.data(), plain.size(), cipher.data(), cipher.size(), &retsize));
        TSUNIT_EQUAL(plain.size(), retsize);

        TSUNIT_ASSERT(algo.decrypt(cipher.data(), retsize, decipher.data(), decipher.size(), &retsize));
        TSUNIT_EQUAL(cipher.size(), retsize);

        if (!ts::MemEqual(plain.data(), decipher.data(), size)) {
            debug()
                << "CryptoTest: " << name << " failed" << std::endl
                << "  Initial plain: " << ts::UString::Dump(plain.data(), size, ts::UString::SINGLE_LINE) << std::endl
                << "  Returned plain: " << ts::UString::Dump(decipher.data(), size, ts::UString::SINGLE_LINE) << std::endl;
            TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + " failed");
        }

        // Same data but encrypt/decrypt in place.
        ts::ByteBlock inplace(plain);

        TSUNIT_ASSERT(algo.encrypt(inplace.data(), inplace.size(), inplace.data(), inplace.size(), &retsize));
        TSUNIT_EQUAL(inplace.size(), retsize);

        if (!ts::MemEqual(inplace.data(), cipher.data(), size)) {
            debug()
                << "CryptoTest: " << name << " encrypt 'in place' failed" << std::endl
                << "  Initial cipher: " << ts::UString::Dump(cipher.data(), size, ts::UString::SINGLE_LINE) << std::endl
                << "  Returned cipher: " << ts::UString::Dump(inplace.data(), size, ts::UString::SINGLE_LINE) << std::endl;
            TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + " encrypt 'in place' failed");
        }

        TSUNIT_ASSERT(algo.decrypt(inplace.data(), inplace.size(), inplace.data(), inplace.size(), &retsize));
        TSUNIT_EQUAL(inplace.size(), retsize);

        if (!ts::MemEqual(inplace.data(), plain.data(), size)) {
            debug()
                << "CryptoTest: " << name << " decrypt 'in place' failed" << std::endl
                << "  Initial plain: " << ts::UString::Dump(plain.data(), size, ts::UString::SINGLE_LINE) << std::endl
                << "  Returned plain: " << ts::UString::Dump(inplace.data(), size, ts::UString::SINGLE_LINE) << std::endl;
            TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + " decrypt 'in place' failed");
        }

        size = va_arg(ap, int);
    }
    va_end(ap);
}

void CryptoTest::testHash(utest::TSUnitBenchmark& bench,
                          ts::Hash& algo,
                          size_t tv_index,
                          size_t tv_count,
                          const char* message,
                          const void* hash,
                          size_t hash_size)
{
    const ts::UString name(ts::UString::Format(u"%s test vector %d/%d", {algo.name(), tv_index + 1, tv_count}));
    ts::ByteBlock tmp(2 * hash_size);
    size_t retsize = 0;
    const size_t message_length = std::strlen(message);

    TSUNIT_ASSERT(algo.init());
    TSUNIT_ASSERT(algo.add(message, message_length));
    TSUNIT_ASSERT(algo.getHash(tmp.data(), tmp.size(), &retsize));
    TSUNIT_EQUAL(hash_size, retsize);

    if (!ts::MemEqual(hash, tmp.data(), hash_size)) {
        debug()
            << "CryptoTest: " << name << " failed" << std::endl
            << "  Expected hash: " << ts::UString::Dump(hash, hash_size, ts::UString::SINGLE_LINE) << std::endl
            << "  Returned hash: " << ts::UString::Dump(tmp.data(), retsize, ts::UString::SINGLE_LINE) << std::endl;
        TSUNIT_FAIL("CryptoTest: " + name.toUTF8() + " failed");
    }
    else if (bench.iterations > 1) {
        TSUNIT_ASSERT(algo.init());
        bool ok = true;
        bench.start();
        for (size_t iter = 0; iter < bench.iterations; ++iter) {
            ok = algo.add(message, message_length) && ok;
        }
        bench.stop();
        TSUNIT_ASSERT(ok);
    }
}

void CryptoTest::testAES()
{
    ts::AES128 aes128;
    ts::AES256 aes256;

    TSUNIT_ASSERT(aes128.blockSize()  == 16);
    TSUNIT_ASSERT(aes128.minKeySize() == 16);
    TSUNIT_ASSERT(aes128.maxKeySize() == 16);
    TSUNIT_ASSERT(!aes128.isValidKeySize(0));
    TSUNIT_ASSERT(!aes128.isValidKeySize(8));
    TSUNIT_ASSERT(aes128.isValidKeySize(16));
    TSUNIT_ASSERT(!aes128.isValidKeySize(24));
    TSUNIT_ASSERT(!aes128.isValidKeySize(32));
    TSUNIT_ASSERT(!aes128.isValidKeySize(64));

    TSUNIT_ASSERT(aes256.blockSize() == 16);
    TSUNIT_ASSERT(aes256.minKeySize() == 32);
    TSUNIT_ASSERT(aes256.maxKeySize() == 32);
    TSUNIT_ASSERT(!aes256.isValidKeySize(0));
    TSUNIT_ASSERT(!aes256.isValidKeySize(8));
    TSUNIT_ASSERT(!aes256.isValidKeySize(16));
    TSUNIT_ASSERT(!aes256.isValidKeySize(24));
    TSUNIT_ASSERT(aes256.isValidKeySize(32));
    TSUNIT_ASSERT(!aes256.isValidKeySize(64));

    utest::TSUnitBenchmark bench(u"TSUNIT_AES_ITERATIONS");

    const size_t tv_count = sizeof(tv_aes) / sizeof(TV_AES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES* tv = tv_aes + tvi;
        if (tv->key_size == ts::AES128::KEY_SIZE) {
            testCipher(bench, aes128, tvi, tv_count, tv->key, tv->key_size, tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
        }
        else if (tv->key_size == ts::AES256::KEY_SIZE) {
            testCipher(bench, aes256, tvi, tv_count, tv->key, tv->key_size, tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
        }
    }

    bench.report(u"CryptoTest::testAES");
}

void CryptoTest::testAES_ECB()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_AES_ECB_ITERATIONS");

    ts::ECB<ts::AES128> aes128;
    ts::ECB<ts::AES256> aes256;

    const size_t tv_count = sizeof(tv_ecb_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_ecb_aes + tvi;
        if (tv->key_size == ts::AES128::KEY_SIZE) {
            testChaining(bench, aes128, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
        else if (tv->key_size == ts::AES256::KEY_SIZE) {
            testChaining(bench, aes256, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
    }

    bench.report(u"CryptoTest::testAES_ECB");

    testChainingSizes(aes128, 16, 32, 64, 65536, 0);
    testChainingSizes(aes256, 16, 32, 64, 65536, 0);
}

void CryptoTest::testAES_CBC()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_AES_CBC_ITERATIONS");

    ts::CBC<ts::AES128> aes128;
    ts::CBC<ts::AES256> aes256;

    const size_t tv_count = sizeof(tv_cbc_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_cbc_aes + tvi;
        if (tv->key_size == ts::AES128::KEY_SIZE) {
            testChaining(bench, aes128, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
        else if (tv->key_size == ts::AES256::KEY_SIZE) {
            testChaining(bench, aes256, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
    }

    bench.report(u"CryptoTest::testAES_CBC");

    testChainingSizes(aes128, 16, 32, 64, 65536, 0);
    testChainingSizes(aes256, 16, 32, 64, 65536, 0);
}

void CryptoTest::testAES_CTR()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_AES_CTR_ITERATIONS");

    ts::CTR<ts::AES128> aes128;
    ts::CTR<ts::AES256> aes256;

    const size_t tv_count = sizeof(tv_ctr_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_ctr_aes + tvi;
        if (tv->key_size == ts::AES128::KEY_SIZE) {
            testChaining(bench, aes128, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
        else if (tv->key_size == ts::AES256::KEY_SIZE) {
            testChaining(bench, aes256, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
    }

    bench.report(u"CryptoTest::testAES_CTR");

    testChainingSizes(aes128, 16, 32, 64, 65536, 0);
    testChainingSizes(aes256, 16, 32, 64, 65536, 0);
}

void CryptoTest::testAES_CTS1()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_AES_CTS1_ITERATIONS");

    ts::CTS1<ts::AES128> aes128;
    ts::CTS1<ts::AES256> aes256;

    const size_t tv_count = sizeof(tv_cts_aes) / sizeof(TV_AES_CHAIN);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_AES_CHAIN* tv = tv_cts_aes + tvi;
        if (tv->key_size == ts::AES128::KEY_SIZE) {
            testChaining(bench, aes128, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
        else if (tv->key_size == ts::AES256::KEY_SIZE) {
            testChaining(bench, aes256, tvi, tv_count, tv->key, tv->key_size, tv->iv, tv->iv_size, tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
        }
    }

    bench.report(u"CryptoTest::testAES_CTS1");

    // With CTS1, message size must be greater than block size.
    testChainingSizes(aes128, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
    testChainingSizes(aes256, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_CTS2()
{
    ts::CTS2<ts::AES128> aes128;
    ts::CTS2<ts::AES256> aes256;
    testChainingSizes(aes128, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
    testChainingSizes(aes256, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_CTS3()
{
    ts::CTS3<ts::AES128> aes128;
    ts::CTS3<ts::AES256> aes256;
    testChainingSizes(aes128, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
    testChainingSizes(aes256, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_CTS4()
{
    ts::CTS4<ts::AES128> aes128;
    ts::CTS4<ts::AES256> aes256;
    testChainingSizes(aes128, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
    testChainingSizes(aes256, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testAES_DVS042()
{
    ts::DVS042<ts::AES128> aes128;
    ts::DVS042<ts::AES256> aes256;
    testChainingSizes(aes128, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
    testChainingSizes(aes256, 16, 17, 23, 31, 32, 33, 45, 64, 67, 184, 12345, 0);
}

void CryptoTest::testDES()
{
    ts::DES des;

    TSUNIT_ASSERT(des.blockSize()  == 8);
    TSUNIT_ASSERT(des.minKeySize() == 8);
    TSUNIT_ASSERT(des.maxKeySize() == 8);
    TSUNIT_ASSERT(des.isValidKeySize(8));
    TSUNIT_ASSERT(!des.isValidKeySize(0));
    TSUNIT_ASSERT(!des.isValidKeySize(16));

    utest::TSUnitBenchmark bench(u"TSUNIT_DES_ITERATIONS");

    const size_t tv_count = sizeof(tv_des) / sizeof(TV_DES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_DES* tv = tv_des + tvi;
        testCipher(bench, des, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
    }

    bench.report(u"CryptoTest::testDES");
}

void CryptoTest::testTDES()
{
    ts::TDES tdes;

    TSUNIT_ASSERT(tdes.blockSize()  == 8);
    TSUNIT_ASSERT(tdes.minKeySize() == 24);
    TSUNIT_ASSERT(tdes.maxKeySize() == 24);
    TSUNIT_ASSERT(tdes.isValidKeySize(24));
    TSUNIT_ASSERT(!tdes.isValidKeySize(0));
    TSUNIT_ASSERT(!tdes.isValidKeySize(8));
    TSUNIT_ASSERT(!tdes.isValidKeySize(16));

    utest::TSUnitBenchmark bench(u"TSUNIT_TDES_ITERATIONS");

    const size_t tv_count = sizeof(tv_tdes) / sizeof(TV_TDES);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_TDES* tv = tv_tdes + tvi;
        testCipher(bench, tdes, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, sizeof(tv->plain), tv->cipher, sizeof(tv->cipher));
    }

    bench.report(u"CryptoTest::testTDES");
}

void CryptoTest::testTDES_CBC()
{
    ts::CBC<ts::TDES> cbc_tdes;

    TSUNIT_ASSERT(cbc_tdes.blockSize()  == 8);
    TSUNIT_ASSERT(cbc_tdes.minKeySize() == 24);
    TSUNIT_ASSERT(cbc_tdes.maxKeySize() == 24);
    TSUNIT_ASSERT(cbc_tdes.isValidKeySize(24));
    TSUNIT_ASSERT(!cbc_tdes.isValidKeySize(0));
    TSUNIT_ASSERT(!cbc_tdes.isValidKeySize(8));
    TSUNIT_ASSERT(!cbc_tdes.isValidKeySize(16));

    utest::TSUnitBenchmark bench(u"TSUNIT_TDES_CBC_ITERATIONS");

    const size_t tv_count = sizeof(tv_tdes_cbc) / sizeof(TV_TDES_CBC);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_TDES_CBC* tv = tv_tdes_cbc + tvi;
        testChaining(bench, cbc_tdes, tvi, tv_count, tv->key, sizeof(tv->key), tv->iv, sizeof(tv->iv), tv->plain, tv->size, tv->cipher, tv->size);
    }

    bench.report(u"CryptoTest::testTDES_CBC");
}

void CryptoTest::testDVBCSA2()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_DVBCSA2_ITERATIONS");

    ts::DVBCSA2 csa;
    const size_t tv_count = sizeof(tv_dvb_csa2) / sizeof(tv_dvb_csa2[0]);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_DVB_CSA2* tv = tv_dvb_csa2 + tvi;
        testCipher(bench, csa, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, tv->size, tv->cipher, tv->size);
    }

    bench.report(u"CryptoTest::testDVBCSA2");
}

void CryptoTest::testDVBCISSA()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_DVBCISSA_ITERATIONS");

    ts::DVBCISSA cissa;
    const size_t tv_count = sizeof(tv_dvb_cissa) / sizeof(tv_dvb_cissa[0]);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_DVB_CISSA* tv = tv_dvb_cissa + tvi;
        const size_t hsize = tv->plain.getHeaderSize();
        const size_t psize = tv->plain.getPayloadSize();
        const size_t size = psize - psize % cissa.blockSize();
        TSUNIT_EQUAL(hsize, tv->cipher.getHeaderSize());
        TSUNIT_EQUAL(psize, tv->cipher.getPayloadSize());
        TSUNIT_EQUAL(16, cissa.blockSize());
        testCipher(bench, cissa, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain.b + hsize, size, tv->cipher.b + hsize, size);
    }

    bench.report(u"CryptoTest::testDVBCISSA");
}

void CryptoTest::testIDSA()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_IDSA_ITERATIONS");

    ts::IDSA idsa;
    const size_t tv_count = sizeof(tv_atis_idsa) / sizeof(tv_atis_idsa[0]);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_ATIS_IDSA* tv = tv_atis_idsa + tvi;
        testCipher(bench, idsa, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, tv->size, tv->cipher, tv->size);
    }

    bench.report(u"CryptoTest::testIDSA");
}

void CryptoTest::testSCTE52_2003()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_SCTE52_2003_ITERATIONS");

    ts::SCTE52_2003 scte;
    const size_t tv_count = sizeof(tv_scte52_2003) / sizeof(tv_scte52_2003[0]);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SCTE52_2003* tv = tv_scte52_2003 + tvi;
        testChaining(bench, scte, tvi, tv_count, tv->key, sizeof(tv->key), tv->iv, sizeof(tv->iv), tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }

    bench.report(u"CryptoTest::testSCTE52_2003");
}

void CryptoTest::testSCTE52_2008()
{
    utest::TSUnitBenchmark bench(u"TSUNIT_SCTE52_2008_ITERATIONS");

    ts::SCTE52_2008 scte;
    const size_t tv_count = sizeof(tv_scte52_2008) / sizeof(tv_scte52_2008[0]);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SCTE52_2008* tv = tv_scte52_2008 + tvi;
        TSUNIT_ASSERT(scte.setIV(tv->iv, sizeof(tv->iv)));
        TSUNIT_ASSERT(scte.setShortIV(tv->short_iv, sizeof(tv->short_iv)));
        testCipher(bench, scte, tvi, tv_count, tv->key, sizeof(tv->key), tv->plain, tv->plain_size, tv->cipher, tv->cipher_size);
    }

    bench.report(u"CryptoTest::testSCTE52_2008");
}

void CryptoTest::testSHA1()
{
    ts::SHA1 sha1;
    TSUNIT_ASSERT(sha1.hashSize() == 20);

    utest::TSUnitBenchmark bench(u"TSUNIT_SHA1_ITERATIONS");

    const size_t tv_count = sizeof(tv_sha1) / sizeof(TV_SHA1);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA1* tv = tv_sha1 + tvi;
        testHash(bench, sha1, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }

    bench.report(u"CryptoTest::testSHA1");
}

void CryptoTest::testSHA256()
{
    ts::SHA256 sha256;
    TSUNIT_ASSERT(sha256.hashSize() == 32);

    utest::TSUnitBenchmark bench(u"TSUNIT_SHA256_ITERATIONS");

    const size_t tv_count = sizeof(tv_sha256) / sizeof(TV_SHA256);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA256* tv = tv_sha256 + tvi;
        testHash(bench, sha256, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }

    bench.report(u"CryptoTest::testSHA256");
}

void CryptoTest::testSHA512()
{
    ts::SHA512 sha512;
    TSUNIT_ASSERT(sha512.hashSize() == 64);

    utest::TSUnitBenchmark bench(u"TSUNIT_SHA512_ITERATIONS");

    const size_t tv_count = sizeof(tv_sha512) / sizeof(TV_SHA512);
    for (size_t tvi = 0; tvi < tv_count; ++tvi) {
        const TV_SHA512* tv = tv_sha512 + tvi;
        testHash(bench, sha512, tvi, tv_count, tv->message, tv->hash, sizeof(tv->hash));
    }

    bench.report(u"CryptoTest::testSHA512");
}
