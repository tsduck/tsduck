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
//  TSUnit test suite for codex-related classes.
//
//----------------------------------------------------------------------------

#include "tsAccessUnitIterator.h"
#include "tsAVC.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CodecsTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testIterator();

    TSUNIT_TEST_BEGIN(CodecsTest);
    TSUNIT_TEST(testIterator);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(CodecsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CodecsTest::beforeTest()
{
}

// Test suite cleanup method.
void CodecsTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void CodecsTest::testIterator()
{
    static const uint8_t data[] = {
        0x00, 0x00, 0x00, 0x01, 0x09, 0x50, 0x00, 0x00, 0x01, 0x06, 0x01, 0x01, 0x32, 0x80, 0x00, 0x00,
        0x00, 0x01, 0x41, 0x9F, 0xFC, 0x06, 0x2F, 0x11, 0xD6, 0x7F, 0xE5, 0x5E, 0x39, 0xD5, 0xA2, 0x55,
        0x88, 0x7C, 0x88, 0x98, 0x9C, 0x67, 0xD3, 0x11, 0x26, 0xE9, 0x68, 0x1A, 0xE4, 0xAD, 0xB9, 0xBE,
        0xF7, 0xD5, 0xE5, 0xC2, 0xC7, 0x05, 0x1C, 0x46, 0x10, 0xAC, 0x96, 0x9A, 0x0A, 0xD8, 0x5D, 0xA4,
        0x80, 0xF4, 0xC7, 0xF3, 0xF0, 0xCC, 0x7C, 0x7B, 0x8D, 0x21, 0x44, 0x54, 0x9B, 0x94, 0xF8, 0xC1,
        0xA2, 0xD2, 0x00, 0xF3, 0xB3, 0x5A, 0x4E, 0xC1, 0xE3, 0xB6, 0x67, 0x02, 0x15, 0x8F, 0x22, 0x20,
        0x0E, 0x95, 0x25, 0x65, 0x52, 0xB9, 0xDD, 0x82, 0xC8, 0x3D, 0x83, 0x42, 0xDC, 0x7F, 0x3F, 0xFB,
        0x59, 0xF9, 0xF4, 0x1F, 0x64, 0x21, 0xB0, 0xBC, 0x2A, 0xC0, 0xDC, 0x1A, 0x00, 0x1D, 0x4C, 0xE1,
        0x2D, 0x71, 0x56, 0x96, 0x6D, 0x41, 0x0D, 0xC4, 0xDC, 0x3E, 0x17, 0x8B, 0x76, 0x47, 0xE0, 0xFE,
        0x0E, 0xDD, 0x48, 0x43, 0xB6, 0xF6, 0xF4, 0x3F, 0xA5, 0xF1, 0xD0, 0xC2, 0x61, 0xD5, 0x59, 0x4B,
        0x8D, 0xFC, 0x2A, 0xD3, 0xFA, 0x07, 0x9A, 0xEB, 0x90, 0xCC, 0x07, 0xEF, 0x59, 0x47, 0xBD, 0x8E,
        0xF2, 0x3F, 0xA3, 0xF5, 0x3C, 0x0B, 0x4E, 0x00, 0xA1, 0x6C, 0x1A, 0xF5, 0x35, 0x11, 0xE4, 0x89,
        0xC1, 0x04, 0x68, 0x3B, 0xED, 0xCF, 0x35, 0xB4, 0xF1, 0x41, 0x85, 0xEC, 0x65, 0x3D, 0xFA, 0x38,
        0xED, 0x43, 0x9D, 0x16, 0x2C, 0x5A, 0xBB, 0x30, 0xA7, 0xE6, 0x08, 0x9F, 0xB2, 0x02, 0x14, 0x4A,
        0xB7, 0xF9, 0x6A, 0xAF, 0x08, 0x60, 0xA3, 0x52, 0x58, 0x9C, 0x42, 0x27, 0x06, 0x40, 0x48, 0x7C,
        0xA4, 0x0A, 0xD9, 0xFF, 0xAD, 0x37, 0x08, 0x9C, 0x30, 0x45, 0x0B, 0x58, 0x91, 0x38, 0x58, 0x6A,
    };

    ts::AccessUnitIterator iter(data, sizeof(data), ts::ST_AVC_VIDEO);

    TSUNIT_ASSERT(iter.isValid());
    TSUNIT_EQUAL(ts::CodecType::AVC, iter.videoFormat());

    TSUNIT_ASSERT(!iter.atEnd());
    TSUNIT_EQUAL(4, iter.currentAccessUnit() - data);
    TSUNIT_EQUAL(4, iter.currentAccessUnitOffset());
    TSUNIT_EQUAL(0, iter.currentAccessUnitIndex());
    TSUNIT_EQUAL(1, iter.currentAccessUnitHeaderSize());
    TSUNIT_EQUAL(2, iter.currentAccessUnitSize());
    TSUNIT_EQUAL(ts::AVC_AUT_DELIMITER, iter.currentAccessUnitType());
    TSUNIT_ASSERT(!iter.currentAccessUnitIsSEI());

    TSUNIT_ASSERT(iter.next());

    TSUNIT_ASSERT(!iter.atEnd());
    TSUNIT_EQUAL(9, iter.currentAccessUnit() - data);
    TSUNIT_EQUAL(9, iter.currentAccessUnitOffset());
    TSUNIT_EQUAL(1, iter.currentAccessUnitIndex());
    TSUNIT_EQUAL(1, iter.currentAccessUnitHeaderSize());
    TSUNIT_EQUAL(5, iter.currentAccessUnitSize());
    TSUNIT_EQUAL(ts::AVC_AUT_SEI, iter.currentAccessUnitType());
    TSUNIT_ASSERT(iter.currentAccessUnitIsSEI());

    TSUNIT_ASSERT(iter.next());

    TSUNIT_ASSERT(!iter.atEnd());
    TSUNIT_EQUAL(18, iter.currentAccessUnit() - data);
    TSUNIT_EQUAL(18, iter.currentAccessUnitOffset());
    TSUNIT_EQUAL(2, iter.currentAccessUnitIndex());
    TSUNIT_EQUAL(1, iter.currentAccessUnitHeaderSize());
    TSUNIT_EQUAL(238, iter.currentAccessUnitSize());
    TSUNIT_EQUAL(ts::AVC_AUT_NON_IDR, iter.currentAccessUnitType());
    TSUNIT_ASSERT(!iter.currentAccessUnitIsSEI());

    TSUNIT_ASSERT(!iter.next());

    TSUNIT_ASSERT(iter.atEnd());
    TSUNIT_EQUAL(3, iter.currentAccessUnitIndex());

    TSUNIT_ASSERT(!iter.next());

    TSUNIT_ASSERT(iter.atEnd());
    TSUNIT_EQUAL(3, iter.currentAccessUnitIndex());
}
