//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::Names
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsFileUtils.h"
#include "tsDuckContext.h"
#include "tsOUI.h"
#include "tsMPEG2.h"
#include "tsAVC.h"
#include "tsStreamType.h"
#include "tsCodecType.h"
#include "tsPES.h"
#include "tsDVB.h"
#include "tsDescriptorList.h"
#include "tsDVBAC3Descriptor.h"
#include "tsComponentDescriptor.h"
#include "tsRegistrationDescriptor.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class NamesTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Name);
    TSUNIT_DECLARE_TEST(Names);
    TSUNIT_DECLARE_TEST(Value);
    TSUNIT_DECLARE_TEST(Unique);
    TSUNIT_DECLARE_TEST(NameList);
    TSUNIT_DECLARE_TEST(Error);
    TSUNIT_DECLARE_TEST(OUI);
    TSUNIT_DECLARE_TEST(Dektec);
    TSUNIT_DECLARE_TEST(HiDes);
    TSUNIT_DECLARE_TEST(IP);
    TSUNIT_DECLARE_TEST(TID);
    TSUNIT_DECLARE_TEST(SharedTID);
    TSUNIT_DECLARE_TEST(DID);
    TSUNIT_DECLARE_TEST(XDID);
    TSUNIT_DECLARE_TEST(StreamType);
    TSUNIT_DECLARE_TEST(PDS);
    TSUNIT_DECLARE_TEST(REGID);
    TSUNIT_DECLARE_TEST(CASFamily);
    TSUNIT_DECLARE_TEST(CASId);
    TSUNIT_DECLARE_TEST(BouquetId);
    TSUNIT_DECLARE_TEST(OriginalNetworkId);
    TSUNIT_DECLARE_TEST(NetworkId);
    TSUNIT_DECLARE_TEST(DataBroadcastId);
    TSUNIT_DECLARE_TEST(Content);
    TSUNIT_DECLARE_TEST(ServiceType);
    TSUNIT_DECLARE_TEST(ComponentType);
    TSUNIT_DECLARE_TEST(SubtitlingType);
    TSUNIT_DECLARE_TEST(LinkageType);
    TSUNIT_DECLARE_TEST(TeletextType);
    TSUNIT_DECLARE_TEST(RunningStatus);
    TSUNIT_DECLARE_TEST(StreamId);
    TSUNIT_DECLARE_TEST(PESStartCode);
    TSUNIT_DECLARE_TEST(AspectRatio);
    TSUNIT_DECLARE_TEST(ChromaFormat);
    TSUNIT_DECLARE_TEST(AVCUnitType);
    TSUNIT_DECLARE_TEST(AVCProfile);
    TSUNIT_DECLARE_TEST(AC3ComponentType);
    TSUNIT_DECLARE_TEST(ScramblingControl);
    TSUNIT_DECLARE_TEST(DTSExtendedSurroundMode);
    TSUNIT_DECLARE_TEST(DTSSurroundMode);
    TSUNIT_DECLARE_TEST(DTSBitRateCode);
    TSUNIT_DECLARE_TEST(DTSSampleRateCode);
    TSUNIT_DECLARE_TEST(AudioType);
    TSUNIT_DECLARE_TEST(T2MIPacketType);
    TSUNIT_DECLARE_TEST(PlatformId);
    TSUNIT_DECLARE_TEST(Inheritance);
    TSUNIT_DECLARE_TEST(Extension);

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
    _tempFileName = ts::TempFile(u".names");
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

TSUNIT_DEFINE_TEST(Name)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    TSUNIT_ASSERT(!e1.empty());
    TSUNIT_EQUAL(u"FirstElement", e1.name(-1));
    TSUNIT_EQUAL(u"SecondElement", e1.name(7));
    TSUNIT_EQUAL(u"FirstRepetition", e1.name(47));
    TSUNIT_EQUAL(u"OtherValue", e1.name(-123));
    TSUNIT_EQUAL(u"AddedElement", e1.name(458));

    e1.add(u"Other7", 7);
    const ts::UString v7(e1.name(7));
    TSUNIT_ASSERT(v7 == u"SecondElement" || v7 == u"Other7");
}

