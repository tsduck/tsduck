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
//  TSUnit test suite for class ts::NamesFile
//
//----------------------------------------------------------------------------

#include "tsNamesFile.h"
#include "tsNames.h"
#include "tsNullReport.h"
#include "tsFileUtils.h"
#include "tsDuckContext.h"
#include "tsCASFamily.h"
#include "tsMPEG2.h"
#include "tsAVC.h"
#include "tsCodecType.h"
#include "tsDVBAC3Descriptor.h"
#include "tsComponentDescriptor.h"
#include "tsPES.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public tsunit::Test
{
public:
    NamesTest();

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
    void testDektec();
    void testHiDes();
    void testIP();
    void testExtension();
    void testInheritance();

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
    TSUNIT_TEST(testDektec);
    TSUNIT_TEST(testHiDes);
    TSUNIT_TEST(testIP);
    TSUNIT_TEST(testExtension);
    TSUNIT_TEST(testInheritance);
    TSUNIT_TEST_END();

private:
    ts::UString _tempFileName;
};

TSUNIT_REGISTER(NamesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
NamesTest::NamesTest() :
    _tempFileName()
{
}

// Test suite initialization method.
void NamesTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".names");
    }
    ts::DeleteFile(_tempFileName, NULLREP);
}

// Test suite cleanup method.
void NamesTest::afterTest()
{
    ts::DeleteFile(_tempFileName, NULLREP);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void NamesTest::testConfigFile()
{
    const ts::NamesFile* const dtv = ts::NamesFile::Instance(ts::NamesFile::Predefined::DTV);
    debug() << "NamesTest: DTV configuration file: " << dtv->configurationFile() << std::endl;
    TSUNIT_ASSERT(!dtv->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(dtv->configurationFile()));
    TSUNIT_EQUAL(0, dtv->errorCount());

    const ts::NamesFile* const oui = ts::NamesFile::Instance(ts::NamesFile::Predefined::OUI);
    debug() << "NamesTest: OUI configuration file: " << oui->configurationFile() << std::endl;
    TSUNIT_ASSERT(!oui->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(oui->configurationFile()));
    TSUNIT_EQUAL(0, oui->errorCount());

    const ts::NamesFile* const ip = ts::NamesFile::Instance(ts::NamesFile::Predefined::IP);
    debug() << "NamesTest: IP configuration file: " << ip->configurationFile() << std::endl;
    TSUNIT_ASSERT(!ip->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(ip->configurationFile()));
    TSUNIT_EQUAL(0, ip->errorCount());

    const ts::NamesFile* const dektec = ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC);
    debug() << "NamesTest: Dektec configuration file: " << dektec->configurationFile() << std::endl;
    TSUNIT_ASSERT(!dektec->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(dektec->configurationFile()));
    TSUNIT_EQUAL(0, dektec->errorCount());

    const ts::NamesFile* const hides = ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES);
    debug() << "NamesTest: HiDes configuration file: " << hides->configurationFile() << std::endl;
    TSUNIT_ASSERT(!hides->configurationFile().empty());
    TSUNIT_ASSERT(ts::FileExists(hides->configurationFile()));
    TSUNIT_EQUAL(0, hides->errorCount());
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
    TSUNIT_EQUAL(u"SafeAccess EMM-A (0x86)", ts::names::TID(duck, ts::TID_SA_EMM_A, ts::CASID_SAFEACCESS, ts::NamesFlags::VALUE));
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

    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::VALUE));
    TSUNIT_EQUAL(u"0x00000028 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::FIRST));
    TSUNIT_EQUAL(u"40 (EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::DECIMAL_FIRST));
    TSUNIT_EQUAL(u"0x00000028 (40, EACEM/EICTA)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::FIRST | ts::NamesFlags::BOTH));
    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028, 40)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::VALUE | ts::NamesFlags::BOTH));
    TSUNIT_EQUAL(u"EACEM/EICTA (40)", ts::names::PrivateDataSpecifier(0x28, ts::NamesFlags::VALUE | ts::NamesFlags::DECIMAL));

    TSUNIT_EQUAL(u"unknown (0x00008123)", ts::names::PrivateDataSpecifier(0x8123));
    TSUNIT_EQUAL(u"unknown (33059)", ts::names::PrivateDataSpecifier(0x8123, ts::NamesFlags::DECIMAL));
    TSUNIT_EQUAL(u"33059 (unknown)", ts::names::PrivateDataSpecifier(0x8123, ts::NamesFlags::DECIMAL_FIRST));
    TSUNIT_EQUAL(u"unknown (0x00008123, 33059)", ts::names::PrivateDataSpecifier(0x8123, ts::NamesFlags::DECIMAL | ts::NamesFlags::HEXA));
}

