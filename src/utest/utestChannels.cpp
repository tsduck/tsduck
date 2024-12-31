//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ts::DuckChannels class.
//
//----------------------------------------------------------------------------

#include "tsChannelFile.h"
#include "tsNullReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ChannelsTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Text);
};

TSUNIT_REGISTER(ChannelsTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Text)
{
    static const ts::UChar* const document =
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<tsduck>\n"
        u"  <network id=\"0x1234\" type=\"ATSC\">\n"
        u"    <ts id=\"0x5678\" onid=\"0x9ABC\">\n"
        u"      <atsc frequency=\"123,456\" modulation=\"16-VSB\"/>\n"
        u"      <service id=\"0x0001\" atsc_major_id=\"1\" atsc_minor_id=\"3\"/>\n"
        u"      <service id=\"0x0002\" name=\"Foo Channel\" provider=\"Foo Provider\" LCN=\"23\" PMTPID=\"0x0789\" type=\"0x12\" cas=\"true\" atsc_major_id=\"1\" atsc_minor_id=\"4\"/>\n"
        u"    </ts>\n"
        u"  </network>\n"
        u"  <network id=\"0x7883\" type=\"DVB-C\">\n"
        u"    <ts id=\"0x7890\" onid=\"0x7412\">\n"
        u"      <dvbc frequency=\"789,654,123\" symbolrate=\"6,900,000\" modulation=\"64-QAM\"/>\n"
        u"      <service id=\"0x0056\"/>\n"
        u"      <service id=\"0x0879\"/>\n"
        u"      <service id=\"0x7895\"/>\n"
        u"    </ts>\n"
        u"    <ts id=\"0x7896\" onid=\"0x7412\">\n"
        u"      <dvbc frequency=\"1,236,987\" symbolrate=\"456,987\" modulation=\"32-QAM\" FEC=\"5/6\" inversion=\"off\"/>\n"
        u"      <service id=\"0x0102\" name=\"Azerty\" LCN=\"48\" PMTPID=\"0x1368\" type=\"0x78\" cas=\"false\"/>\n"
        u"    </ts>\n"
        u"  </network>\n"
        u"  <network id=\"0x8753\" type=\"DVB-S\">\n"
        u"    <ts id=\"0x8793\" onid=\"0x5896\">\n"
        u"      <dvbs frequency=\"8,523,698\" symbolrate=\"1,237,418\" modulation=\"8-PSK\" system=\"DVB-S2\" polarity=\"horizontal\" FEC=\"7/8\" pilots=\"on\" rolloff=\"0.35\"/>\n"
        u"      <service id=\"0x4591\" name=\"Foo Channel\"/>\n"
        u"    </ts>\n"
        u"  </network>\n"
        u"  <network id=\"0x5934\" type=\"DVB-T\">\n"
        u"    <ts id=\"0x7843\" onid=\"0x2596\">\n"
        u"      <dvbt frequency=\"548,123\" modulation=\"16-QAM\" HPFEC=\"7/8\" LPFEC=\"3/4\" bandwidth=\"8,000,000\" transmission=\"8K\" guard=\"1/16\" hierarchy=\"2\" PLP=\"7\"/>\n"
        u"      <service id=\"0x0458\"/>\n"
        u"    </ts>\n"
        u"  </network>\n"
        u"</tsduck>\n"
        u"";

    ts::ChannelFile channels;
    TSUNIT_ASSERT(channels.parse(document));
    TSUNIT_EQUAL(4, channels.networkCount());

    ts::ChannelFile::NetworkPtr net;
    ts::ChannelFile::TransportStreamPtr ts;
    ts::ChannelFile::ServicePtr srv;

    TSUNIT_ASSERT(channels.searchService(net, ts, srv, u"foochannel", false));

    TSUNIT_ASSERT(net != nullptr);
    TSUNIT_EQUAL(0x1234, net->id);
    TSUNIT_EQUAL(ts::TT_ATSC, net->type);

    TSUNIT_ASSERT(ts != nullptr);
    TSUNIT_EQUAL(0x5678, ts->id);
    TSUNIT_EQUAL(0x9ABC, ts->onid);
    TSUNIT_ASSERT(ts->tune.hasModulationArgs());
    TSUNIT_ASSERT(ts->tune.frequency.has_value());
    TSUNIT_EQUAL(123456, ts->tune.frequency.value());
    TSUNIT_ASSERT(ts->tune.modulation.has_value());
    TSUNIT_EQUAL(ts::VSB_16, ts->tune.modulation.value());

    TSUNIT_ASSERT(srv != nullptr);
    TSUNIT_EQUAL(2, srv->id);
    TSUNIT_EQUAL(u"Foo Channel", srv->name);
    TSUNIT_EQUAL(u"Foo Provider", srv->provider);
    TSUNIT_ASSERT(srv->lcn.has_value());
    TSUNIT_EQUAL(23, srv->lcn.value());
    TSUNIT_ASSERT(srv->pmtPID.has_value());
    TSUNIT_EQUAL(0x0789, srv->pmtPID.value());
    TSUNIT_ASSERT(srv->type.has_value());
    TSUNIT_EQUAL(0x12, srv->type.value());
    TSUNIT_ASSERT(srv->cas.has_value());
    TSUNIT_ASSERT(srv->cas.value());

    TSUNIT_ASSERT(channels.searchService(net, ts, srv, ts::DeliverySystemSet({ts::DS_DVB_S, ts::DS_DVB_S2}), u"foochannel", false));

    TSUNIT_ASSERT(net != nullptr);
    TSUNIT_EQUAL(0x8753, net->id);
    TSUNIT_EQUAL(ts::TT_DVB_S, net->type);

    TSUNIT_ASSERT(ts != nullptr);
    TSUNIT_EQUAL(0x8793, ts->id);
    TSUNIT_EQUAL(0x5896, ts->onid);
    TSUNIT_ASSERT(ts->tune.hasModulationArgs());
    TSUNIT_ASSERT(ts->tune.frequency.has_value());
    TSUNIT_EQUAL(8523698, ts->tune.frequency.value());
    TSUNIT_ASSERT(ts->tune.symbol_rate.has_value());
    TSUNIT_EQUAL(1237418, ts->tune.symbol_rate.value());
    TSUNIT_ASSERT(ts->tune.modulation.has_value());
    TSUNIT_EQUAL(ts::PSK_8, ts->tune.modulation.value());
    TSUNIT_ASSERT(ts->tune.delivery_system.has_value());
    TSUNIT_EQUAL(ts::DS_DVB_S2, ts->tune.delivery_system.value());
    TSUNIT_ASSERT(ts->tune.polarity.has_value());
    TSUNIT_EQUAL(ts::POL_HORIZONTAL, ts->tune.polarity.value());
    TSUNIT_ASSERT(ts->tune.inner_fec.has_value());
    TSUNIT_EQUAL(ts::FEC_7_8, ts->tune.inner_fec.value());
    TSUNIT_ASSERT(ts->tune.pilots.has_value());
    TSUNIT_EQUAL(ts::PILOT_ON, ts->tune.pilots.value());
    TSUNIT_ASSERT(ts->tune.roll_off.has_value());
    TSUNIT_EQUAL(ts::ROLLOFF_35, ts->tune.roll_off.value());

    TSUNIT_ASSERT(srv != nullptr);
    TSUNIT_EQUAL(0x4591, srv->id);
    TSUNIT_EQUAL(u"Foo Channel", srv->name);
    TSUNIT_EQUAL(u"", srv->provider);
    TSUNIT_ASSERT(!srv->lcn.has_value());
    TSUNIT_ASSERT(!srv->pmtPID.has_value());
    TSUNIT_ASSERT(!srv->type.has_value());
    TSUNIT_ASSERT(!srv->cas.has_value());

    TSUNIT_ASSERT(!channels.searchService(net, ts, srv, u"foo", false, NULLREP));
    TSUNIT_ASSERT(net == nullptr);
    TSUNIT_ASSERT(ts == nullptr);
    TSUNIT_ASSERT(srv == nullptr);

    // Search by ATSC major.minor
    TSUNIT_ASSERT(channels.searchService(net, ts, srv, u"1.4", false));

    TSUNIT_ASSERT(net != nullptr);
    TSUNIT_EQUAL(0x1234, net->id);
    TSUNIT_EQUAL(ts::TT_ATSC, net->type);

    TSUNIT_ASSERT(ts != nullptr);
    TSUNIT_EQUAL(0x5678, ts->id);
    TSUNIT_EQUAL(0x9ABC, ts->onid);
    TSUNIT_ASSERT(ts->tune.hasModulationArgs());
    TSUNIT_ASSERT(ts->tune.frequency.has_value());
    TSUNIT_EQUAL(123456, ts->tune.frequency.value());
    TSUNIT_ASSERT(ts->tune.modulation.has_value());
    TSUNIT_EQUAL(ts::VSB_16, ts->tune.modulation.value());

    TSUNIT_ASSERT(srv != nullptr);
    TSUNIT_EQUAL(2, srv->id);
    TSUNIT_EQUAL(u"Foo Channel", srv->name);
    TSUNIT_EQUAL(u"Foo Provider", srv->provider);
    TSUNIT_ASSERT(srv->lcn.has_value());
    TSUNIT_EQUAL(23, srv->lcn.value());
    TSUNIT_ASSERT(srv->pmtPID.has_value());
    TSUNIT_EQUAL(ts::PID(0x0789), srv->pmtPID.value());
    TSUNIT_ASSERT(srv->type.has_value());
    TSUNIT_EQUAL(0x12, srv->type.value());
    TSUNIT_ASSERT(srv->cas.has_value());
    TSUNIT_ASSERT(srv->cas.value());
    TSUNIT_ASSERT(srv->atscMajorId.has_value());
    TSUNIT_EQUAL(1, srv->atscMajorId.value());
    TSUNIT_ASSERT(srv->atscMinorId.has_value());
    TSUNIT_EQUAL(4, srv->atscMinorId.value());

    TSUNIT_ASSERT(!channels.searchService(net, ts, srv, u"1.5", false, NULLREP));
    TSUNIT_ASSERT(net == nullptr);
    TSUNIT_ASSERT(ts == nullptr);
    TSUNIT_ASSERT(srv == nullptr);

    TSUNIT_EQUAL(document, channels.toXML());
}
