//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsBuffer.h"
#include "tsunit.h"
TSDUCK_SOURCE;


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
    TSUNIT_TEST_END();
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
    TSUNIT_EQUAL(1, b.getBit(1));
    TSUNIT_EQUAL(0, b.getBit(0));
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
    TSUNIT_EQUAL(1, b.getBit(1));
    TSUNIT_EQUAL(0, b.getBit(0));
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
    TSUNIT_EQUAL(TS_UCONST64(0x9AA2AAB2BB434B5), b.getBits<uint64_t>(60, 0));
    TSUNIT_EQUAL(87, b.currentReadBitOffset());

    // 001101011 (9 remaining bits)
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(-1, b.getBits<int32_t>(10, -1)); // after eof
    TSUNIT_EQUAL(87, b.currentReadBitOffset());
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.endOfRead());
    b.clearError();
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(0x6B, b.getBits<int32_t>(9, -1));
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