void NamesTest::testCASFamily()
{
    TSUNIT_EQUAL(u"Other", ts::CASFamilyName(ts::CAS_OTHER));
    TSUNIT_EQUAL(u"MediaGuard", ts::CASFamilyName(ts::CAS_MEDIAGUARD));
    TSUNIT_EQUAL(u"Nagravision", ts::CASFamilyName(ts::CAS_NAGRA));
    TSUNIT_EQUAL(u"Viaccess", ts::CASFamilyName(ts::CAS_VIACCESS));
    TSUNIT_EQUAL(u"ThalesCrypt", ts::CASFamilyName(ts::CAS_THALESCRYPT));
    TSUNIT_EQUAL(u"SafeAccess", ts::CASFamilyName(ts::CAS_SAFEACCESS));
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
    TSUNIT_EQUAL(ts::MICRO_SIGN + ts::UString(u"Tech Tecnologia"), ts::NameFromOUI(0xF8E7B5));
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
    ts::DuckContext duck1;
    TSUNIT_EQUAL(u"game show/quiz/contest", ts::names::Content(duck1, 0x31));
    duck1.addStandards(ts::Standards::JAPAN);
    TSUNIT_EQUAL(u"overseas drama", ts::names::Content(duck1, 0x31));
    ts::DuckContext duck2;
    duck2.addStandards(ts::Standards::ABNT);
    TSUNIT_EQUAL(u"soap opera", ts::names::Content(duck2, 0x31));
}

