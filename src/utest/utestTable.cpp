//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for various tables.
//
//----------------------------------------------------------------------------

#include "tsCAT.h"
#include "tsPMT.h"
#include "tsBAT.h"
#include "tsNIT.h"
#include "tsSDT.h"
#include "tsTOT.h"
#include "tsTSDT.h"
#include "tsEIT.h"
#include "tsAIT.h"
#include "tsLTST.h"
#include "tsContainerTable.h"
#include "tsBinaryTable.h"
#include "tsCADescriptor.h"
#include "tsAVCVideoDescriptor.h"
#include "tsDVBAC3Descriptor.h"
#include "tsEacemPreferredNameIdentifierDescriptor.h"
#include "tsEacemLogicalChannelNumberDescriptor.h"
#include "tsEacemStreamIdentifierDescriptor.h"
#include "tsEutelsatChannelNumberDescriptor.h"
#include "tsPrivateDataSpecifierDescriptor.h"
#include "tsRegistrationDescriptor.h"
#include "tsISO639LanguageDescriptor.h"
#include "tsCueIdentifierDescriptor.h"
#include "tsDuckContext.h"
#include "tsZlib.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TableTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(AssignPMT);
    TSUNIT_DECLARE_TEST(CopyPMT);
    TSUNIT_DECLARE_TEST(AIT);
    TSUNIT_DECLARE_TEST(BAT);
    TSUNIT_DECLARE_TEST(CAT);
    TSUNIT_DECLARE_TEST(EIT);
    TSUNIT_DECLARE_TEST(NIT);
    TSUNIT_DECLARE_TEST(SDT);
    TSUNIT_DECLARE_TEST(TOT);
    TSUNIT_DECLARE_TEST(TSDT);
    TSUNIT_DECLARE_TEST(CleanupPrivateDescriptors);
    TSUNIT_DECLARE_TEST(PrivateDescriptors);
    TSUNIT_DECLARE_TEST(ContainerTable);
    TSUNIT_DECLARE_TEST(LTST);
};

TSUNIT_REGISTER(TableTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(AssignPMT)
{
    ts::DuckContext duck;
    ts::PMT pmt1(1, true, 27, 1001);
    pmt1.descs.add(duck, ts::CADescriptor(0x1234, 2002));
    pmt1.streams[3003].stream_type = 45;
    pmt1.streams[3003].descs.add(duck, ts::AVCVideoDescriptor());
    pmt1.streams[4004].stream_type = 149;
    pmt1.streams[4004].descs.add(duck, ts::DVBAC3Descriptor());
    pmt1.streams[4004].descs.add(duck, ts::CADescriptor());

    const ts::PMT pmt2(pmt1);

    TSUNIT_ASSERT(pmt2.isValid());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.tableId());
    TSUNIT_ASSERT(pmt2.isCurrent());
    TSUNIT_EQUAL(1, pmt2.version());
    TSUNIT_EQUAL(27, pmt2.service_id);
    TSUNIT_EQUAL(1001, pmt2.pcr_pid);

    TSUNIT_EQUAL(ts::TID_PMT, pmt2.descs.tableId());
    TSUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    TSUNIT_EQUAL(1, pmt2.descs.count());
    TSUNIT_ASSERT(pmt2.descs[0].isValid());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.descs[0].tag());

    TSUNIT_EQUAL(2, pmt2.streams.size());

    TSUNIT_EQUAL(45, pmt2.streams[3003].stream_type);
    TSUNIT_EQUAL(1, pmt2.streams[3003].descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_AVC_VIDEO, pmt2.streams[3003].descs[0].tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[3003].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    TSUNIT_EQUAL(149, pmt2.streams[4004].stream_type);
    TSUNIT_EQUAL(2, pmt2.streams[4004].descs.count());
    TSUNIT_EQUAL(ts::DID_DVB_AC3, pmt2.streams[4004].descs[0].tag());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.streams[4004].descs[1].tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[4004].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[4004].descs.table() == &pmt2);
}

