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
//
//  TSUnit test suite for utilities in class ts::Buffer
//
//----------------------------------------------------------------------------

#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsunit.h"

// Some floating-point literal (implicitly double) are used as ieee_float32_t
TS_LLVM_NOWARNING(implicit-float-conversion)
TS_MSC_NOWARNING(4056) // overflow in floating-point constant arithmetic
TS_MSC_NOWARNING(4305) // truncation from 'double' to 'ts::ieee_float32_t'


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class BufferTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testConstructors();
    void testReset();
    void testResize();
    void testSeek();
    void testByteAligned();
    void testSkipBack();
    void testReadBitBigEndian();
    void testReadBitLittleEndian();
    void testReadBitsBigEndian();
    void testReadBitsLittleEndian();
    void testGetUInt8();
    void testGetUInt16BE();
    void testGetUInt16LE();
    void testGetUInt24BE();
    void testGetUInt24LE();
    void testGetUInt32BE();
    void testGetUInt32LE();
    void testGetUInt40BE();
    void testGetUInt40LE();
    void testGetUInt48BE();
    void testGetUInt48LE();
    void testGetUInt64BE();
    void testGetUInt64LE();
    void testGetInt8();
    void testGetInt16BE();
    void testGetInt16LE();
    void testGetInt24BE();
    void testGetInt24LE();
    void testGetInt32BE();
    void testGetInt32LE();
    void testGetInt40BE();
    void testGetInt40LE();
    void testGetInt48BE();
    void testGetInt48LE();
    void testGetInt64BE();
    void testGetInt64LE();
    void testGetBitsSigned();
    void testPutBCD();
    void testGetBCD();
    void testTryGetASCII();
    void testGetUTF8();
    void testGetUTF8WithLength();
    void testGetUTF16();
    void testGetUTF16WithLength();
    void testPutUTF8();
    void testPutFixedUTF8();
    void testPutPartialUTF8();
    void testPutUTF8WithLength();
    void testPutPartialUTF8WithLength();
    void testPutUTF16();
    void testPutFixedUTF16();
    void testPutPartialUTF16();
    void testPutUTF16WithLength();
    void testPutPartialUTF16WithLength();
    void testGetFloat32LE();
    void testGetFloat32BE();
    void testGetFloat64LE();
    void testGetFloat64BE();
    void testPutFloat32LE();
    void testPutFloat32BE();
    void testPutFloat64LE();
    void testPutFloat64BE();
    void testGetVluimsbf5();
    void testPutVluimsbf5();

    TSUNIT_TEST_BEGIN(BufferTest);
    TSUNIT_TEST(testConstructors);
    TSUNIT_TEST(testReset);
    TSUNIT_TEST(testResize);
    TSUNIT_TEST(testSeek);
    TSUNIT_TEST(testByteAligned);
    TSUNIT_TEST(testSkipBack);
    TSUNIT_TEST(testReadBitBigEndian);
    TSUNIT_TEST(testReadBitLittleEndian);
    TSUNIT_TEST(testReadBitsBigEndian);
    TSUNIT_TEST(testReadBitLittleEndian);
    TSUNIT_TEST(testGetUInt8);
    TSUNIT_TEST(testGetUInt16BE);
    TSUNIT_TEST(testGetUInt16LE);
    TSUNIT_TEST(testGetUInt24BE);
    TSUNIT_TEST(testGetUInt24LE);
    TSUNIT_TEST(testGetUInt32BE);
    TSUNIT_TEST(testGetUInt32LE);
    TSUNIT_TEST(testGetUInt40BE);
    TSUNIT_TEST(testGetUInt40LE);
    TSUNIT_TEST(testGetUInt48BE);
    TSUNIT_TEST(testGetUInt48LE);
    TSUNIT_TEST(testGetUInt64BE);
    TSUNIT_TEST(testGetUInt64LE);
    TSUNIT_TEST(testGetInt8);
    TSUNIT_TEST(testGetInt16BE);
    TSUNIT_TEST(testGetInt16LE);
    TSUNIT_TEST(testGetInt24BE);
    TSUNIT_TEST(testGetInt24LE);
    TSUNIT_TEST(testGetInt32BE);
    TSUNIT_TEST(testGetInt32LE);
    TSUNIT_TEST(testGetInt40BE);
    TSUNIT_TEST(testGetInt40LE);
    TSUNIT_TEST(testGetInt48BE);
    TSUNIT_TEST(testGetInt48LE);
    TSUNIT_TEST(testGetInt64BE);
    TSUNIT_TEST(testGetInt64LE);
    TSUNIT_TEST(testGetBitsSigned);
    TSUNIT_TEST(testPutBCD);
    TSUNIT_TEST(testGetBCD);
    TSUNIT_TEST(testTryGetASCII);
    TSUNIT_TEST(testGetUTF8);
    TSUNIT_TEST(testGetUTF8WithLength);
    TSUNIT_TEST(testGetUTF16);
    TSUNIT_TEST(testGetUTF16WithLength);
    TSUNIT_TEST(testPutUTF8);
    TSUNIT_TEST(testPutFixedUTF8);
    TSUNIT_TEST(testPutPartialUTF8);
    TSUNIT_TEST(testPutUTF8WithLength);
    TSUNIT_TEST(testPutPartialUTF8WithLength);
    TSUNIT_TEST(testPutUTF16);
    TSUNIT_TEST(testPutFixedUTF16);
    TSUNIT_TEST(testPutPartialUTF16);
    TSUNIT_TEST(testPutUTF16WithLength);
    TSUNIT_TEST(testPutPartialUTF16WithLength);
    TSUNIT_TEST(testGetFloat32LE);
    TSUNIT_TEST(testGetFloat32BE);
    TSUNIT_TEST(testGetFloat64LE);
    TSUNIT_TEST(testGetFloat64BE);
    TSUNIT_TEST(testPutFloat32LE);
    TSUNIT_TEST(testPutFloat32BE);
    TSUNIT_TEST(testPutFloat64LE);
    TSUNIT_TEST(testPutFloat64BE);
    TSUNIT_TEST(testGetVluimsbf5);
    TSUNIT_TEST(testPutVluimsbf5);
    TSUNIT_TEST_END();

private:
    // Return a byte block with bytes swapped two by two.
    static ts::ByteBlock SwapBytes(const ts::UString& str);
    static ts::ByteBlock SwapBytes(const void* data, size_t size);
};

