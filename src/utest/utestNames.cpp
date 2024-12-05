//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::NamesFile
//
//----------------------------------------------------------------------------

#include "tsNamesFile.h"
#include "tsNames.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsDuckContext.h"
#include "tsMPEG2.h"
#include "tsAVC.h"
#include "tsStreamType.h"
#include "tsCodecType.h"
#include "tsDVBAC3Descriptor.h"
#include "tsComponentDescriptor.h"
#include "tsCAS.h"
#include "tsPES.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ConfigFile);
    TSUNIT_DECLARE_TEST(TID);
    TSUNIT_DECLARE_TEST(SharedTID);
    TSUNIT_DECLARE_TEST(DID);
    TSUNIT_DECLARE_TEST(EDID);
    TSUNIT_DECLARE_TEST(StreamType);
    TSUNIT_DECLARE_TEST(StreamId);
    TSUNIT_DECLARE_TEST(PESStartCode);
    TSUNIT_DECLARE_TEST(PrivateDataSpecifier);
    TSUNIT_DECLARE_TEST(CASFamily);
    TSUNIT_DECLARE_TEST(CASId);
    TSUNIT_DECLARE_TEST(BouquetId);
    TSUNIT_DECLARE_TEST(OriginalNetworkId);
    TSUNIT_DECLARE_TEST(NetworkId);
    TSUNIT_DECLARE_TEST(DataBroadcastId);
    TSUNIT_DECLARE_TEST(Content);
    TSUNIT_DECLARE_TEST(OUI);
    TSUNIT_DECLARE_TEST(AspectRatio);
    TSUNIT_DECLARE_TEST(ChromaFormat);
    TSUNIT_DECLARE_TEST(AVCUnitType);
    TSUNIT_DECLARE_TEST(AVCProfile);
    TSUNIT_DECLARE_TEST(ServiceType);
    TSUNIT_DECLARE_TEST(ScramblingControl);
    TSUNIT_DECLARE_TEST(DTSExtendedSurroundMode);
    TSUNIT_DECLARE_TEST(DTSSurroundMode);
    TSUNIT_DECLARE_TEST(DTSBitRateCode);
    TSUNIT_DECLARE_TEST(DTSSampleRateCode);
    TSUNIT_DECLARE_TEST(AC3ComponentType);
    TSUNIT_DECLARE_TEST(ComponentType);
    TSUNIT_DECLARE_TEST(SubtitlingType);
    TSUNIT_DECLARE_TEST(LinkageType);
    TSUNIT_DECLARE_TEST(TeletextType);
    TSUNIT_DECLARE_TEST(RunningStatus);
    TSUNIT_DECLARE_TEST(AudioType);
    TSUNIT_DECLARE_TEST(T2MIPacketType);
    TSUNIT_DECLARE_TEST(PlatformId);
    TSUNIT_DECLARE_TEST(Dektec);
    TSUNIT_DECLARE_TEST(HiDes);
    TSUNIT_DECLARE_TEST(IP);
    TSUNIT_DECLARE_TEST(Extension);
    TSUNIT_DECLARE_TEST(Inheritance);

public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

private:
    fs::path _tempFileName {};
};