TSUNIT_DEFINE_TEST(CopyPMT)
{
    ts::DuckContext duck;
    ts::PMT pmt1(1, true, 27, 1001);
    pmt1.descs.add(duck, ts::CADescriptor(0x1234, 2002));
    pmt1.streams[3003].stream_type = 45;
    pmt1.streams[3003].descs.add(duck, ts::AVCVideoDescriptor());
    pmt1.streams[4004].stream_type = 149;
    pmt1.streams[4004].descs.add(duck, ts::DVBAC3Descriptor());
    pmt1.streams[4004].descs.add(duck, ts::CADescriptor());

    ts::PMT pmt2;
    pmt2 = pmt1;

    TSUNIT_ASSERT(pmt2.isValid());
    TSUNIT_EQUAL(uint8_t(ts::TID_PMT), pmt2.tableId());
    TSUNIT_ASSERT(pmt2.isCurrent());
    TSUNIT_EQUAL(1, pmt2.version());
    TSUNIT_EQUAL(27, pmt2.service_id);
    TSUNIT_EQUAL(1001, pmt2.pcr_pid);

    TSUNIT_EQUAL(ts::TID_PMT, pmt2.descs.tableId());
    TSUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    TSUNIT_EQUAL(1, pmt2.descs.count());
    TSUNIT_ASSERT(pmt2.descs[0].isValid());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.descs[0].tag());

    TSUNIT_EQUAL(2, pmt2.streams.size());

    TSUNIT_EQUAL(45, pmt2.streams[3003].stream_type);
    TSUNIT_EQUAL(1, pmt2.streams[3003].descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_AVC_VIDEO, pmt2.streams[3003].descs[0].tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[3003].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    TSUNIT_EQUAL(149, pmt2.streams[4004].stream_type);
    TSUNIT_EQUAL(2, pmt2.streams[4004].descs.count());
    TSUNIT_EQUAL(ts::DID_DVB_AC3, pmt2.streams[4004].descs[0].tag());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.streams[4004].descs[1].tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[4004].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[4004].descs.table() == &pmt2);
}

TSUNIT_DEFINE_TEST(AIT)
{
    ts::DuckContext duck;
    ts::ApplicationIdentifier id;
    ts::AIT ait1;
    ait1.applications[id].descs.add(duck, ts::CADescriptor());
    TSUNIT_EQUAL(1, ait1.applications.size());
    TSUNIT_ASSERT(ait1.applications.begin()->first == id);
    TSUNIT_ASSERT(ait1.applications.begin()->second.descs.table() == &ait1);

    ts::AIT ait2(ait1);
    TSUNIT_EQUAL(1, ait2.applications.size());
    TSUNIT_ASSERT(ait2.applications.begin()->first == id);
    TSUNIT_ASSERT(ait2.applications.begin()->second.descs.table() == &ait2);

    ts::AIT ait3;
    ait3 = ait1;
    TSUNIT_EQUAL(1, ait3.applications.size());
    TSUNIT_ASSERT(ait3.applications.begin()->first == id);
    TSUNIT_ASSERT(ait3.applications.begin()->second.descs.table() == &ait3);
}