TSUNIT_DEFINE_TEST(Names)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    std::vector<int> vec;
    TSUNIT_EQUAL(u"", e1.names(vec));

    vec.push_back(7);
    TSUNIT_EQUAL(u"SecondElement", e1.names(vec));

    vec.push_back(458);
    TSUNIT_EQUAL(u"SecondElement, AddedElement", e1.names(vec));

    vec.push_back(432);
    TSUNIT_EQUAL(u"SecondElement, AddedElement, 432", e1.names(vec));
}

TSUNIT_DEFINE_TEST(Value)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    TSUNIT_ASSERT(e1.value(u"FirstElement") == -1);
    TSUNIT_ASSERT(e1.value(u"SecondElement") == 7);
    TSUNIT_ASSERT(e1.value(u"FirstRepetition") == 47);
    TSUNIT_ASSERT(e1.value(u"OtherValue") == -123);
    TSUNIT_ASSERT(e1.value(u"AddedElement") == 458);

    TSUNIT_ASSERT(e1.value(u"FirstElement", true) == -1);
    TSUNIT_ASSERT(e1.value(u"FirstElement", false) == -1);
    TSUNIT_ASSERT(e1.value(u"firste") == ts::Names::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", true) == ts::Names::UNKNOWN);
    TSUNIT_ASSERT(e1.value(u"firste", false) == -1);

    TSUNIT_ASSERT(e1.value(u"FirstElem") == -1);
    TSUNIT_ASSERT(e1.value(u"FirstE") == -1);
    TSUNIT_ASSERT(e1.value(u"First") == ts::Names::UNKNOWN);

    e1.add(u"FirstRepetition", 48);

    const ts::Names::int_t vFirstRepetition = e1.value(u"FirstRepetition");
    TSUNIT_ASSERT(vFirstRepetition == 47 || vFirstRepetition == 48);

    TSUNIT_ASSERT(e1.value(u"1") == 1);
    TSUNIT_ASSERT(e1.value(u"-1234") == -1234);
    TSUNIT_ASSERT(e1.value(u"0x10") == 16);
    TSUNIT_ASSERT(e1.value(u"x10") == ts::Names::UNKNOWN);
}

