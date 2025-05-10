//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(Repository);
    TSUNIT_DECLARE_TEST(DVB);
};

TSUNIT_REGISTER(DVBCharsetTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Repository)
{
    debug() << "DVBCharsetTest::testRepository: charsets: " << ts::UString::Join(ts::Charset::GetAllNames()) << std::endl;
    TSUNIT_EQUAL(40, ts::Charset::GetAllNames().size());

    // ARIB, ARIB-STD-B24,
    // DVB, ISO-6937, UNICODE, UTF-16, UTF-8,
    // ISO-8859-1, ISO-8859-2, ISO-8859-3, ISO-8859-4, ISO-8859-5, ISO-8859-6, ISO-8859-7, ISO-8859-8, ISO-8859-9,
    // ISO-8859-10, ISO-8859-11, ISO-8859-13, ISO-8859-14, ISO-8859-15,
    // RAW-ISO-6937,
    // RAW-ISO-8859-1, RAW-ISO-8859-2, RAW-ISO-8859-3, RAW-ISO-8859-4, RAW-ISO-8859-5, RAW-ISO-8859-6, RAW-ISO-8859-7, RAW-ISO-8859-8, RAW-ISO-8859-9,
    // RAW-ISO-8859-10, RAW-ISO-8859-11, RAW-ISO-8859-13, RAW-ISO-8859-14, RAW-ISO-8859-15,
    // RAW-UNICODE, RAW-UTF-16, RAW-UTF-8
    // DUMP
}

TSUNIT_DEFINE_TEST(DVB)
{
    static const char s1[]= "abCD 89#()";
    TSUNIT_EQUAL(u"abCD 89#()", ts::DVBCharset::DVB.decoded(reinterpret_cast<const uint8_t*>(s1), std::strlen(s1)));

    static const uint8_t dvb1[] = {0x30, 0xC2, 0x65, 0xC3, 0x75};
    const ts::UString str1{u'0', ts::LATIN_SMALL_LETTER_E_WITH_ACUTE, ts::LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX};
    TSUNIT_EQUAL(str1, ts::DVBCharset::DVB.decoded(dvb1, sizeof(dvb1)));
    TSUNIT_ASSERT(ts::ByteBlock(dvb1, sizeof(dvb1)) == ts::DVBCharset::DVB.encoded(str1.toDecomposedDiacritical()));
}
