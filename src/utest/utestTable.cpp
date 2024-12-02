//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
#include "tsCADescriptor.h"
#include "tsAVCVideoDescriptor.h"
#include "tsDVBAC3Descriptor.h"
#include "tsEacemPreferredNameIdentifierDescriptor.h"
#include "tsEacemLogicalChannelNumberDescriptor.h"
#include "tsEutelsatChannelNumberDescriptor.h"
#include "tsDuckContext.h"
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
    TSUNIT_ASSERT(pmt2.is_current);
    TSUNIT_EQUAL(1, pmt2.version);
    TSUNIT_EQUAL(27, pmt2.service_id);
    TSUNIT_EQUAL(1001, pmt2.pcr_pid);

    TSUNIT_EQUAL(ts::TID_PMT, pmt2.descs.tableId());
    TSUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    TSUNIT_EQUAL(1, pmt2.descs.count());
    TSUNIT_ASSERT(pmt2.descs[0]->isValid());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.descs[0]->tag());

    TSUNIT_EQUAL(2, pmt2.streams.size());

    TSUNIT_EQUAL(45, pmt2.streams[3003].stream_type);
    TSUNIT_EQUAL(1, pmt2.streams[3003].descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_AVC_VIDEO, pmt2.streams[3003].descs[0]->tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[3003].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    TSUNIT_EQUAL(149, pmt2.streams[4004].stream_type);
    TSUNIT_EQUAL(2, pmt2.streams[4004].descs.count());
    TSUNIT_EQUAL(ts::DID_DVB_AC3, pmt2.streams[4004].descs[0]->tag());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.streams[4004].descs[1]->tag());
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
    TSUNIT_ASSERT(pmt2.is_current);
    TSUNIT_EQUAL(1, pmt2.version);
    TSUNIT_EQUAL(27, pmt2.service_id);
    TSUNIT_EQUAL(1001, pmt2.pcr_pid);

    TSUNIT_EQUAL(ts::TID_PMT, pmt2.descs.tableId());
    TSUNIT_ASSERT(pmt2.descs.table() == &pmt2);
    TSUNIT_EQUAL(1, pmt2.descs.count());
    TSUNIT_ASSERT(pmt2.descs[0]->isValid());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.descs[0]->tag());

    TSUNIT_EQUAL(2, pmt2.streams.size());

    TSUNIT_EQUAL(45, pmt2.streams[3003].stream_type);
    TSUNIT_EQUAL(1, pmt2.streams[3003].descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_AVC_VIDEO, pmt2.streams[3003].descs[0]->tag());
    TSUNIT_EQUAL(ts::TID_PMT, pmt2.streams[3003].descs.tableId());
    TSUNIT_ASSERT(pmt2.streams[3003].descs.table() == &pmt2);

    TSUNIT_EQUAL(149, pmt2.streams[4004].stream_type);
    TSUNIT_EQUAL(2, pmt2.streams[4004].descs.count());
    TSUNIT_EQUAL(ts::DID_DVB_AC3, pmt2.streams[4004].descs[0]->tag());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, pmt2.streams[4004].descs[1]->tag());
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
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot1.descs[0]->tag());

    ts::TOT tot2(tot1);
    TSUNIT_ASSERT(tot2.descs.table() == &tot2);
    TSUNIT_EQUAL(1, tot2.descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot2.descs[0]->tag());

    ts::TOT tot3;
    tot3 = tot1;
    TSUNIT_ASSERT(tot3.descs.table() == &tot3);
    TSUNIT_EQUAL(1, tot2.descs.count());
    TSUNIT_EQUAL(ts::DID_MPEG_CA, tot2.descs[0]->tag());
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
    TSUNIT_EQUAL(ts::DID_DVB_SERVICE, dlist[0]->tag());
}