void NamesTest::testDID()
{
    TSUNIT_EQUAL(u"CA", ts::names::DID(ts::DID_CA));
    TSUNIT_EQUAL(u"ISO-639 Language", ts::names::DID(ts::DID_LANGUAGE));
    TSUNIT_EQUAL(u"Data Broadcast Id", ts::names::DID(ts::DID_DATA_BROADCAST_ID));
    TSUNIT_EQUAL(u"unknown (0x83)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM));
    TSUNIT_EQUAL(u"Logical Channel Number", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM));
    TSUNIT_EQUAL(u"0x83 (Logical Channel Number)", ts::names::DID(ts::DID_LOGICAL_CHANNEL_NUM, ts::PDS_EACEM, ts::TID_NULL, ts::NamesFlags::FIRST));
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
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromDTV(u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromDTV(u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromDTV(u"pes.stream_id", 0xEC));
}

void NamesTest::testPESStartCode()
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromDTV(u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromDTV(u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromDTV(u"pes.stream_id", 0xEC));
    TSUNIT_EQUAL(u"Slice 117", ts::NameFromDTV(u"pes.stream_id", 0x75));
    TSUNIT_EQUAL(u"Sequence header", ts::NameFromDTV(u"pes.stream_id", 0xB3));
}

void NamesTest::testAspectRatio()
{
    TSUNIT_EQUAL(u"16:9", ts::NameFromDTV(u"mpeg2.aspect_ratio", ts::AR_16_9));
}

void NamesTest::testChromaFormat()
{
    TSUNIT_EQUAL(u"4:2:0", ts::NameFromDTV(u"mpeg2.chroma_format", ts::CHROMA_420));
}

void NamesTest::testAVCUnitType()
{
    TSUNIT_EQUAL(u"Picture parameter set", ts::AccessUnitTypeName(ts::CodecType::AVC, ts::AVC_AUT_PICPARAMS));
}

void NamesTest::testAVCProfile()
{
    TSUNIT_EQUAL(u"extended profile", ts::NameFromDTV(u"avc.profile", 88));
}

void NamesTest::testServiceType()
{
    TSUNIT_EQUAL(u"Data broadcast service", ts::names::ServiceType(0x0C));
    TSUNIT_EQUAL(u"unknown (0x80)", ts::names::ServiceType(128));
}

void NamesTest::testScramblingControl()
{
    TSUNIT_EQUAL(u"even", ts::NameFromDTV(u"ts.scrambling_control", 2));
}

void NamesTest::testDTSExtendedSurroundMode()
{
    TSUNIT_EQUAL(u"matrixed", ts::NameFromDTV(u"DTS_descriptor.ExtendedSurroundMode", 1));
}

void NamesTest::testDTSSurroundMode()
{
    TSUNIT_EQUAL(u"3 / C+L+R", ts::NameFromDTV(u"DTS_descriptor.SurroundMode", 5));
}

void NamesTest::testDTSBitRateCode()
{
    TSUNIT_EQUAL(u"512 kb/s", ts::NameFromDTV(u"DTS_descriptor.BitRate", 12));
}

void NamesTest::testDTSSampleRateCode()
{
    TSUNIT_EQUAL(u"22.05 kHz", ts::NameFromDTV(u"DTS_descriptor.SampleRate", 7));
}

void NamesTest::testAC3ComponentType()
{
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::DVBAC3Descriptor::ComponentTypeName(0x92));
    TSUNIT_EQUAL(u"0x92 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::DVBAC3Descriptor::ComponentTypeName(0x92, ts::NamesFlags::FIRST));
}

void NamesTest::testComponentType()
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"MPEG-2 video, 4:3 aspect ratio, 30 Hz", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0x05));
    TSUNIT_EQUAL(u"DVB subtitles, no aspect ratio", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x10));
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::ComponentDescriptor::ComponentTypeName(duck, 4, 0, 0x92));
    TSUNIT_EQUAL(u"0x0492 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::ComponentDescriptor::ComponentTypeName(duck, 4, 0, 0x92, ts::NamesFlags::FIRST));
    TSUNIT_EQUAL(u"MPEG-2 high definition video, > 16:9 aspect ratio, 30 Hz", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0x10));
    TSUNIT_EQUAL(u"MPEG-2 video", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0xB4));
    TSUNIT_EQUAL(u"0x0341 (Video is standard dynamic range (SDR))", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x41, ts::NamesFlags::FIRST));

    duck.addStandards(ts::Standards::JAPAN);
    TSUNIT_EQUAL(u"unknown (0x0110)", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0x10));
    TSUNIT_EQUAL(u"Video 1080i(1125i), >16:9 aspect ratio", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0xB4));
}

void NamesTest::testSubtitlingType()
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"DVB subtitles, high definition", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x14));
}

void NamesTest::testLinkageType()
{
    TSUNIT_EQUAL(u"data broadcast service", ts::NameFromDTV(u"linkage_descriptor.linkage_type", 0x06));
}

void NamesTest::testTeletextType()
{
    TSUNIT_EQUAL(u"Teletext subtitles", ts::NameFromDTV(u"teletext_descriptor.teletext_type", 2));
}

void NamesTest::testRunningStatus()
{
    TSUNIT_EQUAL(u"running", ts::names::RunningStatus(4));
}

void NamesTest::testAudioType()
{
    TSUNIT_EQUAL(u"hearing impaired", ts::NameFromDTV(u"ISO_639_language_descriptor.audio_type", 2));
}

void NamesTest::testT2MIPacketType()
{
    TSUNIT_EQUAL(u"Individual addressing", ts::NameFromDTV(u"t2mi.packet_type", 0x21));
}

void NamesTest::testPlatformId()
{
    TSUNIT_EQUAL(u"Horizonsat", ts::NameFromDTV(u"INT.platform_id", 10));
    TSUNIT_EQUAL(u"0x000004 (TV digitale mobile, Telecom Italia)", ts::NameFromDTV(u"INT.platform_id", 4, ts::NamesFlags::FIRST));
    TSUNIT_EQUAL(u"VTC Mobile TV (0x704001)", ts::NameFromDTV(u"INT.platform_id", 0x704001, ts::NamesFlags::VALUE));
}