TSUNIT_REGISTER(BufferTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void BufferTest::beforeTest()
{
}

// Test suite cleanup method.
void BufferTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Reference byte array
//----------------------------------------------------------------------------

namespace {
    // Reference byte array: 256 bytes, index == value
    const uint8_t _bytes1[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    };

    const uint8_t _bytes2[16] = {
        0x50, // 01010000
        0x51, // 01010001
        0x52, // 01010010
        0x53, // 01010011
        0x54, // 01010100
        0x55, // 01010101
        0x56, // 01010110
        0x57, // 01010111
        0x68, // 01101000
        0x69, // 01101001
        0x6A, // 01101010
        0x6B, // 01101011
        0x6C, // 01101100
        0x6D, // 01101101
        0x6E, // 01101110
        0x6F, // 01101111
    };
}


//----------------------------------------------------------------------------
// Return a byte block with bytes swapped two by two.
//----------------------------------------------------------------------------

ts::ByteBlock BufferTest::SwapBytes(const void* data, size_t size)
{
    ts::ByteBlock result(size);
    const uint8_t* const in = reinterpret_cast<const uint8_t*>(data);

    for (size_t i = 0; i+1 < result.size(); i += 2) {
        result[i] = in[i + 1];
        result[i + 1] = in[i];
    }
    return result;
}

ts::ByteBlock BufferTest::SwapBytes(const ts::UString& str)
{
    return SwapBytes(str.data(), 2 * str.size());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void BufferTest::testConstructors()
{
    ts::Buffer b1;
    TSUNIT_ASSERT(b1.isValid());
    TSUNIT_ASSERT(!b1.readOnly());
    TSUNIT_ASSERT(b1.data() != nullptr);
    TSUNIT_ASSERT(b1.internalMemory());
    TSUNIT_ASSERT(!b1.externalMemory());
    TSUNIT_EQUAL(ts::Buffer::DEFAULT_SIZE, b1.capacity());
    TSUNIT_EQUAL(ts::Buffer::DEFAULT_SIZE, b1.size());

    uint8_t bin2[256];
    ts::Buffer b2(bin2, sizeof(bin2));
    TSUNIT_ASSERT(b2.isValid());
    TSUNIT_ASSERT(!b2.readOnly());
    TSUNIT_ASSERT(!b2.internalMemory());
    TSUNIT_ASSERT(b2.externalMemory());
    TSUNIT_EQUAL(bin2, b2.data());
    TSUNIT_EQUAL(sizeof(bin2), b2.capacity());
    TSUNIT_EQUAL(sizeof(bin2), b2.size());
    TSUNIT_EQUAL(0, b2.currentReadBitOffset());
    TSUNIT_EQUAL(0, b2.currentReadByteOffset());
    TSUNIT_EQUAL(0, b2.currentWriteBitOffset());
    TSUNIT_EQUAL(0, b2.currentWriteByteOffset());

    static const uint8_t bin3[128] = {0};
    ts::Buffer b3(bin3, sizeof(bin3));
    TSUNIT_ASSERT(b3.isValid());
    TSUNIT_ASSERT(b3.readOnly());
    TSUNIT_ASSERT(!b3.internalMemory());
    TSUNIT_ASSERT(b3.externalMemory());
    TSUNIT_EQUAL(bin3, b3.data());
    TSUNIT_EQUAL(sizeof(bin3), b3.capacity());
    TSUNIT_EQUAL(sizeof(bin3), b3.size());
    TSUNIT_EQUAL(0, b3.currentReadBitOffset());
    TSUNIT_EQUAL(0, b3.currentReadByteOffset());
    TSUNIT_EQUAL(sizeof(bin3) * 8, b3.currentWriteBitOffset());
    TSUNIT_EQUAL(sizeof(bin3), b3.currentWriteByteOffset());

    b1.putBits(0, 11);
    TSUNIT_EQUAL(11, b1.currentWriteBitOffset());
    TSUNIT_EQUAL(1, b1.currentWriteByteOffset());

    ts::Buffer b4(b1);
    TSUNIT_ASSERT(b4.isValid());
    TSUNIT_ASSERT(!b4.readOnly());
    TSUNIT_ASSERT(b4.data() != nullptr);
    TSUNIT_ASSERT(b4.data() != b1.data());
    TSUNIT_ASSERT(b4.internalMemory());
    TSUNIT_ASSERT(!b4.externalMemory());
    TSUNIT_EQUAL(ts::Buffer::DEFAULT_SIZE, b4.capacity());
    TSUNIT_EQUAL(ts::Buffer::DEFAULT_SIZE, b4.size());
    TSUNIT_EQUAL(11, b4.currentWriteBitOffset());
    TSUNIT_EQUAL(1, b4.currentWriteByteOffset());

    ts::Buffer b5(b3);
    TSUNIT_ASSERT(b5.isValid());
    TSUNIT_ASSERT(b5.readOnly());
    TSUNIT_ASSERT(!b5.internalMemory());
    TSUNIT_ASSERT(b5.externalMemory());
    TSUNIT_EQUAL(bin3, b5.data());
    TSUNIT_EQUAL(sizeof(bin3), b5.capacity());
    TSUNIT_EQUAL(sizeof(bin3), b5.size());
    TSUNIT_EQUAL(0, b5.currentReadBitOffset());
    TSUNIT_EQUAL(0, b5.currentReadByteOffset());
    TSUNIT_EQUAL(sizeof(bin3) * 8, b5.currentWriteBitOffset());
    TSUNIT_EQUAL(sizeof(bin3), b5.currentWriteByteOffset());
}

void BufferTest::testReset()
{
    ts::Buffer b(512);

    TSUNIT_ASSERT(b.isValid());
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_ASSERT(b.data() != nullptr);
    TSUNIT_ASSERT(b.internalMemory());
    TSUNIT_ASSERT(!b.externalMemory());
    TSUNIT_EQUAL(512, b.capacity());
    TSUNIT_EQUAL(512, b.size());

    static const uint8_t bin[128] = {0};
    b.reset(bin, sizeof(bin));

    TSUNIT_ASSERT(b.isValid());
    TSUNIT_ASSERT(b.readOnly());
    TSUNIT_EQUAL(bin, b.data());
    TSUNIT_ASSERT(!b.internalMemory());
    TSUNIT_ASSERT(b.externalMemory());
    TSUNIT_EQUAL(128, b.capacity());
    TSUNIT_EQUAL(128, b.size());
}

void BufferTest::testResize()
{
    ts::Buffer b(512);

    TSUNIT_ASSERT(b.isValid());
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_ASSERT(b.data() != nullptr);
    TSUNIT_ASSERT(b.internalMemory());
    TSUNIT_ASSERT(!b.externalMemory());
    TSUNIT_EQUAL(512, b.capacity());
    TSUNIT_EQUAL(512, b.size());

    TSUNIT_ASSERT(b.resize(256, false));
    TSUNIT_EQUAL(512, b.capacity());
    TSUNIT_EQUAL(256, b.size());

    TSUNIT_ASSERT(!b.resize(600, false));
    TSUNIT_EQUAL(512, b.capacity());
    TSUNIT_EQUAL(512, b.size());

    TSUNIT_ASSERT(b.resize(600, true));
    TSUNIT_EQUAL(600, b.capacity());
    TSUNIT_EQUAL(600, b.size());

    TSUNIT_ASSERT(b.resize(4, false));
    TSUNIT_EQUAL(600, b.capacity());
    TSUNIT_EQUAL(4, b.size());

    TSUNIT_ASSERT(b.resize(4, true));
    TSUNIT_EQUAL(16, b.capacity());
    TSUNIT_EQUAL(4, b.size());
}

void BufferTest::testSeek()
{
    // 0          10         20         30          40         50         60         70        79
    // |          |          |          |           |          |          |          |         |
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001

    ts::Buffer b(_bytes2, 10);

    TSUNIT_ASSERT(b.externalMemory());
    TSUNIT_EQUAL(0, b.currentReadBitOffset());
    TSUNIT_EQUAL(80, b.remainingReadBits());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_ASSERT(b.readSeek(2, 3));
    TSUNIT_EQUAL(19, b.currentReadBitOffset());
    TSUNIT_EQUAL(61, b.remainingReadBits());

    TSUNIT_EQUAL(4, b.getBits<uint8_t>(3));
    TSUNIT_EQUAL(22, b.currentReadBitOffset());
    TSUNIT_EQUAL(58, b.remainingReadBits());

    TSUNIT_EQUAL(0x94D5, b.getUInt16()); // 10 01010011 010101
    TSUNIT_EQUAL(38, b.currentReadBitOffset());
    TSUNIT_EQUAL(42, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_ASSERT(b.readSeek(9));
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(72, b.currentReadBitOffset());
    TSUNIT_EQUAL(8, b.remainingReadBits());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(0x69, b.getUInt8());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(80, b.currentReadBitOffset());
    TSUNIT_EQUAL(0, b.remainingReadBits());
    TSUNIT_ASSERT(b.endOfRead());
    TSUNIT_ASSERT(!b.readError());

    TSUNIT_EQUAL(0xFFFF, b.getUInt16());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(80, b.currentReadBitOffset());
    TSUNIT_EQUAL(0, b.remainingReadBits());
    TSUNIT_ASSERT(b.endOfRead());
    TSUNIT_ASSERT(b.readError());

    TSUNIT_ASSERT(b.readSeek(8));
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(64, b.currentReadBitOffset());
    TSUNIT_EQUAL(16, b.remainingReadBits());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_ASSERT(b.readError());
    b.clearReadError();
    TSUNIT_ASSERT(!b.readError());

    TSUNIT_EQUAL(0x68, b.getUInt8());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(72, b.currentReadBitOffset());
    TSUNIT_EQUAL(8, b.remainingReadBits());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_ASSERT(!b.readError());
}

void BufferTest::testByteAligned()
{
    ts::Buffer b(_bytes2, 10);

    TSUNIT_ASSERT(b.readIsByteAligned());

    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    b.readSeek(2,7);
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.readIsByteAligned());
}

void BufferTest::testSkipBack()
{
    ts::Buffer b(_bytes2, 10);
    TSUNIT_ASSERT(b.readIsByteAligned());

    TSUNIT_ASSERT(b.skipBits(3));
    TSUNIT_EQUAL(3, b.currentReadBitOffset());
    TSUNIT_EQUAL(77, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_EQUAL(0x82, b.getUInt8()); // 10000 010
    TSUNIT_EQUAL(11, b.currentReadBitOffset());
    TSUNIT_EQUAL(69, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_ASSERT(b.skipBits(5));
    TSUNIT_EQUAL(16, b.currentReadBitOffset());
    TSUNIT_EQUAL(64, b.remainingReadBits());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(2, b.currentReadByteOffset());
    TSUNIT_EQUAL(8, b.remainingReadBytes());

    TSUNIT_ASSERT(b.skipBits(12));
    TSUNIT_EQUAL(28, b.currentReadBitOffset());
    TSUNIT_EQUAL(52, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_ASSERT(b.skipBytes(2));
    TSUNIT_EQUAL(40, b.currentReadBitOffset());
    TSUNIT_EQUAL(40, b.remainingReadBits());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(5, b.currentReadByteOffset());
    TSUNIT_EQUAL(5, b.remainingReadBytes());

    TSUNIT_ASSERT(b.backBits(3));
    TSUNIT_EQUAL(37, b.currentReadBitOffset());
    TSUNIT_EQUAL(43, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_EQUAL(0x02, b.getBits<uint8_t>(2));
    TSUNIT_EQUAL(39, b.currentReadBitOffset());
    TSUNIT_EQUAL(41, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_ASSERT(b.backBytes(3));
    TSUNIT_EQUAL(8, b.currentReadBitOffset());
    TSUNIT_EQUAL(72, b.remainingReadBits());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.currentReadByteOffset());
    TSUNIT_EQUAL(9, b.remainingReadBytes());

    TSUNIT_ASSERT(b.readRealignByte());
    TSUNIT_EQUAL(8, b.currentReadBitOffset());
    TSUNIT_EQUAL(72, b.remainingReadBits());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.currentReadByteOffset());
    TSUNIT_EQUAL(9, b.remainingReadBytes());

    TSUNIT_ASSERT(b.skipBits(3));
    TSUNIT_EQUAL(11, b.currentReadBitOffset());
    TSUNIT_EQUAL(69, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readIsByteAligned());

    TSUNIT_ASSERT(b.readRealignByte());
    TSUNIT_EQUAL(16, b.currentReadBitOffset());
    TSUNIT_EQUAL(64, b.remainingReadBits());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(2, b.currentReadByteOffset());
    TSUNIT_EQUAL(8, b.remainingReadBytes());
}

void BufferTest::testReadBitBigEndian()
{
    // 0          10         20         30          40         50         60         70          80     87
    // |          |          |          |           |          |          |          |           |      |
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010

    ts::Buffer b(_bytes2, 11);

    TSUNIT_ASSERT(b.isValid());
    TSUNIT_ASSERT(b.readOnly());
    TSUNIT_ASSERT(!b.internalMemory());
    TSUNIT_ASSERT(b.externalMemory());
    TSUNIT_EQUAL(11, b.capacity());
    TSUNIT_EQUAL(11, b.size());
    TSUNIT_ASSERT(b.isBigEndian());
    TSUNIT_ASSERT(!b.isLittleEndian());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(0, b.currentReadByteOffset());
    TSUNIT_EQUAL(0, b.currentReadBitOffset());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL(88, b.currentWriteBitOffset());
    TSUNIT_EQUAL(11, b.remainingReadBytes());
    TSUNIT_EQUAL(88, b.remainingReadBits());
    TSUNIT_EQUAL(0, b.remainingWriteBytes());
    TSUNIT_EQUAL(0, b.remainingWriteBits());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_ASSERT(b.endOfWrite());

    // Offset 0
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.getBit());

    // Offset 10
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());

    // Offset 20
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 30
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 40
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());

    // Offset 50
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());

    // Offset 60
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 70
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(9, b.currentReadByteOffset());
    TSUNIT_EQUAL(79, b.currentReadBitOffset());
    TSUNIT_EQUAL(1, b.getBit());

    // Offset 80
    TSUNIT_EQUAL(10, b.currentReadByteOffset());
    TSUNIT_EQUAL(80, b.currentReadBitOffset());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());
    TSUNIT_EQUAL(81, b.currentReadBitOffset());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.endOfRead());

    // End of stream
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.error());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.error());
    TSUNIT_EQUAL(0, b.getBit());
}

