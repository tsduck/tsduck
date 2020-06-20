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
//  TSUnit test suite for class ts::Names
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsMPEG.h"
#include "tsSysUtils.h"
#include "tsDuckContext.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testConfigFile();
    void testTID();
    void testSharedTID();
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
    void testT2MIPacketType();
    void testPlatformId();

    TSUNIT_TEST_BEGIN(NamesTest);
    TSUNIT_TEST(testConfigFile);
    TSUNIT_TEST(testTID);
    TSUNIT_TEST(testSharedTID);
    TSUNIT_TEST(testDID);
    TSUNIT_TEST(testEDID);
    TSUNIT_TEST(testStreamType);
    TSUNIT_TEST(testStreamId);
    TSUNIT_TEST(testPESStartCode);
    TSUNIT_TEST(testPrivateDataSpecifier);
    TSUNIT_TEST(testCASFamily);
    TSUNIT_TEST(testCASId);
    TSUNIT_TEST(testBouquetId);
    TSUNIT_TEST(testOriginalNetworkId);
    TSUNIT_TEST(testNetworkId);
    TSUNIT_TEST(testDataBroadcastId);
    TSUNIT_TEST(testContent);
    TSUNIT_TEST(testOUI);
    TSUNIT_TEST(testAspectRatio);
    TSUNIT_TEST(testChromaFormat);
    TSUNIT_TEST(testAVCUnitType);
    TSUNIT_TEST(testAVCProfile);
    TSUNIT_TEST(testServiceType);
    TSUNIT_TEST(testScramblingControl);
    TSUNIT_TEST(testDTSExtendedSurroundMode);
    TSUNIT_TEST(testDTSSurroundMode);
    TSUNIT_TEST(testDTSBitRateCode);
    TSUNIT_TEST(testDTSSampleRateCode);
    TSUNIT_TEST(testAC3ComponentType);
    TSUNIT_TEST(testComponentType);
    TSUNIT_TEST(testSubtitlingType);
    TSUNIT_TEST(testLinkageType);
    TSUNIT_TEST(testTeletextType);
    TSUNIT_TEST(testRunningStatus);
    TSUNIT_TEST(testAudioType);
    TSUNIT_TEST(testT2MIPacketType);
    TSUNIT_TEST(testPlatformId);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(NamesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void NamesTest::beforeTest()
{
}

// Test suite cleanup method.
void NamesTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void NamesTest::testConfigFile()
{
    debug() << "NamesTest: DVB configuration file: " << ts::NamesMain::Instance()->configurationFile() << std::endl
            << "NamesTest: OUI configuration file: " << ts::NamesOUI::Instance()->configurationFile() << std::endl;

    TSUNIT_ASSERT(!ts::NamesMain::Instance()->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(ts::NamesMain::Instance()->configurationFile()));
    TSUNIT_EQUAL(0, ts::NamesMain::Instance()->errorCount());

    TSUNIT_ASSERT(!ts::NamesOUI::Instance()->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(ts::NamesOUI::Instance()->configurationFile()));
    TSUNIT_EQUAL(0, ts::NamesOUI::Instance()->errorCount());
}

void NamesTest::testTID()
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"CAT", ts::names::TID(duck, ts::TID_CAT));
    TSUNIT_EQUAL(u"CAT", ts::names::TID(duck, ts::TID_CAT, ts::CASID_NAGRA_MIN));
    TSUNIT_EQUAL(u"PMT", ts::names::TID(duck, ts::TID_PMT, ts::CASID_VIACCESS_MIN));
    TSUNIT_EQUAL(u"Viaccess EMM-U", ts::names::TID(duck, ts::TID_VIA_EMM_U, ts::CASID_VIACCESS_MIN));
    TSUNIT_EQUAL(u"EIT schedule Actual", ts::names::TID(duck, ts::TID_EIT_S_ACT_MIN + 4));
    TSUNIT_EQUAL(u"ECM (odd)", ts::names::TID(duck, ts::TID_ECM_81));
    TSUNIT_EQUAL(u"Nagravision ECM (odd)", ts::names::TID(duck, ts::TID_ECM_81, ts::CASID_NAGRA_MIN));
    TSUNIT_EQUAL(u"SafeAccess EMM-A (0x86)", ts::names::TID(duck, ts::TID_SA_EMM_A, ts::CASID_SAFEACCESS, ts::names::VALUE));
    TSUNIT_EQUAL(u"Logiways DMT", ts::names::TID(duck, ts::TID_LW_DMT, ts::CASID_SAFEACCESS));
    TSUNIT_EQUAL(u"unknown (0x90)", ts::names::TID(duck, ts::TID_LW_DMT));
}

