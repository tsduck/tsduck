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
    void testReadBit();
    void testReadBits();

    TSUNIT_TEST_BEGIN(BufferTest);
    TSUNIT_TEST(testConstructors);
    TSUNIT_TEST(testReadBit);
    TSUNIT_TEST(testReadBits);
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
    const uint8_t _bytes[16] = {
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
}

void BufferTest::testReadBit()
{
    // 0          10         20         30          40         50         60         70          80     87
    // |          |          |          |           |          |          |          |           |      |
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010
    ts::Buffer b(_bytes, 11);

    TSUNIT_ASSERT(b.valid());
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
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(b.readIsByteAligned());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(!b.readIsByteAligned());
    TSUNIT_EQUAL(1, b.readBit());

    // Offset 10
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());

    // Offset 20
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());

    // Offset 30
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());

    // Offset 40
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());

    // Offset 50
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());

    // Offset 60
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());

    // Offset 70
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(9, b.currentReadByteOffset());
    TSUNIT_EQUAL(79, b.currentReadBitOffset());
    TSUNIT_EQUAL(1, b.readBit());

    // Offset 80
    TSUNIT_EQUAL(10, b.currentReadByteOffset());
    TSUNIT_EQUAL(80, b.currentReadBitOffset());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(10, b.currentReadByteOffset());
    TSUNIT_EQUAL(81, b.currentReadBitOffset());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_EQUAL(1, b.readBit());
    TSUNIT_ASSERT(!b.endOfRead());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(b.endOfRead());

    // End of stream
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(!b.error());
    TSUNIT_EQUAL(0, b.readBit());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.writeError());
    TSUNIT_ASSERT(b.error());
    TSUNIT_EQUAL(1, b.readBit(1));
    TSUNIT_EQUAL(0, b.readBit(0));
}

void BufferTest::testReadBits()
{
    // 0          10         20         30          40         50         60         70          80         90
    // |          |          |          |           |          |          |          |           |          |
    // 01010000 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010 01101011
    ts::Buffer b(_bytes, 12);

    // 0101000
    TSUNIT_EQUAL(0x28, b.readBits<int>(7));
    TSUNIT_EQUAL(7, b.currentReadBitOffset());

    // 00101
    TSUNIT_EQUAL(0x05, b.readBits<int>(5));
    TSUNIT_EQUAL(12, b.currentReadBitOffset());

    // 000101010010010
    TSUNIT_EQUAL(0xA92, b.readBits<uint16_t>(15));
    TSUNIT_EQUAL(27, b.currentReadBitOffset());

    // Important: Due to a code generation bug in Microsoft Visual C++ 2010,
    // the default value (second parameter of Buffer::readBits)
    // must be explicitly provided when instantiated on 64-bit integer types.

    // 100110101010001010101010101100101011101101000011010010110101
    TSUNIT_EQUAL(TS_UCONST64(0x9AA2AAB2BB434B5), b.readBits<uint64_t>(60, 0));
    TSUNIT_EQUAL(87, b.currentReadBitOffset());

    // 001101011 (9 remaining bits)
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(-1, b.readBits<int32_t>(10, -1)); // after eof
    TSUNIT_EQUAL(87, b.currentReadBitOffset());
    TSUNIT_EQUAL(9, b.remainingReadBits());
    TSUNIT_ASSERT(b.readError());
    TSUNIT_ASSERT(!b.endOfRead());
    b.clearError();
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(!b.endOfRead());

    TSUNIT_EQUAL(0x6B, b.readBits<int32_t>(9, -1));
    TSUNIT_ASSERT(!b.readError());
    TSUNIT_ASSERT(b.endOfRead());
    TSUNIT_EQUAL(96, b.currentReadBitOffset());
}