void BufferTest::testReadBitLittleEndian()
{
    //        0        8       16       24       32       40       48       56       64       72 87    80
    //      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-| |    <-|
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010

    ts::Buffer b(_bytes2, 11);
    b.setLittleEndian();

    TSUNIT_ASSERT(b.isValid());
    TSUNIT_ASSERT(b.readOnly());
    TSUNIT_ASSERT(!b.internalMemory());
    TSUNIT_ASSERT(b.externalMemory());
    TSUNIT_EQUAL(11, b.capacity());
    TSUNIT_EQUAL(11, b.size());
    TSUNIT_ASSERT(!b.isBigEndian());
    TSUNIT_ASSERT(b.isLittleEndian());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(0, b.currentReadByteOffset());
    TSUNIT_EQUAL(0, b.currentReadBitOffset());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL(88, b.currentWriteBitOffset());
    TSUNIT_EQUAL(11, b.remainingReadBytes());
    TSUNIT_EQUAL(88, b.remainingReadBits());
    TSUNIT_EQUAL(0, b.remainingWriteBytes());
    TSUNIT_EQUAL(0, b.remainingWriteBits());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_ASSERT(b.endOfWrite());

    // Offset 0
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(7, b.currentReadBitOffset());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(8, b.currentReadBitOffset());

    // Offset 8
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 16
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 24
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 32
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 40
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 48
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 56
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 64
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 72
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());

    // Offset 80
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(1, b.getBit());
    TSUNIT_EQUAL(87, b.currentReadBitOffset());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_EQUAL(88, b.currentReadBitOffset());
    TSUNIT_ASSERT(b.endOfRead());

    // End of stream
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.error());
    TSUNIT_EQUAL(0, b.getBit());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.error());
    TSUNIT_EQUAL(0, b.getBit());
}