void NamesTest::testSharedTID()
{
    // Shared table ids between ATSC and ISDB.

    ts::DuckContext duck;
    TSUNIT_EQUAL(ts::TID_MGT, ts::TID_LDT);
    TSUNIT_EQUAL(ts::TID_TVCT, ts::TID_CDT);

    duck.addStandards(ts::Standards::ISDB);
    TSUNIT_EQUAL(u"LDT (ISDB)", ts::names::TID(duck, ts::TID_MGT));
    TSUNIT_EQUAL(u"CDT (ISDB)", ts::names::TID(duck, ts::TID_TVCT));

    duck.resetStandards(ts::Standards::ATSC);
    TSUNIT_EQUAL(u"MGT (ATSC)", ts::names::TID(duck, ts::TID_MGT));
    TSUNIT_EQUAL(u"TVCT (ATSC)", ts::names::TID(duck, ts::TID_TVCT));
}

void NamesTest::testPrivateDataSpecifier()
{
    const ts::UString tdfRef = ts::UString(u"T") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"l") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"diffusion de France (TDF)");

    TSUNIT_EQUAL(u"EACEM/EICTA", ts::names::PrivateDataSpecifier(0x28));
    TSUNIT_EQUAL(tdfRef, ts::names::PrivateDataSpecifier(0x1A));

    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE));
    TSUNIT_EQUAL(u"0x00000028 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::FIRST));
    TSUNIT_EQUAL(u"40 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::DECIMAL_FIRST));
    TSUNIT_EQUAL(u"0x00000028 (40, EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::names::FIRST | ts::names::BOTH));
    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028, 40)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE | ts::names::BOTH));
    TSUNIT_EQUAL(u"EACEM/EICTA (40)", ts::names::PrivateDataSpecifier(0x28, ts::names::VALUE | ts::names::DECIMAL));

    TSUNIT_EQUAL(u"unknown (0x00008123)", ts::names::PrivateDataSpecifier(0x8123));
    TSUNIT_EQUAL(u"unknown (33059)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL));
    TSUNIT_EQUAL(u"33059 (unknown)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL_FIRST));
    TSUNIT_EQUAL(u"unknown (0x00008123, 33059)", ts::names::PrivateDataSpecifier(0x8123, ts::names::DECIMAL | ts::names::HEXA));
}

void NamesTest::testCASFamily()
{
    TSUNIT_EQUAL(u"Other", ts::names::CASFamily(ts::CAS_OTHER));
    TSUNIT_EQUAL(u"MediaGuard", ts::names::CASFamily(ts::CAS_MEDIAGUARD));
    TSUNIT_EQUAL(u"Nagravision", ts::names::CASFamily(ts::CAS_NAGRA));
    TSUNIT_EQUAL(u"Viaccess", ts::names::CASFamily(ts::CAS_VIACCESS));
    TSUNIT_EQUAL(u"ThalesCrypt", ts::names::CASFamily(ts::CAS_THALESCRYPT));
    TSUNIT_EQUAL(u"SafeAccess", ts::names::CASFamily(ts::CAS_SAFEACCESS));
}

void NamesTest::testCASId()
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"Viaccess", ts::names::CASId(duck, 0x500));
    TSUNIT_EQUAL(u"Irdeto", ts::names::CASId(duck, 0x601));
}

void NamesTest::testBouquetId()
{
    TSUNIT_EQUAL(ts::UString(u"T") + ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS + ts::UString(u"rk Telekom"), ts::names::BouquetId(0x55));
}

void NamesTest::testDataBroadcastId()
{
    TSUNIT_EQUAL(u"OpenTV Data Carousel", ts::names::DataBroadcastId(0x0107));
}

void NamesTest::testOUI()
{
    TSUNIT_EQUAL(ts::MICRO_SIGN + ts::UString(u"Tech Tecnologia"), ts::names::OUI(0xF8E7B5));
}

void NamesTest::testOriginalNetworkId()
{
    TSUNIT_EQUAL(u"Skylogic", ts::names::OriginalNetworkId(0x4C));
}

void NamesTest::testNetworkId()
{
    TSUNIT_EQUAL(ts::UString(u"Eutelsat satellite system at 4") + ts::DEGREE_SIGN + ts::UString(u"East"), ts::names::NetworkId(0x4C));
}

void NamesTest::testContent()
{
    TSUNIT_EQUAL(u"game show/quiz/contest", ts::names::Content(0x31));
}