TSUNIT_DEFINE_TEST(BAT)
{
    ts::DuckContext duck;
    ts::BAT bat1;
    bat1.transports[ts::TransportStreamId(1, 2)].descs.add(duck, ts::CADescriptor());
    TSUNIT_ASSERT(bat1.descs.table() == &bat1);
    TSUNIT_EQUAL(1, bat1.transports.size());
    TSUNIT_ASSERT(bat1.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(bat1.transports.begin()->second.descs.table() == &bat1);

    ts::BAT bat2(bat1);
    TSUNIT_ASSERT(bat2.descs.table() == &bat2);
    TSUNIT_EQUAL(1, bat2.transports.size());
    TSUNIT_ASSERT(bat2.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(bat2.transports.begin()->second.descs.table() == &bat2);

    ts::BAT bat3;
    bat3 = bat1;
    TSUNIT_ASSERT(bat3.descs.table() == &bat3);
    TSUNIT_EQUAL(1, bat3.transports.size());
    TSUNIT_ASSERT(bat3.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(bat3.transports.begin()->second.descs.table() == &bat3);
}

TSUNIT_DEFINE_TEST(CAT)
{
    ts::CAT cat1;
    TSUNIT_ASSERT(cat1.descs.table() == &cat1);

    ts::CAT cat2(cat1);
    TSUNIT_ASSERT(cat2.descs.table() == &cat2);

    ts::CAT cat3;
    cat3 = cat1;
    TSUNIT_ASSERT(cat3.descs.table() == &cat3);
}

TSUNIT_DEFINE_TEST(EIT)
{
    ts::DuckContext duck;
    ts::EIT eit1;
    eit1.events[1].descs.add(duck, ts::CADescriptor());
    TSUNIT_EQUAL(1, eit1.events.size());
    TSUNIT_ASSERT(eit1.events.begin()->first == 1);
    TSUNIT_ASSERT(eit1.events.begin()->second.descs.table() == &eit1);

    ts::EIT eit2(eit1);
    TSUNIT_EQUAL(1, eit2.events.size());
    TSUNIT_ASSERT(eit2.events.begin()->first == 1);
    TSUNIT_ASSERT(eit2.events.begin()->second.descs.table() == &eit2);

    ts::EIT eit3;
    eit3 = eit1;
    TSUNIT_EQUAL(1, eit3.events.size());
    TSUNIT_ASSERT(eit3.events.begin()->first == 1);
    TSUNIT_ASSERT(eit3.events.begin()->second.descs.table() == &eit3);
}

TSUNIT_DEFINE_TEST(NIT)
{
    ts::DuckContext duck;
    ts::NIT nit1;
    nit1.transports[ts::TransportStreamId(1, 2)].descs.add(duck, ts::CADescriptor());
    TSUNIT_ASSERT(nit1.descs.table() == &nit1);
    TSUNIT_EQUAL(1, nit1.transports.size());
    TSUNIT_ASSERT(nit1.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(nit1.transports.begin()->second.descs.table() == &nit1);

    ts::NIT nit2(nit1);
    TSUNIT_ASSERT(nit2.descs.table() == &nit2);
    TSUNIT_EQUAL(1, nit2.transports.size());
    TSUNIT_ASSERT(nit2.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(nit2.transports.begin()->second.descs.table() == &nit2);

    ts::NIT nit3;
    nit3 = nit1;
    TSUNIT_ASSERT(nit3.descs.table() == &nit3);
    TSUNIT_EQUAL(1, nit3.transports.size());
    TSUNIT_ASSERT(nit3.transports.begin()->first == ts::TransportStreamId(1, 2));
    TSUNIT_ASSERT(nit3.transports.begin()->second.descs.table() == &nit3);
}

TSUNIT_DEFINE_TEST(SDT)
{
    ts::DuckContext duck;
    ts::SDT sdt1;
    sdt1.services[1].descs.add(duck, ts::CADescriptor());
    TSUNIT_EQUAL(1, sdt1.services.size());
    TSUNIT_ASSERT(sdt1.services.begin()->first == 1);
    TSUNIT_ASSERT(sdt1.services.begin()->second.descs.table() == &sdt1);

    ts::SDT sdt2(sdt1);
    TSUNIT_EQUAL(1, sdt2.services.size());
    TSUNIT_ASSERT(sdt2.services.begin()->first == 1);
    TSUNIT_ASSERT(sdt2.services.begin()->second.descs.table() == &sdt2);

    ts::SDT sdt3;
    sdt3 = sdt1;
    TSUNIT_EQUAL(1, sdt3.services.size());
    TSUNIT_ASSERT(sdt3.services.begin()->first == 1);
    TSUNIT_ASSERT(sdt3.services.begin()->second.descs.table() == &sdt3);
}

TSUNIT_DEFINE_TEST(TOT)
{
    ts::DuckContext duck;
    ts::TOT tot1;
    tot1.descs.add(duck, ts::CADescriptor());
    TSUNIT_ASSERT(tot1.descs.table() == &tot1);
    TSUNIT_EQUAL(1, tot1.descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot1.descs[0].tag());

    ts::TOT tot2(tot1);
    TSUNIT_ASSERT(tot2.descs.table() == &tot2);
    TSUNIT_EQUAL(1, tot2.descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot2.descs[0].tag());

    ts::TOT tot3;
    tot3 = tot1;
    TSUNIT_ASSERT(tot3.descs.table() == &tot3);
    TSUNIT_EQUAL(1, tot2.descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot2.descs[0].tag());
}

TSUNIT_DEFINE_TEST(TSDT)
{
    ts::TSDT tsdt1;
    TSUNIT_ASSERT(tsdt1.descs.table() == &tsdt1);

    ts::TSDT tsdt2(tsdt1);
    TSUNIT_ASSERT(tsdt2.descs.table() == &tsdt2);

    ts::TSDT tsdt3;
    tsdt3 = tsdt1;
    TSUNIT_ASSERT(tsdt3.descs.table() == &tsdt3);
}

TSUNIT_DEFINE_TEST(CleanupPrivateDescriptors)
{
    // Issue #87 non-regression.
    ts::DuckContext duck;
    ts::DescriptorList dlist(nullptr);
    dlist.add(duck, ts::EacemPreferredNameIdentifierDescriptor());
    dlist.add(duck, ts::EacemLogicalChannelNumberDescriptor());
    dlist.add(duck, ts::ServiceDescriptor());
    dlist.add(duck, ts::EutelsatChannelNumberDescriptor());

    TSUNIT_EQUAL(4, dlist.count());
    dlist.removeInvalidPrivateDescriptors();
    TSUNIT_EQUAL(1, dlist.count());
    TSUNIT_EQUAL(ts::DID_DVB_SERVICE, dlist[0].tag());
}

TSUNIT_DEFINE_TEST(PrivateDescriptors)
{
    ts::DuckContext duck;
    duck.addStandards(ts::Standards::DVB);

    ts::PMT pmt;
    pmt.descs.add(duck, ts::ISO639LanguageDescriptor(u"foo", 0));
    pmt.descs.add(duck, ts::PrivateDataSpecifierDescriptor(ts::PDS_OFCOM));
    pmt.descs.add(duck, ts::RegistrationDescriptor(ts::REGID_HDMV));

    // Program-level descriptor list:
    //   0: ISO639LanguageDescriptor
    //   1: PrivateDataSpecifierDescriptor (OFCOM)
    //   2: RegistrationDescriptor (HDMV)

    TSUNIT_EQUAL(3, pmt.descs.size());
    TSUNIT_EQUAL(ts::REGID_NULL, pmt.descs.registrationId(0));
    TSUNIT_EQUAL(ts::REGID_HDMV, pmt.descs.registrationId(10));
    TSUNIT_EQUAL(ts::PDS_NULL, pmt.descs.privateDataSpecifier(0));
    TSUNIT_EQUAL(ts::PDS_OFCOM, pmt.descs.privateDataSpecifier(10));

    pmt.streams[100].descs.add(duck, ts::ISO639LanguageDescriptor(u"bar", 0));
    pmt.streams[100].descs.add(duck, ts::CueIdentifierDescriptor(ts::CUE_SEGMENTATION));

    // Component-level descriptor list:
    //   0: ISO639LanguageDescriptor
    //   1: CueIdentifierDescriptor

    TSUNIT_EQUAL(2, pmt.streams[100].descs.size());
    TSUNIT_EQUAL(ts::REGID_HDMV, pmt.streams[100].descs.registrationId(10));
    TSUNIT_EQUAL(ts::PDS_NULL, pmt.streams[100].descs.privateDataSpecifier(10));

    ts::DescriptorContext context1(duck, pmt.streams[100].descs, 1);
    ts::AbstractDescriptorPtr desc = pmt.streams[100].descs[1].deserialize(duck, context1);
    TSUNIT_ASSERT(desc == nullptr);

    pmt.descs.add(duck, ts::RegistrationDescriptor(ts::REGID_CUEI));

    // Program-level descriptor list:
    //   0: ISO639LanguageDescriptor
    //   1: PrivateDataSpecifierDescriptor (OFCOM)
    //   2: RegistrationDescriptor (HDMV)
    //   3: RegistrationDescriptor (CUEI)

    TSUNIT_EQUAL(ts::REGID_CUEI, pmt.descs.registrationId(10));
    TSUNIT_EQUAL(ts::REGID_CUEI, pmt.streams[100].descs.registrationId(0));

    ts::DescriptorContext context2(duck, pmt.streams[100].descs, 1);
    desc = pmt.streams[100].descs[1].deserialize(duck, context2);
    TSUNIT_ASSERT(desc != nullptr);

    ts::CueIdentifierDescriptor* cue_desc = dynamic_cast<ts::CueIdentifierDescriptor*>(desc.get());
    TSUNIT_ASSERT(cue_desc != nullptr);
    TSUNIT_EQUAL(ts::CUE_SEGMENTATION, cue_desc->cue_stream_type);

    pmt.streams[100].descs.add(duck, ts::EacemStreamIdentifierDescriptor(7));
    pmt.streams[100].descs.add(duck, ts::PrivateDataSpecifierDescriptor(ts::PDS_EACEM));
    pmt.streams[100].descs.add(duck, ts::EacemStreamIdentifierDescriptor(9));

    // Component-level descriptor list:
    //   0: ISO639LanguageDescriptor
    //   1: CueIdentifierDescriptor
    //   2: EacemStreamIdentifierDescriptor
    //   3: PrivateDataSpecifierDescriptor (EACEM)
    //   4: EacemStreamIdentifierDescriptor

    TSUNIT_EQUAL(4, pmt.descs.size());
    TSUNIT_EQUAL(5, pmt.streams[100].descs.size());
    TSUNIT_EQUAL(ts::DID_EACEM_STREAM_ID, pmt.streams[100].descs[2].tag());
    TSUNIT_EQUAL(ts::DID_DVB_PRIV_DATA_SPECIF, pmt.streams[100].descs[3].tag());
    TSUNIT_EQUAL(ts::DID_EACEM_STREAM_ID, pmt.streams[100].descs[4].tag());

    ts::DescriptorContext context3(duck, pmt.streams[100].descs, 2);
    desc = pmt.streams[100].descs[2].deserialize(duck, context3);
    TSUNIT_ASSERT(desc == nullptr);

    ts::DescriptorContext context4(duck, pmt.streams[100].descs, 4);
    desc = pmt.streams[100].descs[4].deserialize(duck, context4);
    TSUNIT_ASSERT(desc != nullptr);

    ts::EacemStreamIdentifierDescriptor* esi_desc = dynamic_cast<ts::EacemStreamIdentifierDescriptor*>(desc.get());
    TSUNIT_ASSERT(esi_desc != nullptr);
    TSUNIT_EQUAL(9, esi_desc->version);
}

TSUNIT_DEFINE_TEST(ContainerTable)
{
    ts::DuckContext duck;

    // Build container data which can be easily compressed.
    ts::ByteBlock container;
    container.reserve(256 * 50);
    for (int val = 0; val < 256; val++) {
        for (int count = 50; count > 0; count--) {
            container.push_back(uint8_t(val));
        }
    }
    debug() << "TableTest::ContainerTable: container size: " << container.size() << std::endl;

    ts::ContainerTable ct;
    ct.container_id = 0x1234;
    TSUNIT_ASSERT(ct.setContainer(container, false));
    TSUNIT_EQUAL(ct.compression_wrapper.size(), container.size() + 1);
    TSUNIT_EQUAL(0, ct.compression_wrapper[0]);
    TSUNIT_ASSERT(ts::MemEqual(ct.compression_wrapper.data() + 1, container.data(), container.size()));

    ts::ByteBlock out;
    TSUNIT_ASSERT(ct.getContainer(out));
    TSUNIT_ASSERT(out == container);

    ts::ByteBlock compressed;
    TSUNIT_ASSERT(ts::Zlib::Compress(compressed, container, 6));
    debug() << "TableTest::ContainerTable: direct compressed size: " << compressed.size() << std::endl;
    out.clear();
    TSUNIT_ASSERT(ts::Zlib::Decompress(out, compressed));
    debug() << "TableTest::ContainerTable: direct decompressed size: " << out.size() << std::endl;
    TSUNIT_ASSERT(out == container);

    ct.setContainer(container, true);
    debug() << "TableTest::ContainerTable: serialized size: " << ct.compression_wrapper.size() << std::endl;
    TSUNIT_ASSERT(ct.compression_wrapper.size() > 4);
    TSUNIT_ASSERT(ct.compression_wrapper.size() < container.size() + 4);
    TSUNIT_EQUAL(1, ct.compression_wrapper[0]);
    TSUNIT_EQUAL(container.size(), ts::GetUInt24(ct.compression_wrapper.data() + 1));

    out.clear();
    TSUNIT_ASSERT(ts::Zlib::Decompress(out, ct.compression_wrapper.data() + 4, ct.compression_wrapper.size() - 4));
    TSUNIT_EQUAL(container.size(), out.size());
    TSUNIT_ASSERT(out == container);

    out.clear();
    TSUNIT_ASSERT(ct.getContainer(out));
    TSUNIT_ASSERT(out == container);

    debug() << "TableTest::ContainerTable: serialize" << std::endl;
    ts::BinaryTable table;
    TSUNIT_ASSERT(ct.serialize(duck, table));

    debug() << "TableTest::ContainerTable: deserialize" << std::endl;
    ts::ContainerTable ct2(duck, table);
    TSUNIT_ASSERT(ct2.isValid());
    TSUNIT_EQUAL(0x1234, ct2.container_id);
    TSUNIT_ASSERT(!ct2.compression_wrapper.empty());
    TSUNIT_EQUAL(1, ct2.compression_wrapper[0]);

    out.clear();
    TSUNIT_ASSERT(ct2.getContainer(out));
    TSUNIT_ASSERT(out == container);
}

TSUNIT_DEFINE_TEST(LTST)
{
    ts::DuckContext duck;
    ts::LTST ltst(1, 2);

    auto& src1(ltst.sources.newEntry());
    src1.source_id = 11;
    auto& data11(src1.data.newEntry());
    data11.data_id = 111;
    data11.length_in_seconds = cn::seconds(1111);

    auto& src2(ltst.sources.newEntry());
    src2.source_id = 22;
    auto& data21(src2.data.newEntry());
    data21.data_id = 211;
    data21.length_in_seconds = cn::seconds(2111);
    auto& data22(src2.data.newEntry());
    data22.data_id = 221;
    data22.length_in_seconds = cn::seconds(2211);

    TSUNIT_ASSERT(ltst.sources.table() == &ltst);
    TSUNIT_ASSERT(ltst.sources[0].data.table() == &ltst);
    TSUNIT_ASSERT(ltst.sources[0].data[0].descs.table() == &ltst);
    TSUNIT_ASSERT(ltst.sources[1].data.table() == &ltst);
    TSUNIT_ASSERT(ltst.sources[1].data[0].descs.table() == &ltst);
    TSUNIT_ASSERT(ltst.sources[1].data[1].descs.table() == &ltst);

    // Test copy constructor.
    ts::LTST ltst2(ltst);
    TSUNIT_EQUAL(1, ltst2.version());
    TSUNIT_EQUAL(2, ltst2.table_id_extension);
    TSUNIT_EQUAL(2, ltst2.tableIdExtension());
    TSUNIT_EQUAL(2, ltst2.sources.size());
    TSUNIT_EQUAL(11, ltst2.sources[0].source_id);
    TSUNIT_EQUAL(1, ltst2.sources[0].data.size());
    TSUNIT_EQUAL(111, ltst2.sources[0].data[0].data_id);
    TSUNIT_EQUAL(1111, ltst2.sources[0].data[0].length_in_seconds.count());
    TSUNIT_EQUAL(22, ltst2.sources[1].source_id);
    TSUNIT_EQUAL(2, ltst2.sources[1].data.size());
    TSUNIT_EQUAL(211, ltst2.sources[1].data[0].data_id);
    TSUNIT_EQUAL(2111, ltst2.sources[1].data[0].length_in_seconds.count());
    TSUNIT_EQUAL(221, ltst2.sources[1].data[1].data_id);
    TSUNIT_EQUAL(2211, ltst2.sources[1].data[1].length_in_seconds.count());

    TSUNIT_ASSERT(ltst2.sources.table() == &ltst2);
    TSUNIT_ASSERT(ltst2.sources[0].data.table() == &ltst2);
    TSUNIT_ASSERT(ltst2.sources[0].data[0].descs.table() == &ltst2);
    TSUNIT_ASSERT(ltst2.sources[1].data.table() == &ltst2);
    TSUNIT_ASSERT(ltst2.sources[1].data[0].descs.table() == &ltst2);
    TSUNIT_ASSERT(ltst2.sources[1].data[1].descs.table() == &ltst2);

    // Test assignment.
    ts::LTST ltst3;
    ltst3 = ltst;
    TSUNIT_EQUAL(1, ltst3.version());
    TSUNIT_EQUAL(2, ltst3.table_id_extension);
    TSUNIT_EQUAL(2, ltst3.tableIdExtension());
    TSUNIT_EQUAL(2, ltst3.sources.size());
    TSUNIT_EQUAL(11, ltst3.sources[0].source_id);
    TSUNIT_EQUAL(1, ltst3.sources[0].data.size());
    TSUNIT_EQUAL(111, ltst3.sources[0].data[0].data_id);
    TSUNIT_EQUAL(1111, ltst3.sources[0].data[0].length_in_seconds.count());
    TSUNIT_EQUAL(22, ltst3.sources[1].source_id);
    TSUNIT_EQUAL(2, ltst3.sources[1].data.size());
    TSUNIT_EQUAL(211, ltst3.sources[1].data[0].data_id);
    TSUNIT_EQUAL(2111, ltst3.sources[1].data[0].length_in_seconds.count());
    TSUNIT_EQUAL(221, ltst3.sources[1].data[1].data_id);
    TSUNIT_EQUAL(2211, ltst3.sources[1].data[1].length_in_seconds.count());

    TSUNIT_ASSERT(ltst3.sources.table() == &ltst3);
    TSUNIT_ASSERT(ltst3.sources[0].data.table() == &ltst3);
    TSUNIT_ASSERT(ltst3.sources[0].data[0].descs.table() == &ltst3);
    TSUNIT_ASSERT(ltst3.sources[1].data.table() == &ltst3);
    TSUNIT_ASSERT(ltst3.sources[1].data[0].descs.table() == &ltst3);
    TSUNIT_ASSERT(ltst3.sources[1].data[1].descs.table() == &ltst3);
}