TSUNIT_DEFINE_TEST(Unique)
{
    ts::Names e1;
    e1.add(u"foo", 10, 1'000);
    TSUNIT_ASSERT(e1.freeRange(0, 9));
    TSUNIT_ASSERT(e1.freeRange(1'001, 2'000));
    TSUNIT_ASSERT(!e1.freeRange(2, 10));
    TSUNIT_ASSERT(!e1.freeRange(2, 90));
    TSUNIT_ASSERT(!e1.freeRange(990, 2'000));
    TSUNIT_ASSERT(!e1.freeRange(1000, 2'000));
    TSUNIT_ASSERT(!e1.freeRange(2, 10'000));
    TSUNIT_EQUAL(1'001, e1.addNewValue(u"bar"));
    TSUNIT_ASSERT(!e1.freeRange(1'001, 2'000));

    ts::Names e2;
    e2.add(u"foo", 2, std::numeric_limits<ts::Names::int_t>::max());

    auto newval = e2.addNewValue(u"n1");
    debug() << "NamesTest::Unique: newval = " << newval << std::endl;
    TSUNIT_ASSERT(newval != ts::Names::UNKNOWN);
    TSUNIT_ASSERT(newval >= 0);

    newval = e2.addNewValue(u"n2");
    debug() << "NamesTest::Unique: newval = " << newval << std::endl;
    TSUNIT_ASSERT(newval != ts::Names::UNKNOWN);
    TSUNIT_ASSERT(newval >= 0);

    newval = e2.addNewValue(u"n3");
    debug() << "NamesTest::Unique: newval = " << newval << std::endl;
    TSUNIT_EQUAL(ts::Names::UNKNOWN, newval);
}

TSUNIT_DEFINE_TEST(NameList)
{
    ts::Names e1({{u"FirstElement", -1},
                  {u"SecondElement", 7},
                  {u"FirstRepetition", 47},
                  {u"OtherValue", -123},
                  {u"AddedElement", 458}});

    ts::UStringVector ref;
    ref.push_back(u"FirstElement");
    ref.push_back(u"SecondElement");
    ref.push_back(u"FirstRepetition");
    ref.push_back(u"OtherValue");
    ref.push_back(u"AddedElement");

    const ts::UString list(e1.nameList());
    debug() << "EnumerationTest: e1.nameList() = \"" << list << "\"" << std::endl;

    ts::UStringVector value;
    list.split(value);

    std::sort(ref.begin(), ref.end());
    std::sort(value.begin(), value.end());
    TSUNIT_ASSERT(value == ref);
}

TSUNIT_DEFINE_TEST(Error)
{
    ts::Names e({{u"version",   0},
                 {u"verbose",   1},
                 {u"versatile", 2},
                 {u"other",     3}});

    TSUNIT_EQUAL(u"", e.error(u"oth"));
    TSUNIT_EQUAL(u"", e.error(u"versi"));
    TSUNIT_EQUAL(u"unknown name \"foo\"", e.error(u"foo"));
    TSUNIT_EQUAL(u"ambiguous command \"vers\", could be one of version, versatile", e.error(u"vers", true, true, u"command"));
    TSUNIT_EQUAL(u"ambiguous option \"--ver\", could be one of --version, --verbose, --versatile", e.error(u"ver", true, true, u"option", u"--"));
}

TSUNIT_DEFINE_TEST(OUI)
{
    TSUNIT_EQUAL(u"Cisco", ts::OUIName(12));
    TSUNIT_EQUAL(ts::MICRO_SIGN + ts::UString(u"Tech Tecnologia"), ts::OUIName(0xF8E7B5));
    TSUNIT_EQUAL(u"Apple", ts::OUIName(0xFCFC48, ts::NamesFlags::NAME_OR_VALUE));
    TSUNIT_EQUAL(u"0xFFFFF8", ts::OUIName(0xFFFFF8, ts::NamesFlags::NAME_OR_VALUE));
}

TSUNIT_DEFINE_TEST(Dektec)
{
    // Just check that the names file is correctly read and valid.
    ts::NamesPtr sec = ts::Names::GetSection(u"dektec", u"DtCaps", false);
    TSUNIT_ASSERT(sec != nullptr);
}

TSUNIT_DEFINE_TEST(HiDes)
{
    // Just check that the names file is correctly read and valid.
    ts::NamesPtr sec = ts::Names::GetSection(u"hides", u"HiDesErrorLinux", false);
    TSUNIT_ASSERT(sec != nullptr);
}

TSUNIT_DEFINE_TEST(IP)
{
    // Just check that the names file is correctly read and valid.
    TSUNIT_ASSERT(ts::NameFromSection(u"ip", u"IPProtocol", 6).starts_with(u"TCP"));
}

TSUNIT_DEFINE_TEST(TID)
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"CAT", ts::TIDName(duck, ts::TID_CAT));
    TSUNIT_EQUAL(u"CAT", ts::TIDName(duck, ts::TID_CAT, ts::PID_NULL, ts::CASID_NAGRA_MIN));
    TSUNIT_EQUAL(u"PMT", ts::TIDName(duck, ts::TID_PMT, ts::PID_NULL, ts::CASID_VIACCESS_MIN));
    TSUNIT_EQUAL(u"Viaccess EMM-U", ts::TIDName(duck, ts::TID_VIA_EMM_U, ts::PID_NULL, ts::CASID_VIACCESS_MIN));
    TSUNIT_EQUAL(u"EIT schedule Actual", ts::TIDName(duck, ts::TID_EIT_S_ACT_MIN + 4));
    TSUNIT_EQUAL(u"ECM (odd)", ts::TIDName(duck, ts::TID_ECM_81));
    TSUNIT_EQUAL(u"Nagravision ECM (odd)", ts::TIDName(duck, ts::TID_ECM_81, ts::PID_NULL, ts::CASID_NAGRA_MIN));
    TSUNIT_EQUAL(u"SafeAccess EMM-A (0x86)", ts::TIDName(duck, ts::TID_SA_EMM_A, ts::PID_NULL, ts::CASID_SAFEACCESS, ts::NamesFlags::NAME_VALUE));
    TSUNIT_EQUAL(u"SGT (Astra)", ts::TIDName(duck, ts::TID_ASTRA_SGT));
    TSUNIT_EQUAL(u"unknown (0x90)", ts::TIDName(duck, ts::TID_LW_DMT));
}

TSUNIT_DEFINE_TEST(SharedTID)
{
    // Shared table ids between ATSC and ISDB.

    ts::DuckContext duck;
    TSUNIT_EQUAL(ts::TID_MGT, ts::TID_LDT);
    TSUNIT_EQUAL(ts::TID_TVCT, ts::TID_CDT);

    duck.addStandards(ts::Standards::ISDB);
    TSUNIT_EQUAL(u"LDT (ISDB)", ts::TIDName(duck, ts::TID_MGT));
    TSUNIT_EQUAL(u"CDT (ISDB)", ts::TIDName(duck, ts::TID_TVCT));

    duck.resetStandards(ts::Standards::ATSC);
    TSUNIT_EQUAL(u"MGT (ATSC)", ts::TIDName(duck, ts::TID_MGT));
    TSUNIT_EQUAL(u"TVCT (ATSC)", ts::TIDName(duck, ts::TID_TVCT));
}

TSUNIT_DEFINE_TEST(PDS)
{
    const ts::UString tdfRef = ts::UString(u"T") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"l") + ts::LATIN_SMALL_LETTER_E_WITH_ACUTE + ts::UString(u"diffusion de France (TDF)");

    TSUNIT_EQUAL(u"EACEM/EICTA", ts::PDSName(0x28));
    TSUNIT_EQUAL(tdfRef, ts::PDSName(0x1A));

    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028)", ts::PDSName(0x28, ts::NamesFlags::NAME_VALUE));
    TSUNIT_EQUAL(u"0x00000028 (EACEM/EICTA)", ts::PDSName(0x28, ts::NamesFlags::VALUE_NAME));
    TSUNIT_EQUAL(u"40 (EACEM/EICTA)", ts::PDSName(0x28, ts::NamesFlags::DEC_VALUE_NAME));
    TSUNIT_EQUAL(u"0x00000028 (40, EACEM/EICTA)", ts::PDSName(0x28, ts::NamesFlags::VALUE_NAME | ts::NamesFlags::HEX_DEC));
    TSUNIT_EQUAL(u"EACEM/EICTA (0x00000028, 40)", ts::PDSName(0x28, ts::NamesFlags::NAME_VALUE | ts::NamesFlags::HEX_DEC));
    TSUNIT_EQUAL(u"EACEM/EICTA (40)", ts::PDSName(0x28, ts::NamesFlags::NAME_VALUE | ts::NamesFlags::DECIMAL));

    TSUNIT_EQUAL(u"unknown (0x00008123)", ts::PDSName(0x8123));
    TSUNIT_EQUAL(u"unknown (33059)", ts::PDSName(0x8123, ts::NamesFlags::DECIMAL));
    TSUNIT_EQUAL(u"33059 (unknown)", ts::PDSName(0x8123, ts::NamesFlags::DEC_VALUE_NAME));
    TSUNIT_EQUAL(u"unknown (0x00008123, 33059)", ts::PDSName(0x8123, ts::NamesFlags::DECIMAL | ts::NamesFlags::HEXA));
}

TSUNIT_DEFINE_TEST(REGID)
{
    TSUNIT_EQUAL(u"\"SCTE\", Society of Cable Telecommunications Engineers", ts::REGIDName(0x53435445));
    TSUNIT_EQUAL(u"\"ABCD\"", ts::REGIDName(0x41424344));
    TSUNIT_EQUAL(u"unknown (0x41424302)", ts::REGIDName(0x41424302));
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
    TSUNIT_EQUAL(u"Viaccess", ts::CASIdName(duck, 0x500));
    TSUNIT_EQUAL(u"Irdeto", ts::CASIdName(duck, 0x601));
}

TSUNIT_DEFINE_TEST(BouquetId)
{
    TSUNIT_EQUAL(ts::UString(u"T") + ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS + ts::UString(u"rk Telekom"), ts::BouquetIdName(0x55));
}

TSUNIT_DEFINE_TEST(DataBroadcastId)
{
    TSUNIT_EQUAL(u"OpenTV Data Carousel", ts::DataBroadcastIdName(0x0107));
}

TSUNIT_DEFINE_TEST(OriginalNetworkId)
{
    TSUNIT_EQUAL(u"Skylogic", ts::OriginalNetworkIdName(0x4C));
}

TSUNIT_DEFINE_TEST(NetworkId)
{
    TSUNIT_EQUAL(ts::UString(u"Eutelsat satellite system at 4") + ts::DEGREE_SIGN + ts::UString(u"East"), ts::NetworkIdName(0x4C));
}

TSUNIT_DEFINE_TEST(Content)
{
    ts::DuckContext duck1;
    TSUNIT_EQUAL(u"game show/quiz/contest", ts::ContentIdName(duck1, 0x31));
    duck1.addStandards(ts::Standards::JAPAN);
    TSUNIT_EQUAL(u"overseas drama", ts::ContentIdName(duck1, 0x31));
    ts::DuckContext duck2;
    duck2.addStandards(ts::Standards::ABNT);
    TSUNIT_EQUAL(u"soap opera", ts::ContentIdName(duck2, 0x31));
}

TSUNIT_DEFINE_TEST(DID)
{
    ts::DuckContext duck;
    ts::DescriptorContext context1(duck);
    TSUNIT_EQUAL(u"CA", ts::DIDName(ts::DID_MPEG_CA, context1));
    TSUNIT_EQUAL(u"ISO-639 Language", ts::DIDName(ts::DID_MPEG_LANGUAGE, context1));

    ts::DescriptorContext context_dvb(duck, ts::TID_NULL, ts::Standards::DVB);
    TSUNIT_EQUAL(u"Data Broadcast Id", ts::DIDName(ts::DID_DVB_DATA_BROADCAST_ID, context_dvb));
    TSUNIT_EQUAL(u"unknown (0x83)", ts::DIDName(ts::DID_EACEM_LCN, context_dvb));

    ts::DescriptorContext context_eacem(duck, ts::TID_NULL, ts::Standards::DVB, ts::CASID_NULL, ts::REGIDVector(), ts::PDS_EACEM);
    TSUNIT_EQUAL(u"Logical Channel Number", ts::DIDName(ts::DID_EACEM_LCN, context_eacem));
    TSUNIT_EQUAL(u"0x83 (Logical Channel Number)", ts::DIDName(ts::DID_EACEM_LCN, context_eacem, ts::NamesFlags::VALUE_NAME));
}

TSUNIT_DEFINE_TEST(XDID)
{
    TSUNIT_EQUAL(u"Green Extension", ts::XDIDNameMPEG(ts::XDID_MPEG_GREEN_EXT));
    TSUNIT_EQUAL(u"0x08 (MPEG-H 3D Audio)", ts::XDIDNameMPEG(ts::XDID_MPEG_MPH3D_AUDIO, ts::NamesFlags::VALUE_NAME));
    TSUNIT_EQUAL(u"T2 Delivery System", ts::XDIDNameDVB(ts::XDID_DVB_T2_DELIVERY));
    TSUNIT_EQUAL(u"0xAA (unknown)", ts::XDIDNameDVB(0xAA, ts::NamesFlags::VALUE_NAME));
}

TSUNIT_DEFINE_TEST(StreamType)
{
    TSUNIT_EQUAL(u"MPEG-4 Video", ts::StreamTypeName(ts::ST_MPEG4_VIDEO));
    TSUNIT_EQUAL(u"SCTE 35 Splice Info", ts::StreamTypeName(ts::ST_SCTE35_SPLICE));
    TSUNIT_EQUAL(u"DTS-HD Master Audio", ts::StreamTypeName(ts::ST_SCTE35_SPLICE, {ts::REGID_HDMV}));

    ts::DuckContext duck;
    ts::DescriptorList dlist(nullptr);
    dlist.add(duck, ts::RegistrationDescriptor(ts::REGID_CUEI));
    TSUNIT_EQUAL(u"SCTE 35 Splice Info", ts::StreamTypeName(ts::ST_SCTE35_SPLICE, duck, dlist));

    dlist.add(duck, ts::RegistrationDescriptor(ts::REGID_HDMV));
    TSUNIT_EQUAL(u"DTS-HD Master Audio", ts::StreamTypeName(ts::ST_SCTE35_SPLICE, duck, dlist));
}

TSUNIT_DEFINE_TEST(ServiceType)
{
    TSUNIT_EQUAL(u"Data broadcast service", ts::ServiceTypeName(0x0C));
    TSUNIT_EQUAL(u"unknown (0x80)", ts::ServiceTypeName(128));
}

TSUNIT_DEFINE_TEST(ComponentType)
{
    ts::DuckContext duck;
    TSUNIT_EQUAL(u"MPEG-2 video, 4:3 aspect ratio, 30 Hz", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0x05));
    TSUNIT_EQUAL(u"DVB subtitles, no aspect ratio", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x10));
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::ComponentDescriptor::ComponentTypeName(duck, 4, 0, 0x92));
    TSUNIT_EQUAL(u"0x0492 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::ComponentDescriptor::ComponentTypeName(duck, 4, 0, 0x92, ts::NamesFlags::VALUE_NAME));
    TSUNIT_EQUAL(u"MPEG-2 high definition video, > 16:9 aspect ratio, 30 Hz", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0x10));
    TSUNIT_EQUAL(u"MPEG-2 video", ts::ComponentDescriptor::ComponentTypeName(duck, 1, 0, 0xB4));
    TSUNIT_EQUAL(u"0x0341 (Video is standard dynamic range (SDR))", ts::ComponentDescriptor::ComponentTypeName(duck, 3, 0, 0x41, ts::NamesFlags::VALUE_NAME));

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
    TSUNIT_EQUAL(u"data broadcast service", ts::NameFromSection(u"dtv", u"linkage_descriptor.linkage_type", 0x06));
}

TSUNIT_DEFINE_TEST(TeletextType)
{
    TSUNIT_EQUAL(u"Teletext subtitles", ts::NameFromSection(u"dtv", u"teletext_descriptor.teletext_type", 2));
}

TSUNIT_DEFINE_TEST(RunningStatus)
{
    TSUNIT_EQUAL(u"running", ts::RunningStatusName(4));
}

TSUNIT_DEFINE_TEST(StreamId)
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromSection(u"dtv", u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromSection(u"dtv", u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromSection(u"dtv", u"pes.stream_id", 0xEC));
}

TSUNIT_DEFINE_TEST(PESStartCode)
{
    TSUNIT_EQUAL(u"ISO-13522 Hypermedia", ts::NameFromSection(u"dtv", u"pes.stream_id", ts::SID_ISO13522));
    TSUNIT_EQUAL(u"Audio 24", ts::NameFromSection(u"dtv", u"pes.stream_id", 0xD8));
    TSUNIT_EQUAL(u"Video 12", ts::NameFromSection(u"dtv", u"pes.stream_id", 0xEC));
    TSUNIT_EQUAL(u"Slice 117", ts::NameFromSection(u"dtv", u"pes.stream_id", 0x75));
    TSUNIT_EQUAL(u"Sequence header", ts::NameFromSection(u"dtv", u"pes.stream_id", 0xB3));
}

TSUNIT_DEFINE_TEST(AspectRatio)
{
    TSUNIT_EQUAL(u"16:9", ts::NameFromSection(u"dtv", u"mpeg2.aspect_ratio", ts::AR_16_9));
}

TSUNIT_DEFINE_TEST(ChromaFormat)
{
    TSUNIT_EQUAL(u"4:2:0", ts::NameFromSection(u"dtv", u"mpeg2.chroma_format", ts::CHROMA_420));
}

TSUNIT_DEFINE_TEST(AVCUnitType)
{
    TSUNIT_EQUAL(u"Picture parameter set", ts::AccessUnitTypeName(ts::CodecType::AVC, ts::AVC_AUT_PICPARAMS));
}

TSUNIT_DEFINE_TEST(AVCProfile)
{
    TSUNIT_EQUAL(u"extended profile", ts::NameFromSection(u"dtv", u"avc.profile", 88));
}

TSUNIT_DEFINE_TEST(AC3ComponentType)
{
    TSUNIT_EQUAL(u"Enhanced AC-3, combined, visually impaired, 2 channels", ts::DVBAC3Descriptor::ComponentTypeName(0x92));
    TSUNIT_EQUAL(u"0x92 (Enhanced AC-3, combined, visually impaired, 2 channels)", ts::DVBAC3Descriptor::ComponentTypeName(0x92, ts::NamesFlags::VALUE_NAME));
}

TSUNIT_DEFINE_TEST(ScramblingControl)
{
    TSUNIT_EQUAL(u"even", ts::NameFromSection(u"dtv", u"ts.scrambling_control", 2));
}

TSUNIT_DEFINE_TEST(DTSExtendedSurroundMode)
{
    TSUNIT_EQUAL(u"matrixed", ts::NameFromSection(u"dtv", u"DTS_descriptor.ExtendedSurroundMode", 1));
}

TSUNIT_DEFINE_TEST(DTSSurroundMode)
{
    TSUNIT_EQUAL(u"3 / C+L+R", ts::NameFromSection(u"dtv", u"DTS_descriptor.SurroundMode", 5));
}

TSUNIT_DEFINE_TEST(DTSBitRateCode)
{
    TSUNIT_EQUAL(u"512 kb/s", ts::NameFromSection(u"dtv", u"DTS_descriptor.BitRate", 12));
}

TSUNIT_DEFINE_TEST(DTSSampleRateCode)
{
    TSUNIT_EQUAL(u"22.05 kHz", ts::NameFromSection(u"dtv", u"DTS_descriptor.SampleRate", 7));
}

TSUNIT_DEFINE_TEST(AudioType)
{
    TSUNIT_EQUAL(u"hearing impaired", ts::NameFromSection(u"dtv", u"ISO_639_language_descriptor.audio_type", 2));
}

TSUNIT_DEFINE_TEST(T2MIPacketType)
{
    TSUNIT_EQUAL(u"Individual addressing", ts::NameFromSection(u"dtv", u"t2mi.packet_type", 0x21));
}

TSUNIT_DEFINE_TEST(PlatformId)
{
    TSUNIT_EQUAL(u"Horizonsat", ts::NameFromSection(u"dtv", u"INT.platform_id", 10));
    TSUNIT_EQUAL(u"0x000004 (TV digitale mobile, Telecom Italia)", ts::NameFromSection(u"dtv", u"INT.platform_id", 4, ts::NamesFlags::VALUE_NAME));
    TSUNIT_EQUAL(u"VTC Mobile TV (0x704001)", ts::NameFromSection(u"dtv", u"INT.platform_id", 0x704001, ts::NamesFlags::NAME_VALUE));
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

    ts::NamesPtr sec = ts::Names::GetSection(_tempFileName, u"level3", false);
    TSUNIT_ASSERT(sec != nullptr);

    TSUNIT_ASSERT(sec->contains(3));
    TSUNIT_ASSERT(sec->contains(2));
    TSUNIT_ASSERT(sec->contains(1));
    TSUNIT_ASSERT(!sec->contains(0));

    sec = ts::Names::GetSection(u"", u"level2", false);
    TSUNIT_ASSERT(sec != nullptr);

    TSUNIT_ASSERT(!sec->contains(3));
    TSUNIT_ASSERT(sec->contains(2));
    TSUNIT_ASSERT(sec->contains(1));
    TSUNIT_ASSERT(!sec->contains(0));

    sec = ts::Names::GetSection(u"", u"level1", false);
    TSUNIT_ASSERT(sec != nullptr);

    TSUNIT_ASSERT(!sec->contains(3));
    TSUNIT_ASSERT(!sec->contains(2));
    TSUNIT_ASSERT(sec->contains(1));
    TSUNIT_ASSERT(!sec->contains(0));

    TSUNIT_EQUAL(u"value3", ts::NameFromSection(u"", u"level3", 3));
    TSUNIT_EQUAL(u"value2", ts::NameFromSection(u"", u"level3", 2));
    TSUNIT_EQUAL(u"value1", ts::NameFromSection(u"", u"level3", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", ts::NameFromSection(u"", u"level3", 0));

    TSUNIT_EQUAL(u"unknown (0x03)", ts::NameFromSection(u"", u"level2", 3));
    TSUNIT_EQUAL(u"value2", ts::NameFromSection(u"", u"level2", 2));
    TSUNIT_EQUAL(u"value1", ts::NameFromSection(u"", u"level2", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", ts::NameFromSection(u"", u"level2", 0));

    TSUNIT_EQUAL(u"unknown (0x03)", ts::NameFromSection(u"", u"level1", 3));
    TSUNIT_EQUAL(u"unknown (0x02)", ts::NameFromSection(u"", u"level1", 2));
    TSUNIT_EQUAL(u"value1", ts::NameFromSection(u"", u"level1", 1));
    TSUNIT_EQUAL(u"unknown (0x00)", ts::NameFromSection(u"", u"level1", 0));
}

TSUNIT_DEFINE_TEST(Extension)
{
    // Create a temporary names file.
    debug() << "NamesTest::testExtension: extension file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(ts::UString::Save(ts::UStringVector({u"[CASystemId]", u"0xF123 = test-cas"}), _tempFileName));

    ts::DuckContext duck;
    TSUNIT_EQUAL(u"unknown (0xF123)", ts::CASIdName(duck, 0xF123));
    ts::Names::RegisterExtensionFile reg(_tempFileName);
    TSUNIT_EQUAL(u"test-cas", ts::CASIdName(duck, 0xF123));
}