void BufferTest::testReadBitsBigEndian()
{
    // 0          10         20         30          40         50         60         70          80         90
    // |          |          |          |           |          |          |          |           |          |
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010 01101011
    ts::Buffer b(_bytes2, 12);

    // 0101000
    TSUNIT_EQUAL(0x28, b.getBits<int>(7));
    TSUNIT_EQUAL(7, b.currentReadBitOffset());

    // 00101
    TSUNIT_EQUAL(0x05, b.getBits<int>(5));
    TSUNIT_EQUAL(12, b.currentReadBitOffset());

    // 000101010010010
    TSUNIT_EQUAL(0xA92, b.getBits<uint16_t>(15));
    TSUNIT_EQUAL(27, b.currentReadBitOffset());

    // Important: Due to a code generation bug in Microsoft Visual C++ 2010,
    // the default value (second parameter of Buffer::readBits)
    // must be explicitly provided when instantiated on 64-bit integer types.

    // 100110101010001010101010101100101011101101000011010010110101
    TSUNIT_EQUAL(TS_UCONST64(0x9AA2AAB2BB434B5), b.getBits<uint64_t>(60));
    TSUNIT_EQUAL(87, b.currentReadBitOffset());

    // 001101011 (9 remaining bits)
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(0, b.getBits<int32_t>(10)); // after eof
    TSUNIT_EQUAL(87, b.currentReadBitOffset());
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.endOfRead());
    b.clearError();
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(0x6B, b.getBits<int32_t>(9));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(b.endOfRead());
    TSUNIT_EQUAL(96, b.currentReadBitOffset());
}

void BufferTest::testReadBitsLittleEndian()
{
    //        0        8       16       24       32       40       48       56       64       72       80       88
    //      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|      <-|
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010 01101011
    ts::Buffer b(_bytes2, 12);
    b.setLittleEndian();

    // 1010000
    TSUNIT_EQUAL(0x50, b.getBits<int>(7));
    TSUNIT_EQUAL(7, b.currentReadBitOffset());

    // 0 0001 -> 0001 0
    TSUNIT_EQUAL(0x02, b.getBits<int>(5));
    TSUNIT_EQUAL(12, b.currentReadBitOffset());

    // 0101 01010010 010 -> 010 01010010 0101
    TSUNIT_EQUAL(0x2525, b.getBits<uint16_t>(15));
    TSUNIT_EQUAL(27, b.currentReadBitOffset());
}

void BufferTest::testGetUInt8()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x07);
    TSUNIT_EQUAL(0x07, b.getUInt8());
}

void BufferTest::testGetUInt16BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x23);
    TSUNIT_EQUAL(0x2324, b.getUInt16());
}

void BufferTest::testGetUInt16LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x23);
    TSUNIT_EQUAL(0x2423, b.getUInt16());
}

void BufferTest::testGetUInt24BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x10);
    TSUNIT_EQUAL(0x101112, b.getUInt24());
    b.readSeek(0xCE);
    TSUNIT_EQUAL(0xCECFD0, b.getUInt24());
}

void BufferTest::testGetUInt24LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x10);
    TSUNIT_EQUAL(0x121110, b.getUInt24());
    b.readSeek(0xCE);
    TSUNIT_EQUAL(0xD0CFCE, b.getUInt24());
}

void BufferTest::testGetUInt32BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x47);
    TSUNIT_EQUAL(0x4748494A, b.getUInt32());
}

void BufferTest::testGetUInt32LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x47);
    TSUNIT_EQUAL(0x4A494847, b.getUInt32());
}

void BufferTest::testGetUInt40BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x000000898A8B8C8D), b.getUInt40());
}

void BufferTest::testGetUInt40LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x0000008D8C8B8A89), b.getUInt40());
}

void BufferTest::testGetUInt48BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x0000898A8B8C8D8E), b.getUInt48());
}

void BufferTest::testGetUInt48LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x00008E8D8C8B8A89), b.getUInt48());
}

void BufferTest::testGetUInt64BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x898A8B8C8D8E8F90), b.getUInt64());
}

void BufferTest::testGetUInt64LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x89);
    TSUNIT_EQUAL(TS_UCONST64(0x908F8E8D8C8B8A89), b.getUInt64());
}

void BufferTest::testGetInt8()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x03);
    TSUNIT_EQUAL(3, b.getInt8());
}

void BufferTest::testGetInt16BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0xCE);
    TSUNIT_EQUAL(-12593, b.getInt16()); // 0xCECF
}

void BufferTest::testGetInt16LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0xCE);
    TSUNIT_EQUAL(-12338, b.getInt16()); // 0xCFCE
}

void BufferTest::testGetInt24BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x10);
    TSUNIT_EQUAL(0x101112, b.getInt24());
    b.readSeek(0xCE);
    TSUNIT_EQUAL(-3223600, b.getInt24()); // 0xFFCECFD0
}

void BufferTest::testGetInt24LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x10);
    TSUNIT_EQUAL(0x121110, b.getInt24());
    b.readSeek(0xCE);
    TSUNIT_EQUAL(-3092530, b.getInt24()); // 0xFFD0CFCE
}

void BufferTest::testGetInt32BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0x81);
    TSUNIT_EQUAL(-2122153084, b.getInt32()); // 0x81828384
}

void BufferTest::testGetInt32LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0x81);
    TSUNIT_EQUAL(-2071756159, b.getInt32()); // 0x84838281
}

void BufferTest::testGetInt40BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-219885416496), b.getInt40()); // 0xCCCDCECFD0
}

void BufferTest::testGetInt40LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-202671993396), b.getInt40()); // 0xD0CFCECDCC
}

void BufferTest::testGetInt48BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-56290666622767), b.getInt48()); // 0xCCCDCECFD0D1
}

void BufferTest::testGetInt48LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-50780206871092), b.getInt48()); // 0xD1D0CFCECDCC
}

void BufferTest::testGetInt64BE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-3689065127789604141), b.getInt64()); // 0xCCCDCECFD0D1D2D3
}

