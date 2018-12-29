//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  CppUnit test suite for utilities in class ts::BitStream
//
//----------------------------------------------------------------------------

#include "tsBitStream.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class BitStreamTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testConstructors();
    void testAssignment();
    void testReset();
    void testSeek();
    void testByteAligned();
    void testSkip();
    void testBack();
    void testSkipToNextByte();
    void testReadBit();
    void testRead();

    CPPUNIT_TEST_SUITE (BitStreamTest);
    CPPUNIT_TEST (testConstructors);
    CPPUNIT_TEST (testAssignment);
    CPPUNIT_TEST (testReset);
    CPPUNIT_TEST (testSeek);
    CPPUNIT_TEST (testByteAligned);
    CPPUNIT_TEST (testSkip);
    CPPUNIT_TEST (testBack);
    CPPUNIT_TEST (testSkipToNextByte);
    CPPUNIT_TEST (testReadBit);
    CPPUNIT_TEST (testRead);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BitStreamTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void BitStreamTest::setUp()
{
}

// Test suite cleanup method.
void BitStreamTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Reference byte array
namespace {
    const uint8_t _bytes[] = {
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

// Test cases
void BitStreamTest::testConstructors()
{
    ts::BitStream bs1;
    CPPUNIT_ASSERT(!bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());

    CPPUNIT_ASSERT(sizeof(_bytes) <= 7 + 83);

    ts::BitStream bs2 (_bytes, 83, 7);
    CPPUNIT_ASSERT(bs2.isAssociated());
    CPPUNIT_ASSERT(bs2.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs2.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs2.endOfStream());

    ts::BitStream bs3 (bs2);
    CPPUNIT_ASSERT(bs3.isAssociated());
    CPPUNIT_ASSERT(bs3.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs3.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs3.endOfStream());
}

void BitStreamTest::testAssignment()
{
    ts::BitStream bs1 (_bytes, 83, 7);
    ts::BitStream bs2;

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs2 = bs1;

    CPPUNIT_ASSERT(bs2.isAssociated());
    CPPUNIT_ASSERT(bs2.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs2.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs2.endOfStream());

    // Returned value ignored on purpose, we just want to move on in the bitstream.
    // coverity[CHECKED_RETURN]
    bs2.readBit();
    // coverity[CHECKED_RETURN]
    bs2.readBit();

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    CPPUNIT_ASSERT(bs2.isAssociated());
    CPPUNIT_ASSERT(bs2.currentBitOffset() == 2);
    CPPUNIT_ASSERT(bs2.remainingBitCount() == 81);
    CPPUNIT_ASSERT(!bs2.endOfStream());
}

void BitStreamTest::testReset()
{
    ts::BitStream bs1;

    bs1.reset (_bytes, 83, 7);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());
}

void BitStreamTest::testSeek()
{
    ts::BitStream bs1 (_bytes, 83, 7);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.seek (48);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 48);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 35);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.seek (150);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());

    bs1.seek (83);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());

    bs1.seek (82);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 82);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 1);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.seek (0);

    CPPUNIT_ASSERT(bs1.isAssociated());
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());
}

void BitStreamTest::testByteAligned()
{
    ts::BitStream bs1 (_bytes, 83, 7);

    CPPUNIT_ASSERT(!bs1.byteAligned());

    // Returned value ignored on purpose, we just want to move on in the bitstream.
    // coverity[CHECKED_RETURN]
    bs1.readBit();
    CPPUNIT_ASSERT(bs1.byteAligned());

    // coverity[CHECKED_RETURN]
    bs1.readBit();
    CPPUNIT_ASSERT(!bs1.byteAligned());

    bs1.seek (16);
    CPPUNIT_ASSERT(!bs1.byteAligned());

    bs1.seek (25);
    CPPUNIT_ASSERT(bs1.byteAligned());
}

void BitStreamTest::testSkip()
{
    ts::BitStream bs1 (_bytes, 83, 7);

    bs1.skip (5);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 5);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 78);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.skip (42);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 47);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 36);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.skip (40);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());

    bs1.seek (70);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 70);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 13);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.seek (83);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());
}

void BitStreamTest::testBack()
{
    ts::BitStream bs1 (_bytes, 83, 7);

    bs1.seek (83);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 0);
    CPPUNIT_ASSERT(bs1.endOfStream());

    bs1.back (13);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 70);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 13);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.back (55);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 15);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 68);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.back (30);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 0);
    CPPUNIT_ASSERT(bs1.remainingBitCount() == 83);
    CPPUNIT_ASSERT(!bs1.endOfStream());
}

void BitStreamTest::testSkipToNextByte()
{
    ts::BitStream bs1 (_bytes, 83, 7);

    bs1.skipToNextByte();
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 1);

    bs1.skipToNextByte();
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 9);

    bs1.seek (75);
    bs1.skipToNextByte();
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 81);
    CPPUNIT_ASSERT(!bs1.endOfStream());

    bs1.skipToNextByte();
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.endOfStream());
}

void BitStreamTest::testReadBit()
{
    // 0           10         20         30         40          50         60         70         80
    // |           |          |          |          |           |          |          |          |
    // 0 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010 01
    ts::BitStream bs1 (_bytes, 83, 7);

    // Offset 0
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    // Offset 10
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    // Offset 20
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    // Offset 30
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    // Offset 40
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    // Offset 50
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    // Offset 60
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    // Offset 70
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    // Offset 80
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 1);
    // End of stream
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit() == 0);
    CPPUNIT_ASSERT(bs1.readBit(0) == 0);
    CPPUNIT_ASSERT(bs1.readBit(1) == 1);
}

void BitStreamTest::testRead()
{
    // 0           10         20         30         40          50         60         70         80
    // |           |          |          |          |           |          |          |          |
    // 0 01010001 01010010 01010011 01010100 01010101 01010110 01010111 01101000 01101001 01101010 01
    ts::BitStream bs1 (_bytes, 83, 7);

    // 00101
    CPPUNIT_ASSERT(bs1.read<int>(5) == 0x5);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 5);

    // 000101010010010
    CPPUNIT_ASSERT(bs1.read<uint16_t>(15) == 0xA92);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 20);

    // Important: Due to a code generation bug in Microsoft Visual C++ 2010,
    // the default value (second parameter of BitStream::read)
    // must be explicitly provided when instantiated on 64-bit integer types.

    // 100110101010001010101010101100101011101101000011010010110101
    CPPUNIT_ASSERT(bs1.read<uint64_t>(60, 0) == TS_UCONST64(0x9AA2AAB2BB434B5));
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 80);

    CPPUNIT_ASSERT(bs1.read<int32_t>(8) == 0); // after eof
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 80);

    CPPUNIT_ASSERT(bs1.read<int32_t>(8, 0) == 0); // after eof
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 80);

    CPPUNIT_ASSERT(bs1.read<int32_t>(8, -1) == -1); // after eof
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 80);

    // 001
    CPPUNIT_ASSERT(bs1.read<int32_t>(3) == 1);
    CPPUNIT_ASSERT(bs1.currentBitOffset() == 83);
    CPPUNIT_ASSERT(bs1.endOfStream());
}