void NamesTest::testDektec()
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC)->errorCount());
    TSUNIT_ASSERT(!ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC)->nameFromSection(u"DtCaps", 0).empty());
}

void NamesTest::testHiDes()
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES)->errorCount());
    TSUNIT_ASSERT(!ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES)->nameFromSection(u"HiDesErrorLinux", 0).empty());
}

void NamesTest::testIP()
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::IP)->errorCount());
    TSUNIT_ASSERT(ts::NamesFile::Instance(ts::NamesFile::Predefined::IP)->nameFromSection(u"IPProtocol", 6).startWith(u"TCP"));
}

void NamesTest::testExtension()
{
    // Create a temporary names file.
    debug() << "NamesTest::testExtension: extension file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(ts::UString::Save(ts::UStringVector({u"[CASystemId]", u"0xF123 = test-cas"}), _tempFileName));

    ts::DuckContext duck;
    ts::NamesFile::DeleteInstance(ts::NamesFile::Predefined::DTV);
    ts::NamesFile::RegisterExtensionFile reg(_tempFileName);
    TSUNIT_EQUAL(u"test-cas", ts::names::CASId(duck, 0xF123));
    ts::NamesFile::UnregisterExtensionFile(_tempFileName);
    ts::NamesFile::DeleteInstance(ts::NamesFile::Predefined::DTV);
}

void NamesTest::testInheritance()
{
    // Create a temporary names file.
    TSUNIT_ASSERT(ts::UString::Save(ts::UStringVector({
        u"[level1]",
        u"Bits = 8",
        u"1 = value1",
        u"[level2]",
        u"Bits = 8",
        u"Inherit = level1",
        u"2 = value2",
        u"[level3]",
        u"Bits = 8",
        u"Inherit = level2",
        u"3 = value3",
    }), _tempFileName));

    ts::NamesFile file(_tempFileName);

    TSUNIT_ASSERT(file.nameExists(u"level3", 3));
    TSUNIT_ASSERT(file.nameExists(u"level3", 2));
    TSUNIT_ASSERT(file.nameExists(u"level3", 1));
    TSUNIT_ASSERT(!file.nameExists(u"level3", 0));

    TSUNIT_ASSERT(!file.nameExists(u"level2", 3));
    TSUNIT_ASSERT(file.nameExists(u"level2", 2));
    TSUNIT_ASSERT(file.nameExists(u"level2", 1));
    TSUNIT_ASSERT(!file.nameExists(u"level2", 0));

    TSUNIT_ASSERT(!file.nameExists(u"level1", 3));
    TSUNIT_ASSERT(!file.nameExists(u"level1", 2));
    TSUNIT_ASSERT(file.nameExists(u"level1", 1));
    TSUNIT_ASSERT(!file.nameExists(u"level1", 0));

    TSUNIT_EQUAL(u"value3", file.nameFromSection(u"level3", 3));
    TSUNIT_EQUAL(u"value2", file.nameFromSection(u"level3", 2));
    TSUNIT_EQUAL(u"value1", file.nameFromSection(u"level3", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", file.nameFromSection(u"level3", 0));

    TSUNIT_EQUAL(u"unknown (0x03)", file.nameFromSection(u"level2", 3));
    TSUNIT_EQUAL(u"value2", file.nameFromSection(u"level2", 2));
    TSUNIT_EQUAL(u"value1", file.nameFromSection(u"level2", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", file.nameFromSection(u"level2", 0));

    TSUNIT_EQUAL(u"unknown (0x03)", file.nameFromSection(u"level1", 3));
    TSUNIT_EQUAL(u"unknown (0x02)", file.nameFromSection(u"level1", 2));
    TSUNIT_EQUAL(u"value1", file.nameFromSection(u"level1", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", file.nameFromSection(u"level1", 0));
}