void BufferTest::testGetInt64LE()
{
    ts::Buffer b(_bytes1, sizeof(_bytes1));
    b.setLittleEndian();
    b.readSeek(0xCC);
    TSUNIT_EQUAL(TS_CONST64(-3183251291827679796), b.getInt64()); // 0xD3D2D1D0CFCECDCC
}

void BufferTest::testGetBitsSigned()
{
    uint8_t mem[10];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));

    b.putBits(2, 3);
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());
    TSUNIT_EQUAL(3, b.currentWriteBitOffset());
    TSUNIT_EQUAL(0x40, mem[0]);

    b.putBits(-333, 11);  // -333 = 0xFEB3 = 110 1011 0011 (bin)
    TSUNIT_EQUAL(1, b.currentWriteByteOffset());
    TSUNIT_EQUAL(14, b.currentWriteBitOffset());
    TSUNIT_EQUAL(0x5A, mem[0]);  // 0101 1010 1100 11..
    TSUNIT_EQUAL(0xCC, mem[1]);

    b.putBits(-1, 2);
    TSUNIT_EQUAL(2, b.currentWriteByteOffset());
    TSUNIT_EQUAL(16, b.currentWriteBitOffset());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(0x5A, mem[0]);
    TSUNIT_EQUAL(0xCF, mem[1]);

    TSUNIT_EQUAL(2, b.getBits<int>(3));
    TSUNIT_EQUAL(-333, b.getBits<int>(11));
    TSUNIT_EQUAL(-1, b.getBits<int>(2));
}


void BufferTest::testPutBCD()
{
    uint8_t mem[10];
    ::memset(mem, 0xFF, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putBCD(45, 2));
    TSUNIT_EQUAL(0x45, mem[0]);
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(1, b.currentWriteByteOffset());
    TSUNIT_EQUAL(8, b.currentWriteBitOffset());

    TSUNIT_ASSERT(b.putBCD(912, 5));
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL(0x91, mem[2]);
    TSUNIT_EQUAL(0x2F, mem[3]);
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());
    TSUNIT_EQUAL(28, b.currentWriteBitOffset());

    TSUNIT_ASSERT(b.putBCD(358, 3));
    TSUNIT_EQUAL(0x23, mem[3]);
    TSUNIT_EQUAL(0x58, mem[4]);
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL(40, b.currentWriteBitOffset());
}

void BufferTest::testGetBCD()
{
    ts::Buffer b(_bytes1 + 0x25, 10);
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(25, b.getBCD<uint32_t>(2));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_EQUAL(1, b.currentReadByteOffset());
    TSUNIT_EQUAL(8, b.currentReadBitOffset());

    TSUNIT_EQUAL(26272, b.getBCD<uint32_t>(5));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(3, b.currentReadByteOffset());
    TSUNIT_EQUAL(28, b.currentReadBitOffset());

    TSUNIT_EQUAL(8, b.getBCD<int>(1));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(4, b.currentReadByteOffset());
    TSUNIT_EQUAL(32, b.currentReadBitOffset());

    TSUNIT_EQUAL(292000, b.getBCD<uint32_t>(6));
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(6, b.currentReadByteOffset());
    TSUNIT_EQUAL(48, b.currentReadBitOffset());
}

void BufferTest::testTryGetASCII()
{
    static const char mem[] = "abcdefgh\x00\x00\x00zyxw\x01\x02mnop";
    ts::Buffer b(mem, sizeof(mem) - 1); // exclude trailing zero => - 1
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(u"abcd", b.tryGetASCII(4));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(4, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(8));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(4, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"efgh", b.tryGetASCII(6));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(4));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(1));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(11, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(5));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(11, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"zyxw", b.tryGetASCII(4));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(15, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(6));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(15, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.tryGetASCII(7));
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(15, b.currentReadByteOffset());
}

void BufferTest::testGetUTF8()
{
    static const char mem[] = "abcdefgh";
    ts::Buffer b(mem, 8);
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(u"", b.getUTF8(0));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(0, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"ab", b.getUTF8(2));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(2, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"cde", b.getUTF8(3));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(5, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.getUTF8(10));
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(5, b.currentReadByteOffset());
}

void BufferTest::testGetUTF8WithLength()
{
    static const char mem[] = "\x03" "abc" "\xF0\x02" "de" "\x04" "fgh";
    TSUNIT_EQUAL(13, sizeof(mem));
    ts::Buffer b(mem, 12);
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(u"abc", b.getUTF8WithLength());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(4, b.currentReadByteOffset());

    TSUNIT_EQUAL(0x0F, b.getBits<uint8_t>(4));
    TSUNIT_EQUAL(u"de", b.getUTF8WithLength(12));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(8, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.getUTF8WithLength());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(8, b.currentReadByteOffset());
}

void BufferTest::testGetUTF16()
{
    // GREEK_SMALL_LETTER_ALPHA has a non-zero most significant byte in Unicode and UTF-16
    const ts::UString mem = ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA}) + u"cdefgh";
    debug() << "BufferTest::testGetUTF16: reference = \"" << mem << "\"" << std::endl;

    // Run the test in native endian.
    ts::Buffer b(mem.data(), 2 * mem.size());
    b.setNativeEndian();
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(u"", b.getUTF16(0));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(0, b.currentReadByteOffset());

    TSUNIT_EQUAL(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c'}), b.getUTF16(6));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(6, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"de", b.getUTF16(4));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.getUTF16(20));
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());

    // Same test in opposite endian.
    const ts::ByteBlock mem2(SwapBytes(mem));
    ts::Buffer b2(mem2.data(), mem2.size());
    b2.setNativeEndian();
    b2.switchEndian();
    TSUNIT_ASSERT(b2.readOnly());
    TSUNIT_ASSERT(b.isLittleEndian() + b2.isLittleEndian() == 1);
    TSUNIT_ASSERT(b.isBigEndian() + b2.isBigEndian() == 1);

    TSUNIT_EQUAL(u"", b2.getUTF16(0));
    TSUNIT_ASSERT(!b2.readError());
    TSUNIT_EQUAL(0, b2.currentReadByteOffset());

    TSUNIT_EQUAL(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c'}), b2.getUTF16(6));
    TSUNIT_ASSERT(!b2.readError());
    TSUNIT_EQUAL(6, b2.currentReadByteOffset());

    TSUNIT_EQUAL(u"de", b2.getUTF16(4));
    TSUNIT_ASSERT(!b2.readError());
    TSUNIT_EQUAL(10, b2.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b2.getUTF16(20));
    TSUNIT_ASSERT(b2.readError());
    TSUNIT_EQUAL(10, b2.currentReadByteOffset());
}