void NamesTest::testDID()
{
    TSUNIT_EQUAL(u"CA", ts::names::DID(ts::DID_CA));
    TSUNIT_EQUAL(u"ISO-639 Language", ts::names::DID(ts::DID_LANGUAGE));
    TSUNIT_EQUAL(u"Data Broadcast Id", ts::names::DID(ts::DID_DATA_BROADCAST_ID));
    TSUNIT_EQUAL(u"unknown (0x83)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM));
    TSUNIT_EQUAL(u"Logical Channel Number", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM));
    TSUNIT_EQUAL(u"0x83 (Logical Channel Number)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM, ts::TID_NULL, ts::names::FIRST));
}

void NamesTest::testEDID()
{
    TSUNIT_EQUAL(u"T2 Delivery System", ts::names::EDID(ts::EDID_T2_DELIVERY));
}

void NamesTest::testStreamType()
{
    TSUNIT_EQUAL(u"MPEG-4 Video", ts::names::StreamType(ts::ST_MPEG4_VIDEO));
}

void NamesTest::testStreamId()
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::names::StreamId(ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::names::StreamId(0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::names::StreamId(0xEC));
}

void NamesTest::testPESStartCode()
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::names::PESStartCode(ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::names::PESStartCode(0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::names::PESStartCode(0xEC));
    TSUNIT_EQUAL(u"Slice 117", ts::names::PESStartCode(0x75));
    TSUNIT_EQUAL(u"Sequence header", ts::names::PESStartCode(0xB3));
}

void NamesTest::testAspectRatio()
{
    TSUNIT_EQUAL(u"16:9", ts::names::AspectRatio(ts::AR_16_9));
}

void NamesTest::testChromaFormat()
{
    TSUNIT_EQUAL(u"4:2:0", ts::names::ChromaFormat(ts::CHROMA_420));
}

void NamesTest::testAVCUnitType()
{
    TSUNIT_EQUAL(u"Picture parameter set", ts::names::AVCUnitType(ts::AVC_AUT_PICPARAMS));
}

void NamesTest::testAVCProfile()
{
    TSUNIT_EQUAL(u"extended profile", ts::names::AVCProfile(88));
}

void NamesTest::testServiceType()
{
    TSUNIT_EQUAL(u"Data broadcast service", ts::names::ServiceType(0x0C));
    TSUNIT_EQUAL(u"unknown (0x80)", ts::names::ServiceType(128));
}

void NamesTest::testScramblingControl()
{
    TSUNIT_EQUAL(u"even", ts::names::ScramblingControl(2));
}

void NamesTest::testDTSExtendedSurroundMode()
{
    TSUNIT_EQUAL(u"matrixed", ts::names::DTSExtendedSurroundMode(1));
}

void NamesTest::testDTSSurroundMode()
{
    TSUNIT_EQUAL(u"3 / C+L+R", ts::names::DTSSurroundMode(5));
}

void NamesTest::testDTSBitRateCode()
{
    TSUNIT_EQUAL(u"512 kb/s", ts::names::DTSBitRateCode(12));
}

void NamesTest::testDTSSampleRateCode()
{
    TSUNIT_EQUAL(u"22.05 kHz", ts::names::DTSSampleRateCode(7));
}

void NamesTest::testAC3ComponentType()
{
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::names::AC3ComponentType(0x92));
    TSUNIT_EQUAL(u"0x92 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::names::AC3ComponentType(0x92, ts::names::FIRST));
}

void NamesTest::testComponentType()
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"MPEG-2 video, 4:3 aspect ratio, 30 Hz", ts::names::ComponentType(duck, 0x0105));
    TSUNIT_EQUAL(u"DVB subtitles, no aspect ratio", ts::names::ComponentType(duck, 0x0310));
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::names::ComponentType(duck, 0x0492));
    TSUNIT_EQUAL(u"MPEG-2 high definition video, > 16:9 aspect ratio, 30 Hz", ts::names::ComponentType(duck, 0x0110));
    TSUNIT_EQUAL(u"MPEG-2 video", ts::names::ComponentType(duck, 0x01B4));

    duck.addStandards(ts::Standards::JAPAN);
    TSUNIT_EQUAL(u"unknown (0x0110)", ts::names::ComponentType(duck, 0x0110));
    TSUNIT_EQUAL(u"Video 1080i(1125i), >16:9 aspect ratio", ts::names::ComponentType(duck, 0x01B4));
}

void NamesTest::testSubtitlingType()
{
    TSUNIT_EQUAL(u"DVB subtitles, high definition", ts::names::SubtitlingType(0x14));
}

void NamesTest::testLinkageType()
{
    TSUNIT_EQUAL(u"data broadcast service", ts::names::LinkageType(0x06));
}

void NamesTest::testTeletextType()
{
    TSUNIT_EQUAL(u"Teletext subtitles", ts::names::TeletextType(2));
}

void NamesTest::testRunningStatus()
{
    TSUNIT_EQUAL(u"running", ts::names::RunningStatus(4));
}

void NamesTest::testAudioType()
{
    TSUNIT_EQUAL(u"hearing impaired", ts::names::AudioType(2));
}

void NamesTest::testT2MIPacketType()
{
    TSUNIT_EQUAL(u"Individual addressing", ts::names::T2MIPacketType(0x21));
}

void NamesTest::testPlatformId()
{
    TSUNIT_EQUAL(u"Horizonsat", ts::names::PlatformId(10));
    TSUNIT_EQUAL(u"0x000004 (TV digitale mobile, Telecom Italia)", ts::names::PlatformId(4, ts::names::FIRST));
    TSUNIT_EQUAL(u"VTC Mobile TV (0x704001)", ts::names::PlatformId(0x704001, ts::names::VALUE));
}
