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
//  TSUnit test suite for DVBCharset and subclasses.
//
//----------------------------------------------------------------------------

#include "tsDVBCharset.h"
#include "tsByteBlock.h"
#include "tsunit.h"

//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DVBCharsetTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRepository();
    void testDVB();

    TSUNIT_TEST_BEGIN(DVBCharsetTest);
    TSUNIT_TEST(testRepository);
    TSUNIT_TEST(testDVB);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(DVBCharsetTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DVBCharsetTest::beforeTest()
{
}

// Test suite cleanup method.
void DVBCharsetTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void DVBCharsetTest::testRepository()
{
    debug() << "DVBCharsetTest::testRepository: charsets: " << ts::UString::Join(ts::Charset::GetAllNames()) << std::endl;
    TSUNIT_EQUAL(38, ts::Charset::GetAllNames().size());

    // ARIB, ARIB-STD-B24,
    // DVB, ISO-6937, UNICODE, UTF-8,
    // ISO-8859-1, ISO-8859-2, ISO-8859-3, ISO-8859-4, ISO-8859-5, ISO-8859-6, ISO-8859-7, ISO-8859-8, ISO-8859-9,
    // ISO-8859-10, ISO-8859-11, ISO-8859-13, ISO-8859-14, ISO-8859-15,
    // RAW-ISO-6937,
    // RAW-ISO-8859-1, RAW-ISO-8859-2, RAW-ISO-8859-3, RAW-ISO-8859-4, RAW-ISO-8859-5, RAW-ISO-8859-6, RAW-ISO-8859-7, RAW-ISO-8859-8, RAW-ISO-8859-9,
    // RAW-ISO-8859-10, RAW-ISO-8859-11, RAW-ISO-8859-13, RAW-ISO-8859-14, RAW-ISO-8859-15,
    // RAW-UNICODE, RAW-UTF-8
    // DUMP
}

void DVBCharsetTest::testDVB()
{
    static const char s1[]= "abCD 89#()";
    TSUNIT_EQUAL(u"abCD 89#()", ts::DVBCharset::DVB.decoded(reinterpret_cast<const uint8_t*>(s1), ::strlen(s1)));

    static const uint8_t dvb1[] = {0x30, 0xC2, 0x65, 0xC3, 0x75};
    const ts::UString str1{u'0', ts::LATIN_SMALL_LETTER_E_WITH_ACUTE, ts::LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX};
    TSUNIT_EQUAL(str1, ts::DVBCharset::DVB.decoded(dvb1, sizeof(dvb1)));
    TSUNIT_ASSERT(ts::ByteBlock(dvb1, sizeof(dvb1)) == ts::DVBCharset::DVB.encoded(str1.toDecomposedDiacritical()));
}