TSUNIT_REGISTER(NamesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void NamesTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".names");
    }
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void NamesTest::afterTest()
{
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ConfigFile)
{
    const ts::NamesFile::NamesFilePtr dtv = ts::NamesFile::Instance(ts::NamesFile::Predefined::DTV);
    debug() << "NamesTest: DTV configuration file: " << dtv->configurationFile() << std::endl;
    TSUNIT_ASSERT(!dtv->configurationFile().empty());
    TSUNIT_ASSERT(fs::exists(dtv->configurationFile()));
    TSUNIT_EQUAL(0, dtv->errorCount());

    const ts::NamesFile::NamesFilePtr oui = ts::NamesFile::Instance(ts::NamesFile::Predefined::OUI);
    debug() << "NamesTest: OUI configuration file: " << oui->configurationFile() << std::endl;
    TSUNIT_ASSERT(!oui->configurationFile().empty());
    TSUNIT_ASSERT(fs::exists(oui->configurationFile()));
    TSUNIT_EQUAL(0, oui->errorCount());

    const ts::NamesFile::NamesFilePtr ip = ts::NamesFile::Instance(ts::NamesFile::Predefined::IP);
    debug() << "NamesTest: IP configuration file: " << ip->configurationFile() << std::endl;
    TSUNIT_ASSERT(!ip->configurationFile().empty());
    TSUNIT_ASSERT(fs::exists(ip->configurationFile()));
    TSUNIT_EQUAL(0, ip->errorCount());

    const ts::NamesFile::NamesFilePtr dektec = ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC);
    debug() << "NamesTest: Dektec configuration file: " << dektec->configurationFile() << std::endl;
    TSUNIT_ASSERT(!dektec->configurationFile().empty());
    TSUNIT_ASSERT(fs::exists(dektec->configurationFile()));
    TSUNIT_EQUAL(0, dektec->errorCount());

    const ts::NamesFile::NamesFilePtr hides = ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES);
    debug() << "NamesTest: HiDes configuration file: " << hides->configurationFile() << std::endl;
    TSUNIT_ASSERT(!hides->configurationFile().empty());
    TSUNIT_ASSERT(fs::exists(hides->configurationFile()));
    TSUNIT_EQUAL(0, hides->errorCount());
}

TSUNIT_DEFINE_TEST(TID)
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

TSUNIT_DEFINE_TEST(SharedTID)
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

TSUNIT_DEFINE_TEST(PrivateDataSpecifier)
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

TSUNIT_DEFINE_TEST(CASFamily)
{
    TSUNIT_EQUAL(u"Other", ts::CASFamilyName(ts::CAS_OTHER));
    TSUNIT_EQUAL(u"MediaGuard", ts::CASFamilyName(ts::CAS_MEDIAGUARD));
    TSUNIT_EQUAL(u"Nagravision", ts::CASFamilyName(ts::CAS_NAGRA));
    TSUNIT_EQUAL(u"Viaccess", ts::CASFamilyName(ts::CAS_VIACCESS));
    TSUNIT_EQUAL(u"ThalesCrypt", ts::CASFamilyName(ts::CAS_THALESCRYPT));
    TSUNIT_EQUAL(u"SafeAccess", ts::CASFamilyName(ts::CAS_SAFEACCESS));
}

TSUNIT_DEFINE_TEST(CASId)
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"Viaccess", ts::names::CASId(duck, 0x500));
    TSUNIT_EQUAL(u"Irdeto", ts::names::CASId(duck, 0x601));
}

TSUNIT_DEFINE_TEST(BouquetId)
{
    TSUNIT_EQUAL(ts::UString(u"T") + ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS + ts::UString(u"rk Telekom"), ts::names::BouquetId(0x55));
}

TSUNIT_DEFINE_TEST(DataBroadcastId)
{
    TSUNIT_EQUAL(u"OpenTV Data Carousel", ts::names::DataBroadcastId(0x0107));
}

TSUNIT_DEFINE_TEST(OUI)
{
    TSUNIT_EQUAL(ts::MICRO_SIGN + ts::UString(u"Tech Tecnologia"), ts::NameFromOUI(0xF8E7B5));
}

TSUNIT_DEFINE_TEST(OriginalNetworkId)
{
    TSUNIT_EQUAL(u"Skylogic", ts::names::OriginalNetworkId(0x4C));
}

TSUNIT_DEFINE_TEST(NetworkId)
{
    TSUNIT_EQUAL(ts::UString(u"Eutelsat satellite system at 4") + ts::DEGREE_SIGN + ts::UString(u"East"), ts::names::NetworkId(0x4C));
}

