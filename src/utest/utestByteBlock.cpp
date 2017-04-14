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
//  CppUnit test suite for ts::ByteBlock
//
//----------------------------------------------------------------------------

#include "tsByteBlock.h"
#include "utestCppUnitTest.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ByteBlockTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testAppend();

    CPPUNIT_TEST_SUITE (ByteBlockTest);
    CPPUNIT_TEST (testAppend);
    CPPUNIT_TEST_SUITE_END ();
};

CPPUNIT_TEST_SUITE_REGISTRATION (ByteBlockTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ByteBlockTest::setUp()
{
}

// Test suite cleanup method.
void ByteBlockTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void ByteBlockTest::testAppend()
{
    ts::ByteBlock v;
    ts::ByteBlock valtemp;
    valtemp.push_back(0x42);
    valtemp.push_back(0x65);
    std::string strtemp("a string");

    v.clear();
    CPPUNIT_ASSERT(v.empty());

    v.appendUInt8(0xAA);
    v.appendUInt16BE(0xAA55);
    v.appendUInt32BE(0xFFCCAA55);
    v.appendUInt64BE(0x87654321AABBCCDD);
    v.append(valtemp);
    v.append(strtemp);
    v.appendUInt8(0x3E);
    v.appendUInt16LE(0xAA55);
    v.appendUInt32LE(0xFFCCAA55);
    v.appendUInt64LE(0x87654321AABBCCDD);

    size_t idx = 0;
    CPPUNIT_ASSERT(v.size() == (1+2+4+8+2+8+1+2+4+8));
    CPPUNIT_ASSERT(v[idx++] == 0xAA);

    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0x55);

    CPPUNIT_ASSERT(v[idx++] == 0xFF);
    CPPUNIT_ASSERT(v[idx++] == 0xCC);
    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0x55);

    CPPUNIT_ASSERT(v[idx++] == 0x87);
    CPPUNIT_ASSERT(v[idx++] == 0x65);
    CPPUNIT_ASSERT(v[idx++] == 0x43);
    CPPUNIT_ASSERT(v[idx++] == 0x21);
    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0xBB);
    CPPUNIT_ASSERT(v[idx++] == 0xCC);
    CPPUNIT_ASSERT(v[idx++] == 0xDD);

    CPPUNIT_ASSERT(v[idx++] == 0x42);
    CPPUNIT_ASSERT(v[idx++] == 0x65);

    CPPUNIT_ASSERT(v[idx++] == 'a');
    CPPUNIT_ASSERT(v[idx++] == ' ');
    CPPUNIT_ASSERT(v[idx++] == 's');
    CPPUNIT_ASSERT(v[idx++] == 't');
    CPPUNIT_ASSERT(v[idx++] == 'r');
    CPPUNIT_ASSERT(v[idx++] == 'i');
    CPPUNIT_ASSERT(v[idx++] == 'n');
    CPPUNIT_ASSERT(v[idx++] == 'g');

    CPPUNIT_ASSERT(v[idx++] == 0x3E);
    CPPUNIT_ASSERT(v[idx++] == 0x55);

    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0x55);
    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0xCC);
    CPPUNIT_ASSERT(v[idx++] == 0xFF);

    CPPUNIT_ASSERT(v[idx++] == 0xDD);
    CPPUNIT_ASSERT(v[idx++] == 0xCC);
    CPPUNIT_ASSERT(v[idx++] == 0xBB);
    CPPUNIT_ASSERT(v[idx++] == 0xAA);
    CPPUNIT_ASSERT(v[idx++] == 0x21);
    CPPUNIT_ASSERT(v[idx++] == 0x43);
    CPPUNIT_ASSERT(v[idx++] == 0x65);
    CPPUNIT_ASSERT(v[idx++] == 0x87);
}
