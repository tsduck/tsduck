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
//  CppUnit test suite for class ts::Names
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsMPEG.h"
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testConfigFile();
    void testTID();
    void testPrivateDataSpecifier();
    void testCASFamily();
    void testCASId();
    void testBouquetId();
    void testOriginalNetworkId();
    void testNetworkId();
    void testDataBroadcastId();
    void testContent();
    void testOUI();

    CPPUNIT_TEST_SUITE(NamesTest);
    CPPUNIT_TEST(testConfigFile);
    CPPUNIT_TEST(testTID);
    CPPUNIT_TEST(testPrivateDataSpecifier);
    CPPUNIT_TEST(testCASFamily);
    CPPUNIT_TEST(testCASId);
    CPPUNIT_TEST(testBouquetId);
    CPPUNIT_TEST(testOriginalNetworkId);
    CPPUNIT_TEST(testNetworkId);
    CPPUNIT_TEST(testDataBroadcastId);
    CPPUNIT_TEST(testContent);
    CPPUNIT_TEST(testOUI);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(NamesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void NamesTest::setUp()
{
}

// Test suite cleanup method.
void NamesTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void NamesTest::testConfigFile()
{
    utest::Out() << "NamesTest: DVB configuration file: " << ts::NamesDVB::Instance().configurationFile() << std::endl
                 << "NamesTest: OUI configuration file: " << ts::NamesOUI::Instance().configurationFile() << std::endl;

    CPPUNIT_ASSERT(!ts::NamesDVB::Instance().configurationFile().empty());
    CPPUNIT_ASSERT(ts::FileExists(ts::NamesDVB::Instance().configurationFile()));
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::NamesDVB::Instance().errorCount());

    CPPUNIT_ASSERT(!ts::NamesOUI::Instance().configurationFile().empty());
    CPPUNIT_ASSERT(ts::FileExists(ts::NamesOUI::Instance().configurationFile()));
    CPPUNIT_ASSERT_EQUAL(size_t(0), ts::NamesOUI::Instance().errorCount());
}

void NamesTest::testTID()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"CAT", ts::names::TID(ts::TID_CAT));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"CAT", ts::names::TID(ts::TID_CAT, ts::CAS_NAGRA));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"PMT", ts::names::TID(ts::TID_PMT, ts::CAS_VIACCESS));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Viaccess EMM-U", ts::names::TID(ts::TID_VIA_EMM_U, ts::CAS_VIACCESS));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"EIT schedule Actual", ts::names::TID(ts::TID_EIT_S_ACT_MIN + 4));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ECM (odd)", ts::names::TID(ts::TID_ECM_81));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Nagravision ECM (odd)", ts::names::TID(ts::TID_ECM_81, ts::CAS_NAGRA));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"SafeAccess EMM-A (0x86)", ts::names::TID(ts::TID_SA_EMM_A, ts::CAS_SAFEACCESS, ts::names::VALUE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Logiways DMT", ts::names::TID(ts::TID_LW_DMT, ts::CAS_SAFEACCESS));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (0x90)", ts::names::TID(ts::TID_LW_DMT));
}

void NamesTest::testPrivateDataSpecifier()
{
    const ts::UString tdfRef = ts::UString(u"T") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"l") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"diffusion de France (TDF)");

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"EACEM/EICTA", ts::names::PrivateDataSpecifier(0x28));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(tdfRef, ts::names::PrivateDataSpecifier(0x1A));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"EACEM/EICTA (0x00000028)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x00000028 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::FIRST));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"40 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::DECIMAL_FIRST));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x00000028 (40, EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::FIRST | ts::names::BOTH));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"EACEM/EICTA (0x00000028, 40)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE | ts::names::BOTH));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"EACEM/EICTA (40)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE | ts::names::DECIMAL));

    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (0x00008123)", ts::names::PrivateDataSpecifier(0x8123));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (33059)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"33059 (unknown)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL_FIRST));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (0x00008123, 33059)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL | ts::names::HEXA));
}

void NamesTest::testCASFamily()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Other", ts::names::CASFamily(ts::CAS_OTHER));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"MediaGuard", ts::names::CASFamily(ts::CAS_MEDIAGUARD));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Nagravision", ts::names::CASFamily(ts::CAS_NAGRA));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Viaccess", ts::names::CASFamily(ts::CAS_VIACCESS));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ThalesCrypt", ts::names::CASFamily(ts::CAS_THALESCRYPT));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"SafeAccess", ts::names::CASFamily(ts::CAS_SAFEACCESS));
}

void NamesTest::testCASId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Viaccess", ts::names::CASId(0x500));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Irdeto", ts::names::CASId(0x601));
}

void NamesTest::testBouquetId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString(u"T") + ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS + ts::UString(u"rk Telekom"), ts::names::BouquetId(0x55));
}

void NamesTest::testDataBroadcastId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"OpenTV Data Carousel", ts::names::DataBroadcastId(0x0107));
}

void NamesTest::testOUI()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::MICRO_SIGN + ts::UString(u"Tech Tecnologia"), ts::names::OUI(0xF8E7B5));
}

void NamesTest::testOriginalNetworkId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Skylogic", ts::names::OriginalNetworkId(0x4C));
}

void NamesTest::testNetworkId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Eutelsat satellite system at 4°East", ts::names::NetworkId(0x4C));
}

void NamesTest::testContent()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"game show/quiz/contest", ts::names::Content(0x31));
}