void BufferTest::testGetUTF16WithLength()
{
    static const uint16_t mem[] = {6, u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c', 4, u'd', u'e', 8, u'f', u'g', u'h'};

    // Run the test in native endian.
    ts::Buffer b(mem, sizeof(mem));
    b.setNativeEndian();
    TSUNIT_ASSERT(b.readOnly());

    TSUNIT_EQUAL(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c'}), b.getUTF16WithLength(16));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(8, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"de", b.getUTF16WithLength(16));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_EQUAL(14, b.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b.getUTF16WithLength(16));
    TSUNIT_ASSERT(b.readError());
    TSUNIT_EQUAL(14, b.currentReadByteOffset());

    // Same test in opposite endian.
    const ts::ByteBlock mem2(SwapBytes(mem, sizeof(mem)));
    ts::Buffer b2(mem2.data(), mem2.size());
    b2.setNativeEndian();
    b2.switchEndian();
    TSUNIT_ASSERT(b2.readOnly());
    TSUNIT_ASSERT(b.isLittleEndian() + b2.isLittleEndian() == 1);
    TSUNIT_ASSERT(b.isBigEndian() + b2.isBigEndian() == 1);

    TSUNIT_EQUAL(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c'}), b2.getUTF16WithLength(16));
    TSUNIT_ASSERT(!b2.readError());
    TSUNIT_EQUAL(8, b2.currentReadByteOffset());

    TSUNIT_EQUAL(u"de", b2.getUTF16WithLength(16));
    TSUNIT_ASSERT(!b2.readError());
    TSUNIT_EQUAL(14, b2.currentReadByteOffset());

    TSUNIT_EQUAL(u"", b2.getUTF16WithLength(16));
    TSUNIT_ASSERT(b2.readError());
    TSUNIT_EQUAL(14, b2.currentReadByteOffset());
}

void BufferTest::testPutUTF8()
{
    uint8_t mem[10];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF8(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(2, b.currentWriteByteOffset());
    TSUNIT_EQUAL('d', mem[0]);
    TSUNIT_EQUAL('e', mem[1]);

    TSUNIT_ASSERT(b.putUTF8(u"xyz"));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL('x', mem[2]);
    TSUNIT_EQUAL('y', mem[3]);
    TSUNIT_EQUAL('z', mem[4]);

    TSUNIT_ASSERT(!b.putUTF8(u"123456789"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
}

void BufferTest::testPutFixedUTF8()
{
    uint8_t mem[10];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFixedUTF8(u"abcde", 2));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(2, b.currentWriteByteOffset());
    TSUNIT_EQUAL('a', mem[0]);
    TSUNIT_EQUAL('b', mem[1]);

    TSUNIT_ASSERT(b.putFixedUTF8(u"x", 3, ' '));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL('x', mem[2]);
    TSUNIT_EQUAL(' ', mem[3]);
    TSUNIT_EQUAL(' ', mem[4]);

    TSUNIT_ASSERT(!b.putFixedUTF8(u"9", 9));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
}

void BufferTest::testPutPartialUTF8()
{
    uint8_t mem[5];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(2, b.putPartialUTF8(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(2, b.currentWriteByteOffset());
    TSUNIT_EQUAL('d', mem[0]);
    TSUNIT_EQUAL('e', mem[1]);

    TSUNIT_EQUAL(3, b.putPartialUTF8(u"ijklmn", 1));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.endOfWrite());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL('j', mem[2]);
    TSUNIT_EQUAL('k', mem[3]);
    TSUNIT_EQUAL('l', mem[4]);
}

void BufferTest::testPutUTF8WithLength()
{
    uint8_t mem[10];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF8WithLength(u"abcde", 3, 5));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());
    TSUNIT_EQUAL(2, mem[0]);
    TSUNIT_EQUAL('d', mem[1]);
    TSUNIT_EQUAL('e', mem[2]);

    // Cannot write if not byte-aligned after length field.
    TSUNIT_ASSERT(!b.putUTF8WithLength(u"ijk", 0, ts::NPOS, 7));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putBits(0xFF, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.writeIsByteAligned());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());

    // Cannot write 9 bytes if length field is 3 bits.
    TSUNIT_ASSERT(!b.putUTF8WithLength(u"123456789", 0, ts::NPOS, 3));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(3, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF8WithLength(u"12", 0, ts::NPOS, 3));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(6, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0xFA, mem[3]);
    TSUNIT_EQUAL('1', mem[4]);
    TSUNIT_EQUAL('2', mem[5]);

    // Too large for buffer
    TSUNIT_ASSERT(!b.putUTF8WithLength(u"1234"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(6, b.currentWriteByteOffset());
}

void BufferTest::testPutPartialUTF8WithLength()
{
    uint8_t mem[10];
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(5, b.putPartialUTF8WithLength(u"abcde", 0, ts::NPOS, 16));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(7, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[0]);
    TSUNIT_EQUAL(0x05, mem[1]);
    TSUNIT_EQUAL('a', mem[2]);
    TSUNIT_EQUAL('b', mem[3]);
    TSUNIT_EQUAL('c', mem[4]);
    TSUNIT_EQUAL('d', mem[5]);
    TSUNIT_EQUAL('e', mem[6]);

    TSUNIT_EQUAL(2, b.putPartialUTF8WithLength(u"123456789"));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.endOfWrite());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x02, mem[7]);
    TSUNIT_EQUAL('1', mem[8]);
    TSUNIT_EQUAL('2', mem[9]);
}

void BufferTest::testPutUTF16()
{
    uint8_t mem[20];

    // Run the test in big endian.
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    b.setBigEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[0]);
    TSUNIT_EQUAL('d',  mem[1]);
    TSUNIT_EQUAL(0x00, mem[2]);
    TSUNIT_EQUAL('e',  mem[3]);

    TSUNIT_ASSERT(b.putUTF16(u"xyz"));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[4]);
    TSUNIT_EQUAL('x',  mem[5]);
    TSUNIT_EQUAL(0x00, mem[6]);
    TSUNIT_EQUAL('y',  mem[7]);
    TSUNIT_EQUAL(0x00, mem[8]);
    TSUNIT_EQUAL('z',  mem[9]);

    TSUNIT_ASSERT(!b.putUTF16(u"1234567"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());

    // Run the test in little endian.
    ::memset(mem, 0, sizeof(mem));
    b.reset(mem, sizeof(mem));
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL('d',  mem[0]);
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL('e',  mem[2]);
    TSUNIT_EQUAL(0x00, mem[3]);

    TSUNIT_ASSERT(b.putUTF16(u"xyz"));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
    TSUNIT_EQUAL('x',  mem[4]);
    TSUNIT_EQUAL(0x00, mem[5]);
    TSUNIT_EQUAL('y',  mem[6]);
    TSUNIT_EQUAL(0x00, mem[7]);
    TSUNIT_EQUAL('z',  mem[8]);
    TSUNIT_EQUAL(0x00, mem[9]);

    TSUNIT_ASSERT(!b.putUTF16(u"1234567"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
}

void BufferTest::testPutFixedUTF16()
{
    uint8_t mem[20];

    // Run the test in big endian.
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    b.setBigEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFixedUTF16(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c', u'd', u'e'}), 4));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[0]);
    TSUNIT_EQUAL('a',  mem[1]);
    TSUNIT_EQUAL(ts::GREEK_SMALL_LETTER_ALPHA >> 8, mem[2]);
    TSUNIT_EQUAL(ts::GREEK_SMALL_LETTER_ALPHA & 0xFF, mem[3]);

    TSUNIT_ASSERT(b.putFixedUTF16(u"x", 7, u' '));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[4]);
    TSUNIT_EQUAL('x',  mem[5]);
    TSUNIT_EQUAL(0x00, mem[6]);
    TSUNIT_EQUAL(' ',  mem[7]);
    TSUNIT_EQUAL(0x00, mem[8]);
    TSUNIT_EQUAL(' ',  mem[9]);
    TSUNIT_EQUAL(' ',  mem[10]);

    TSUNIT_ASSERT(!b.putFixedUTF8(u"9", 10));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());

    // Run the test in little endian.
    ::memset(mem, 0, sizeof(mem));
    b.reset(mem, sizeof(mem));
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFixedUTF16(ts::UString({u'a', ts::GREEK_SMALL_LETTER_ALPHA, u'c', u'd', u'e'}), 4));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL('a',  mem[0]);
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL(ts::GREEK_SMALL_LETTER_ALPHA & 0xFF, mem[2]);
    TSUNIT_EQUAL(ts::GREEK_SMALL_LETTER_ALPHA >> 8, mem[3]);

    TSUNIT_ASSERT(b.putFixedUTF16(u"x", 7, u' '));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL('x',  mem[4]);
    TSUNIT_EQUAL(0x00, mem[5]);
    TSUNIT_EQUAL(' ',  mem[6]);
    TSUNIT_EQUAL(0x00, mem[7]);
    TSUNIT_EQUAL(' ',  mem[8]);
    TSUNIT_EQUAL(0x00, mem[9]);
    TSUNIT_EQUAL(' ',  mem[10]);

    TSUNIT_ASSERT(!b.putFixedUTF8(u"9", 10));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
}

void BufferTest::testPutPartialUTF16()
{
    uint8_t mem[11];

    // Run the test in big endian.
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    b.setBigEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(2, b.putPartialUTF16(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[0]);
    TSUNIT_EQUAL('d',  mem[1]);
    TSUNIT_EQUAL(0x00, mem[2]);
    TSUNIT_EQUAL('e',  mem[3]);

    TSUNIT_EQUAL(3, b.putPartialUTF16(u"ijklmn", 1));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[4]);
    TSUNIT_EQUAL('j',  mem[5]);
    TSUNIT_EQUAL(0x00, mem[6]);
    TSUNIT_EQUAL('k',  mem[7]);
    TSUNIT_EQUAL(0x00, mem[8]);
    TSUNIT_EQUAL('l',  mem[9]);

    // Run the test in little endian.
    ::memset(mem, 0, sizeof(mem));
    b.reset(mem, sizeof(mem));
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(2, b.putPartialUTF16(u"abcde", 3, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());
    TSUNIT_EQUAL('d',  mem[0]);
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL('e',  mem[2]);
    TSUNIT_EQUAL(0x00, mem[3]);

    TSUNIT_EQUAL(3, b.putPartialUTF16(u"ijklmn", 1));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(10, b.currentWriteByteOffset());
    TSUNIT_EQUAL('j',  mem[4]);
    TSUNIT_EQUAL(0x00, mem[5]);
    TSUNIT_EQUAL('k',  mem[6]);
    TSUNIT_EQUAL(0x00, mem[7]);
    TSUNIT_EQUAL('l',  mem[8]);
    TSUNIT_EQUAL(0x00, mem[9]);
}

void BufferTest::testPutUTF16WithLength()
{
    uint8_t mem[20];

    // Run the test in big endian.
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    b.setBigEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16WithLength(u"abcde", 3, 5));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x04, mem[0]);
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL('d',  mem[2]);
    TSUNIT_EQUAL(0x00, mem[3]);
    TSUNIT_EQUAL('e',  mem[4]);

    // Cannot write if not byte-aligned after length field.
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"ijk", 0, ts::NPOS, 7));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putBits(0xFF, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.writeIsByteAligned());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    // Cannot write 8 bytes if length field is 3 bits.
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"1234", 0, ts::NPOS, 3));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16WithLength(u"12", 0, ts::NPOS, 11));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0xF8, mem[5]);
    TSUNIT_EQUAL(0x04, mem[6]);
    TSUNIT_EQUAL(0x00, mem[7]);
    TSUNIT_EQUAL('1',  mem[8]);
    TSUNIT_EQUAL(0x00, mem[9]);
    TSUNIT_EQUAL('2',  mem[10]);

    // Too large for buffer
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"12345"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());

    // Run the test in little endian.
    ::memset(mem, 0, sizeof(mem));
    b.reset(mem, sizeof(mem));
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16WithLength(u"abcde", 3, 5));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x04, mem[0]);
    TSUNIT_EQUAL('d',  mem[1]);
    TSUNIT_EQUAL(0x00, mem[2]);
    TSUNIT_EQUAL('e',  mem[3]);
    TSUNIT_EQUAL(0x00, mem[4]);

    // Cannot write if not byte-aligned after length field.
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"ijk", 0, ts::NPOS, 7));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putBits(0xFF, 5));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.writeIsByteAligned());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    // Cannot write 8 bytes if length field is 3 bits.
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"1234", 0, ts::NPOS, 3));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    b.clearWriteError();
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_EQUAL(5, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putUTF16WithLength(u"12", 0, ts::NPOS, 11));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x9F, mem[5]); // 0x9F = 100 11111 = 3-bit length field: 4, previous 5-bit integer field: 0x1F
    TSUNIT_EQUAL(0x00, mem[6]);
    TSUNIT_EQUAL('1',  mem[7]);
    TSUNIT_EQUAL(0x00, mem[8]);
    TSUNIT_EQUAL('2',  mem[9]);
    TSUNIT_EQUAL(0x00, mem[10]);

    // Too large for buffer
    TSUNIT_ASSERT(!b.putUTF16WithLength(u"12345"));
    TSUNIT_ASSERT(b.writeError());
    TSUNIT_EQUAL(11, b.currentWriteByteOffset());
}

