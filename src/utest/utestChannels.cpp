//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  CppUnit test suite for ts::DuckChannels class.
//
//----------------------------------------------------------------------------

#include "tsChannelFile.h"
#include "tsNullReport.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ChannelsTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testText();

    CPPUNIT_TEST_SUITE(ChannelsTest);
    CPPUNIT_TEST(testText);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ChannelsTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ChannelsTest::setUp()
{
}

// Test suite cleanup method.
void ChannelsTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void ChannelsTest::testText()
{
    static const ts::UChar* const document =
        u"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        u"<tsduck>\n"
        u"  <network id=\"0x1234\" type=\"ATSC\">\n"
        u"    <ts id=\"0x5678\" onid=\"0x9ABC\">\n"
        u"      <atsc frequency=\"123,456\" modulation=\"16-VSB\"/>\n"
        u"      <service id=\"0x0001\"/>\n"
        u"      <service id=\"0x0002\" name=\"Foo Channel\" provider=\"Foo Provider\" LCN=\"23\" PMTPID=\"0x0789\" type=\"0x12\" cas=\"true\"/>\n"
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
        u"      <dvbt frequency=\"548,123\" modulation=\"16-QAM\" HPFEC=\"7/8\" LPFEC=\"3/4\" bandwidth=\"8-MHz\" transmission=\"8K\" guard=\"1/16\" hierarchy=\"2\" PLP=\"7\"/>\n"
        u"      <service id=\"0x0458\"/>\n"
        u"    </ts>\n"
        u"  </network>\n"
        u"</tsduck>\n"
        u"";

    ts::ChannelFile channels;
    CPPUNIT_ASSERT(channels.parse(document));
    CPPUNIT_ASSERT_EQUAL(size_t(4), channels.networkCount());

    ts::ChannelFile::NetworkPtr net;
    ts::ChannelFile::TransportStreamPtr ts;
    ts::ChannelFile::ServicePtr srv;

    CPPUNIT_ASSERT(channels.searchService(net, ts, srv, u"foochannel", false));

    CPPUNIT_ASSERT(!net.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x1234), net->id);
    CPPUNIT_ASSERT_EQUAL(ts::ATSC, net->type);

    CPPUNIT_ASSERT(!ts.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x5678), ts->id);
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x9ABC), ts->onid);
    CPPUNIT_ASSERT(!ts->tune.isNull());
    const ts::TunerParametersATSC* atsc = dynamic_cast<const ts::TunerParametersATSC*>(ts->tune.pointer());
    CPPUNIT_ASSERT(atsc != nullptr);
    CPPUNIT_ASSERT_EQUAL(uint64_t(123456), atsc->frequency);
    CPPUNIT_ASSERT_EQUAL(ts::VSB_16, atsc->modulation);

    CPPUNIT_ASSERT(!srv.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(2), srv->id);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Foo Channel", srv->name);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Foo Provider", srv->provider);
    CPPUNIT_ASSERT(srv->lcn.set());
    CPPUNIT_ASSERT_EQUAL(uint16_t(23), srv->lcn.value());
    CPPUNIT_ASSERT(srv->pmtPID.set());
    CPPUNIT_ASSERT_EQUAL(ts::PID(0x0789), srv->pmtPID.value());
    CPPUNIT_ASSERT(srv->type.set());
    CPPUNIT_ASSERT_EQUAL(uint8_t(0x12), srv->type.value());
    CPPUNIT_ASSERT(srv->cas.set());
    CPPUNIT_ASSERT(srv->cas.value());

    CPPUNIT_ASSERT(channels.searchService(net, ts, srv, ts::DVB_S, u"foochannel", false));

    CPPUNIT_ASSERT(!net.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x8753), net->id);
    CPPUNIT_ASSERT_EQUAL(ts::DVB_S, net->type);

    CPPUNIT_ASSERT(!ts.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x8793), ts->id);
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x5896), ts->onid);
    CPPUNIT_ASSERT(!ts->tune.isNull());
    const ts::TunerParametersDVBS* dvbs = dynamic_cast<const ts::TunerParametersDVBS*>(ts->tune.pointer());
    CPPUNIT_ASSERT(dvbs != nullptr);
    CPPUNIT_ASSERT_EQUAL(uint64_t(8523698), dvbs->frequency);
    CPPUNIT_ASSERT_EQUAL(uint32_t(1237418), dvbs->symbol_rate);
    CPPUNIT_ASSERT_EQUAL(ts::PSK_8, dvbs->modulation);
    CPPUNIT_ASSERT_EQUAL(ts::DS_DVB_S2, dvbs->delivery_system);
    CPPUNIT_ASSERT_EQUAL(ts::POL_HORIZONTAL, dvbs->polarity);
    CPPUNIT_ASSERT_EQUAL(ts::FEC_7_8, dvbs->inner_fec);
    CPPUNIT_ASSERT_EQUAL(ts::PILOT_ON, dvbs->pilots);
    CPPUNIT_ASSERT_EQUAL(ts::ROLLOFF_35, dvbs->roll_off);

    CPPUNIT_ASSERT(!srv.isNull());
    CPPUNIT_ASSERT_EQUAL(uint16_t(0x4591), srv->id);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"Foo Channel", srv->name);
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"", srv->provider);
    CPPUNIT_ASSERT(!srv->lcn.set());
    CPPUNIT_ASSERT(!srv->pmtPID.set());
    CPPUNIT_ASSERT(!srv->type.set());
    CPPUNIT_ASSERT(!srv->cas.set());

    CPPUNIT_ASSERT(!channels.searchService(net, ts, srv, u"foo", false, NULLREP));
    CPPUNIT_ASSERT(net.isNull());
    CPPUNIT_ASSERT(ts.isNull());
    CPPUNIT_ASSERT(srv.isNull());

    CPPUNIT_ASSERT_USTRINGS_EQUAL(document, channels.toXML());
}
