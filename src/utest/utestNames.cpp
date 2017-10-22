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
    void testDID();
    void testEDID();
    void testStreamType();
    void testStreamId();
    void testPESStartCode();
    void testPrivateDataSpecifier();
    void testCASFamily();
    void testCASId();
    void testBouquetId();
    void testOriginalNetworkId();
    void testNetworkId();
    void testDataBroadcastId();
    void testContent();
    void testOUI();
    void testAspectRatio();
    void testChromaFormat();
    void testAVCUnitType();
    void testAVCProfile();
    void testServiceType();
    void testScramblingControl();
    void testDTSExtendedSurroundMode();
    void testDTSSurroundMode();
    void testDTSBitRateCode();
    void testDTSSampleRateCode();
    void testAC3ComponentType();
    void testComponentType();
    void testSubtitlingType();
    void testLinkageType();
    void testTeletextType();
    void testRunningStatus();
    void testAudioType();

    CPPUNIT_TEST_SUITE(NamesTest);
    CPPUNIT_TEST(testConfigFile);
    CPPUNIT_TEST(testTID);
    CPPUNIT_TEST(testDID);
    CPPUNIT_TEST(testEDID);
    CPPUNIT_TEST(testStreamType);
    CPPUNIT_TEST(testStreamId);
    CPPUNIT_TEST(testPESStartCode);
    CPPUNIT_TEST(testPrivateDataSpecifier);
    CPPUNIT_TEST(testCASFamily);
    CPPUNIT_TEST(testCASId);
    CPPUNIT_TEST(testBouquetId);
    CPPUNIT_TEST(testOriginalNetworkId);
    CPPUNIT_TEST(testNetworkId);
    CPPUNIT_TEST(testDataBroadcastId);
    CPPUNIT_TEST(testContent);
    CPPUNIT_TEST(testOUI);
    CPPUNIT_TEST(testAspectRatio);
    CPPUNIT_TEST(testChromaFormat);
    CPPUNIT_TEST(testAVCUnitType);
    CPPUNIT_TEST(testAVCProfile);
    CPPUNIT_TEST(testServiceType);
    CPPUNIT_TEST(testScramblingControl);
    CPPUNIT_TEST(testDTSExtendedSurroundMode);
    CPPUNIT_TEST(testDTSSurroundMode);
    CPPUNIT_TEST(testDTSBitRateCode);
    CPPUNIT_TEST(testDTSSampleRateCode);
    CPPUNIT_TEST(testAC3ComponentType);
    CPPUNIT_TEST(testComponentType);
    CPPUNIT_TEST(testSubtitlingType);
    CPPUNIT_TEST(testLinkageType);
    CPPUNIT_TEST(testTeletextType);
    CPPUNIT_TEST(testRunningStatus);
    CPPUNIT_TEST(testAudioType);
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
    CPPUNIT_ASSERT_USTRINGS_EQUAL(ts::UString(u"Eutelsat satellite system at 4") + ts::DEGREE_SIGN + ts::UString(u"East"), ts::names::NetworkId(0x4C));
}

void NamesTest::testContent()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"game show/quiz/contest", ts::names::Content(0x31));
}

void NamesTest::testDID()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"CA", ts::names::DID(ts::DID_CA));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ISO-639 Language", ts::names::DID(ts::DID_LANGUAGE));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Data Broadcast Id", ts::names::DID(ts::DID_DATA_BROADCAST_ID));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (0x83)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Logical Channel Number", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x83 (Logical Channel Number)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM, ts::names::FIRST));
}

void NamesTest::testEDID()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"T2 Delivery System", ts::names::EDID(ts::EDID_T2_DELIVERY));
}

void NamesTest::testStreamType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"MPEG-4 Video", ts::names::StreamType(ts::ST_MPEG4_VIDEO));
}

void NamesTest::testStreamId()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ISO-13522 Hypermedia", ts::names::StreamId(ts::SID_ISO13522));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Audio 24", ts::names::StreamId(0xD8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Video 12", ts::names::StreamId(0xEC));
}

void NamesTest::testPESStartCode()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"ISO-13522 Hypermedia", ts::names::PESStartCode(ts::SID_ISO13522));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Audio 24", ts::names::PESStartCode(0xD8));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Video 12", ts::names::PESStartCode(0xEC));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Slice 117", ts::names::PESStartCode(0x75));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Sequence header", ts::names::PESStartCode(0xB3));
}

void NamesTest::testAspectRatio()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"16:9", ts::names::AspectRatio(ts::AR_16_9));
}

void NamesTest::testChromaFormat()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"4:2:0", ts::names::ChromaFormat(ts::CHROMA_420));
}

void NamesTest::testAVCUnitType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Picture parameter set", ts::names::AVCUnitType(ts::AVC_AUT_PICPARAMS));
}

void NamesTest::testAVCProfile()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"extended profile", ts::names::AVCProfile(88));
}

void NamesTest::testServiceType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Data broadcast service", ts::names::ServiceType(0x0C));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"unknown (0x00)", ts::names::ServiceType(0));
}

void NamesTest::testScramblingControl()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"even", ts::names::ScramblingControl(2));
}

void NamesTest::testDTSExtendedSurroundMode()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"matrixed", ts::names::DTSExtendedSurroundMode(1));
}

void NamesTest::testDTSSurroundMode()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"3 / C+L+R", ts::names::DTSSurroundMode(5));
}

void NamesTest::testDTSBitRateCode()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"512 kb/s", ts::names::DTSBitRateCode(12));
}

void NamesTest::testDTSSampleRateCode()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"22.05 kHz", ts::names::DTSSampleRateCode(7));
}

void NamesTest::testAC3ComponentType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::names::AC3ComponentType(0x92));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"0x92 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::names::AC3ComponentType(0x92, ts::names::FIRST));
}

void NamesTest::testComponentType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"MPEG-2 video, 4:3 aspect ratio, 30 Hz", ts::names::ComponentType(0x0105));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"DVB subtitles, no aspect ratio", ts::names::ComponentType(0x0310));
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::names::ComponentType(0x0492));
}

void NamesTest::testSubtitlingType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"DVB subtitles, high definition", ts::names::SubtitlingType(0x14));
}

void NamesTest::testLinkageType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"data broadcast service", ts::names::LinkageType(0x06));
}

void NamesTest::testTeletextType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Teletext subtitles", ts::names::TeletextType(2));
}

void NamesTest::testRunningStatus()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"running", ts::names::RunningStatus(4));
}

void NamesTest::testAudioType()
{
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"hearing impaired", ts::names::AudioType(2));
}