void BufferTest::testPutPartialUTF16WithLength()
{
    uint8_t mem[20];

    // Run the test in big endian.
    ::memset(mem, 0, sizeof(mem));
    ts::Buffer b(mem, sizeof(mem));
    b.setBigEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(5, b.putPartialUTF16WithLength(u"abcde", 0, ts::NPOS, 16));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(12, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x00, mem[0]);
    TSUNIT_EQUAL(0x0A, mem[1]);
    TSUNIT_EQUAL(0x00, mem[2]);
    TSUNIT_EQUAL('a',  mem[3]);
    TSUNIT_EQUAL(0x00, mem[4]);
    TSUNIT_EQUAL('b',  mem[5]);
    TSUNIT_EQUAL(0x00, mem[6]);
    TSUNIT_EQUAL('c',  mem[7]);
    TSUNIT_EQUAL(0x00, mem[8]);
    TSUNIT_EQUAL('d',  mem[9]);
    TSUNIT_EQUAL(0x00, mem[10]);
    TSUNIT_EQUAL('e',  mem[11]);

    TSUNIT_EQUAL(3, b.putPartialUTF16WithLength(u"123456789"));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(19, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x06, mem[12]);
    TSUNIT_EQUAL(0x00, mem[13]);
    TSUNIT_EQUAL('1',  mem[14]);
    TSUNIT_EQUAL(0x00, mem[15]);
    TSUNIT_EQUAL('2',  mem[16]);
    TSUNIT_EQUAL(0x00, mem[17]);
    TSUNIT_EQUAL('3',  mem[18]);

    // Run the test in little endian.
    ::memset(mem, 0, sizeof(mem));
    b.reset(mem, sizeof(mem));
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_EQUAL(5, b.putPartialUTF16WithLength(u"abcde", 0, ts::NPOS, 16));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(12, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x0A, mem[0]);
    TSUNIT_EQUAL(0x00, mem[1]);
    TSUNIT_EQUAL('a',  mem[2]);
    TSUNIT_EQUAL(0x00, mem[3]);
    TSUNIT_EQUAL('b',  mem[4]);
    TSUNIT_EQUAL(0x00, mem[5]);
    TSUNIT_EQUAL('c',  mem[6]);
    TSUNIT_EQUAL(0x00, mem[7]);
    TSUNIT_EQUAL('d',  mem[8]);
    TSUNIT_EQUAL(0x00, mem[9]);
    TSUNIT_EQUAL('e',  mem[10]);
    TSUNIT_EQUAL(0x00, mem[11]);

    TSUNIT_EQUAL(3, b.putPartialUTF16WithLength(u"123456789"));
    TSUNIT_ASSERT(b.writeIsByteAligned());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(19, b.currentWriteByteOffset());
    TSUNIT_EQUAL(0x06, mem[12]);
    TSUNIT_EQUAL('1',  mem[13]);
    TSUNIT_EQUAL(0x00, mem[14]);
    TSUNIT_EQUAL('2',  mem[15]);
    TSUNIT_EQUAL(0x00, mem[14]);
    TSUNIT_EQUAL('3',  mem[17]);
    TSUNIT_EQUAL(0x00, mem[18]);
}