TSUNIT_DEFINE_TEST(Content)
{
    ts::DuckContext duck1;
    TSUNIT_EQUAL(u"game show/quiz/contest", ts::names::Content(duck1, 0x31));
    duck1.addStandards(ts::Standards::JAPAN);
    TSUNIT_EQUAL(u"overseas drama", ts::names::Content(duck1, 0x31));
    ts::DuckContext duck2;
    duck2.addStandards(ts::Standards::ABNT);
    TSUNIT_EQUAL(u"soap opera", ts::names::Content(duck2, 0x31));
}

TSUNIT_DEFINE_TEST(DID)
{
    TSUNIT_EQUAL(u"CA", ts::names::DID(ts::DID_MPEG_CA));
    TSUNIT_EQUAL(u"ISO-639 Language", ts::names::DID(ts::DID_MPEG_LANGUAGE));
    TSUNIT_EQUAL(u"Data Broadcast Id", ts::names::DID(ts::DID_DVB_DATA_BROADCAST_ID));
    TSUNIT_EQUAL(u"unknown (0x83)", ts::names::DID(ts::DID_EACEM_LCN));
    TSUNIT_EQUAL(u"Logical Channel Number", ts::names::DID(ts::DID_EACEM_LCN, ts::PDS_EACEM));
    TSUNIT_EQUAL(u"0x83 (Logical Channel Number)", ts::names::DID(ts::DID_EACEM_LCN, ts::PDS_EACEM, ts::TID_NULL, ts::NamesFlags::FIRST));
}

TSUNIT_DEFINE_TEST(EDID)
{
    TSUNIT_EQUAL(u"T2 Delivery System", ts::names::EDID(ts::EDID_DVB_T2_DELIVERY));
}

TSUNIT_DEFINE_TEST(StreamType)
{
    TSUNIT_EQUAL(u"MPEG-4 Video", ts::names::StreamType(ts::ST_MPEG4_VIDEO));
}

TSUNIT_DEFINE_TEST(StreamId)
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromDTV(u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromDTV(u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromDTV(u"pes.stream_id", 0xEC));
}

TSUNIT_DEFINE_TEST(PESStartCode)
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromDTV(u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromDTV(u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromDTV(u"pes.stream_id", 0xEC));
    TSUNIT_EQUAL(u"Slice 117", ts::NameFromDTV(u"pes.stream_id", 0x75));
    TSUNIT_EQUAL(u"Sequence header", ts::NameFromDTV(u"pes.stream_id", 0xB3));
}

TSUNIT_DEFINE_TEST(AspectRatio)
{
    TSUNIT_EQUAL(u"16:9", ts::NameFromDTV(u"mpeg2.aspect_ratio", ts::AR_16_9));
}

TSUNIT_DEFINE_TEST(ChromaFormat)
{
    TSUNIT_EQUAL(u"4:2:0", ts::NameFromDTV(u"mpeg2.chroma_format", ts::CHROMA_420));
}

TSUNIT_DEFINE_TEST(AVCUnitType)
{
    TSUNIT_EQUAL(u"Picture parameter set", ts::AccessUnitTypeName(ts::CodecType::AVC, ts::AVC_AUT_PICPARAMS));
}

TSUNIT_DEFINE_TEST(AVCProfile)
{
    TSUNIT_EQUAL(u"extended profile", ts::NameFromDTV(u"avc.profile", 88));
}

TSUNIT_DEFINE_TEST(ServiceType)
{
    TSUNIT_EQUAL(u"Data broadcast service", ts::names::ServiceType(0x0C));
    TSUNIT_EQUAL(u"unknown (0x80)", ts::names::ServiceType(128));
}

TSUNIT_DEFINE_TEST(ScramblingControl)
{
    TSUNIT_EQUAL(u"even", ts::NameFromDTV(u"ts.scrambling_control", 2));
}

TSUNIT_DEFINE_TEST(DTSExtendedSurroundMode)
{
    TSUNIT_EQUAL(u"matrixed", ts::NameFromDTV(u"DTS_descriptor.ExtendedSurroundMode", 1));
}

