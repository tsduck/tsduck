//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class SRTArgs
//
//----------------------------------------------------------------------------

#include "tsSRTArgs.h"
#include "tsReportBuffer.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SRTTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(MinVersion);
    TSUNIT_DECLARE_TEST(URL);
};

TSUNIT_REGISTER(SRTTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(MinVersion)
{
    ts::ReportBuffer<> report;
    ts::SRTArgs args;

    TSUNIT_ASSERT(args.setMinVersion(CERR, u"1.5.2"));
    TSUNIT_ASSERT(args.min_version.has_value());
    TSUNIT_EQUAL(0x010502, *args.min_version);

    args.reset();
    report.clear();
    TSUNIT_ASSERT(!args.min_version.has_value());
    TSUNIT_ASSERT(!args.setMinVersion(report, u"1.256.2"));
    TSUNIT_ASSERT(!args.min_version.has_value());
    TSUNIT_EQUAL(u"Error: invalid SRT minimum version \"1.256.2\"", report.messages());

    args.reset();
    report.clear();
    TSUNIT_ASSERT(!args.setMinVersion(report, u"1.foo"));
    TSUNIT_ASSERT(!args.min_version.has_value());
    TSUNIT_EQUAL(u"Error: invalid SRT minimum version \"1.foo\"", report.messages());
}

TSUNIT_DEFINE_TEST(URL)
{
    ts::SRTArgs args;
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://:1234"));
    TSUNIT_EQUAL(ts::SRTSocketMode::LISTENER, args.mode);
    TSUNIT_ASSERT(!args.local_address.hasAddress());
    TSUNIT_ASSERT(args.local_address.hasPort());
    TSUNIT_ASSERT(!args.remote_address.hasAddress());
    TSUNIT_ASSERT(!args.remote_address.hasPort());
    TSUNIT_EQUAL(u"0.0.0.0:1234", args.local_address.toString());
    TSUNIT_EQUAL(u"0.0.0.0", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:1234"));
    TSUNIT_EQUAL(ts::SRTSocketMode::CALLER, args.mode);
    TSUNIT_ASSERT(!args.local_address.hasAddress());
    TSUNIT_ASSERT(!args.local_address.hasPort());
    TSUNIT_ASSERT(args.remote_address.hasAddress());
    TSUNIT_ASSERT(args.remote_address.hasPort());
    TSUNIT_EQUAL(u"0.0.0.0", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:1234", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:1234?adapter=20.21.22.23"));
    TSUNIT_EQUAL(ts::SRTSocketMode::RENDEZVOUS, args.mode);
    TSUNIT_ASSERT(args.local_address.hasAddress());
    TSUNIT_ASSERT(args.local_address.hasPort());
    TSUNIT_ASSERT(args.remote_address.hasAddress());
    TSUNIT_ASSERT(args.remote_address.hasPort());
    TSUNIT_EQUAL(u"20.21.22.23:1234", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:1234", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.10.10.100:5001?mode=listener"));
    TSUNIT_EQUAL(ts::SRTSocketMode::LISTENER, args.mode);
    TSUNIT_EQUAL(u"10.10.10.100:5001", args.local_address.toString());
    TSUNIT_EQUAL(u"0.0.0.0", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://:5001?adapter=10.10.10.100"));
    TSUNIT_EQUAL(ts::SRTSocketMode::LISTENER, args.mode);
    TSUNIT_EQUAL(u"10.10.10.100:5001", args.local_address.toString());
    TSUNIT_EQUAL(u"0.0.0.0", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:5000?adapter=20.21.22.23&port=4000&mode=caller"));
    TSUNIT_EQUAL(ts::SRTSocketMode::CALLER, args.mode);
    TSUNIT_EQUAL(u"20.21.22.23:4000", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:5000", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:5000?mode=caller&bind=30.31.32.33:4400"));
    TSUNIT_EQUAL(ts::SRTSocketMode::CALLER, args.mode);
    TSUNIT_EQUAL(u"30.31.32.33:4400", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:5000", args.remote_address.toString());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:1234?adapter=20.21.22.23&port=5678"));
    TSUNIT_EQUAL(ts::SRTSocketMode::RENDEZVOUS, args.mode);
    TSUNIT_EQUAL(u"20.21.22.23:5678", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:1234", args.remote_address.toString());
    TSUNIT_EQUAL(0, args.linger_opt.l_onoff);
    TSUNIT_ASSERT(!args.live_mode.has_value());
    TSUNIT_ASSERT(!args.message_api.has_value());

    args.reset();
    TSUNIT_ASSERT(args.setURL(CERR, u"srt://10.11.12.13:1234?conntimeo=3000&fc=40000&linger=4&minversion=1.5.3&packetfilter=foo:bar&transtype=file&messageapi=on"));
    TSUNIT_EQUAL(ts::SRTSocketMode::CALLER, args.mode);
    TSUNIT_EQUAL(u"0.0.0.0", args.local_address.toString());
    TSUNIT_EQUAL(u"10.11.12.13:1234", args.remote_address.toString());
    TSUNIT_ASSERT(args.connection_timeout.has_value());
    TSUNIT_EQUAL(3000, args.connection_timeout->count());
    TSUNIT_ASSERT(args.fc_packets.has_value());
    TSUNIT_EQUAL(40000, *args.fc_packets);
    TSUNIT_EQUAL(1, args.linger_opt.l_onoff);
    TSUNIT_EQUAL(4, args.linger_opt.l_linger);
    TSUNIT_ASSERT(args.min_version.has_value());
    TSUNIT_EQUAL(0x010503, *args.min_version);
    TSUNIT_ASSERT(args.packet_filter.has_value());
    TSUNIT_EQUAL(u"foo:bar", *args.packet_filter);
    TSUNIT_ASSERT(args.live_mode.has_value());
    TSUNIT_ASSERT(!*args.live_mode);
    TSUNIT_ASSERT(args.message_api.has_value());
    TSUNIT_ASSERT(*args.message_api);
}