void BufferTest::testGetFloat32LE()
{
    static const uint8_t bin[] = {0x00, 0x00, 0x80, 0x25};
    ts::Buffer b(bin, sizeof(bin));
    b.setLittleEndian();
    TSUNIT_EQUAL(2.2204460e-16, b.getFloat32());
}

void BufferTest::testGetFloat32BE()
{
    static const uint8_t bin[] = {0x07, 0x80, 0x00, 0x00};
    ts::Buffer b(bin, sizeof(bin));
    TSUNIT_EQUAL(1.9259299e-34, b.getFloat32());
}

void BufferTest::testGetFloat64LE()
{
    static const uint8_t bin[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E};
    ts::Buffer b(bin, sizeof(bin));
    b.setLittleEndian();
    TSUNIT_EQUAL(1.1920928955078125e-07, b.getFloat64());
}

void BufferTest::testGetFloat64BE()
{
    static const uint8_t bin[] = {0x47, 0xEF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00};
    ts::Buffer b(bin, sizeof(bin));
    TSUNIT_EQUAL(3.4028234663852886e+38, b.getFloat64());
}

void BufferTest::testPutFloat32LE()
{
    tsunit::Bytes mem(10, 0xAC);
    ts::Buffer b(&mem[0], mem.size());
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFloat32(1.9259299e-34));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());

    mem.resize(4);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x80, 0x07}), mem);
}

void BufferTest::testPutFloat32BE()
{
    tsunit::Bytes mem(10, 0xAC);
    ts::Buffer b(&mem[0], mem.size());

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFloat32(3.4028235e+38));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(4, b.currentWriteByteOffset());

    mem.resize(4);
    TSUNIT_EQUAL((tsunit::Bytes{0x7F, 0x7F, 0xFF, 0xFF}), mem);
}

void BufferTest::testPutFloat64LE()
{
    tsunit::Bytes mem(10, 0xAC);
    ts::Buffer b(&mem[0], mem.size());
    b.setLittleEndian();

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFloat64(1.9259299443872359e-34));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(8, b.currentWriteByteOffset());

    mem.resize(8);
    TSUNIT_EQUAL((tsunit::Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x38}), mem);
}

void BufferTest::testPutFloat64BE()
{
    tsunit::Bytes mem(10, 0xAC);
    ts::Buffer b(&mem[0], mem.size());

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putFloat64(3.4028234663852886e+38));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(8, b.currentWriteByteOffset());

    mem.resize(8);
    TSUNIT_EQUAL((tsunit::Bytes{0x47, 0xEF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00}), mem);
}

void BufferTest::testGetVluimsbf5()
{
    ts::DuckContext duck;

    static const uint8_t bin1[] = {0x00};
    ts::PSIBuffer b(duck, bin1, sizeof(bin1));
    TSUNIT_EQUAL(0x00, b.getVluimsbf5());
    TSUNIT_EQUAL(5, b.currentReadBitOffset());

    static const uint8_t bin2[] = {0xC2, 0x46};
    b.reset(bin2, sizeof(bin2));
    TSUNIT_EQUAL(0x0123, b.getVluimsbf5());
    TSUNIT_EQUAL(15, b.currentReadBitOffset());

    static const uint8_t bin3[] = {0xEF, 0xFF, 0xF0};
    b.reset(bin3, sizeof(bin3));
    TSUNIT_EQUAL(0xFFFF, b.getVluimsbf5());
    TSUNIT_EQUAL(20, b.currentReadBitOffset());

    static const uint8_t bin4[] = {0xF0, 0xD5, 0xE6, 0x80};
    b.reset(bin4, sizeof(bin4));
    TSUNIT_EQUAL(0x1ABCD, b.getVluimsbf5());
    TSUNIT_EQUAL(25, b.currentReadBitOffset());
}

void BufferTest::testPutVluimsbf5()
{
    ts::DuckContext duck;
    tsunit::Bytes mem(32);
    ts::PSIBuffer b(duck, mem.data(), mem.size());

    TSUNIT_ASSERT(!b.readOnly());
    TSUNIT_EQUAL(0, b.currentWriteByteOffset());

    TSUNIT_ASSERT(b.putVluimsbf5(0));
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.endOfWrite());
    TSUNIT_EQUAL(5, b.currentWriteBitOffset());

    mem.resize(1);
    TSUNIT_EQUAL((tsunit::Bytes{0x00}), mem);

    mem.resize(32);
    b.reset(mem.data(), mem.size());
    TSUNIT_ASSERT(b.putVluimsbf5(0x123));
    TSUNIT_EQUAL(15, b.currentWriteBitOffset());
    mem.resize(2);
    TSUNIT_EQUAL((tsunit::Bytes{0xC2, 0x46}), mem);

    mem.resize(32);
    b.reset(mem.data(), mem.size());
    TSUNIT_ASSERT(b.putVluimsbf5(0xFFFF));
    // 1111.1111.1111.1111
    // --> 1110-.1111.1111.1111.1111
    TSUNIT_EQUAL(20, b.currentWriteBitOffset());
    mem.resize(3);
    TSUNIT_EQUAL((tsunit::Bytes{0xEF, 0xFF, 0xF0}), mem);

    mem.resize(32);
    b.reset(mem.data(), mem.size());
    TSUNIT_ASSERT(b.putVluimsbf5(0x1ABCD));
    // 0001.1010.1011.1100.1101
    // --> 1111.0-000.1101.0101.1110.0110.1-000
    TSUNIT_EQUAL(25, b.currentWriteBitOffset());
    mem.resize(4);
    TSUNIT_EQUAL((tsunit::Bytes{0xF0, 0xD5, 0xE6, 0x80}), mem);
}