TSUNIT_DEFINE_TEST(DTSSurroundMode)
{
    TSUNIT_EQUAL(u"3 / C+L+R", ts::NameFromDTV(u"DTS_descriptor.SurroundMode", 5));
}

TSUNIT_DEFINE_TEST(DTSBitRateCode)
{
    TSUNIT_EQUAL(u"512 kb/s", ts::NameFromDTV(u"DTS_descriptor.BitRate", 12));
}

TSUNIT_DEFINE_TEST(DTSSampleRateCode)
{
    TSUNIT_EQUAL(u"22.05 kHz", ts::NameFromDTV(u"DTS_descriptor.SampleRate", 7));
}

TSUNIT_DEFINE_TEST(AC3ComponentType)
{
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::DVBAC3Descriptor::ComponentTypeName(0x92));
    TSUNIT_EQUAL(u"0x92 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::DVBAC3Descriptor::ComponentTypeName(0x92, ts::NamesFlags::FIRST));
}

TSUNIT_DEFINE_TEST(ComponentType)
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

TSUNIT_DEFINE_TEST(SubtitlingType)
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"DVB subtitles, high definition", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x14));
}

TSUNIT_DEFINE_TEST(LinkageType)
{
    TSUNIT_EQUAL(u"data broadcast service", ts::NameFromDTV(u"linkage_descriptor.linkage_type", 0x06));
}

TSUNIT_DEFINE_TEST(TeletextType)
{
    TSUNIT_EQUAL(u"Teletext subtitles", ts::NameFromDTV(u"teletext_descriptor.teletext_type", 2));
}

TSUNIT_DEFINE_TEST(RunningStatus)
{
    TSUNIT_EQUAL(u"running", ts::names::RunningStatus(4));
}

TSUNIT_DEFINE_TEST(AudioType)
{
    TSUNIT_EQUAL(u"hearing impaired", ts::NameFromDTV(u"ISO_639_language_descriptor.audio_type", 2));
}

TSUNIT_DEFINE_TEST(T2MIPacketType)
{
    TSUNIT_EQUAL(u"Individual addressing", ts::NameFromDTV(u"t2mi.packet_type", 0x21));
}

TSUNIT_DEFINE_TEST(PlatformId)
{
    TSUNIT_EQUAL(u"Horizonsat", ts::NameFromDTV(u"INT.platform_id", 10));
    TSUNIT_EQUAL(u"0x000004 (TV digitale mobile, Telecom Italia)", ts::NameFromDTV(u"INT.platform_id", 4, ts::NamesFlags::FIRST));
    TSUNIT_EQUAL(u"VTC Mobile TV (0x704001)", ts::NameFromDTV(u"INT.platform_id", 0x704001, ts::NamesFlags::VALUE));
}

TSUNIT_DEFINE_TEST(Dektec)
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC)->errorCount());
    TSUNIT_ASSERT(!ts::NamesFile::Instance(ts::NamesFile::Predefined::DEKTEC)->nameFromSection(u"DtCaps", 0).empty());
}

TSUNIT_DEFINE_TEST(HiDes)
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES)->errorCount());
    TSUNIT_ASSERT(!ts::NamesFile::Instance(ts::NamesFile::Predefined::HIDES)->nameFromSection(u"HiDesErrorLinux", 0).empty());
}

TSUNIT_DEFINE_TEST(IP)
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_EQUAL(0, ts::NamesFile::Instance(ts::NamesFile::Predefined::IP)->errorCount());
    TSUNIT_ASSERT(ts::NamesFile::Instance(ts::NamesFile::Predefined::IP)->nameFromSection(u"IPProtocol", 6).startWith(u"TCP"));
}

TSUNIT_DEFINE_TEST(Extension)
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

TSUNIT_DEFINE_TEST(Inheritance)
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
