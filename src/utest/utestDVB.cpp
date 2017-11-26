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
//  CppUnit test suite for DVB classes
//
//----------------------------------------------------------------------------

#include "tsLNB.h"
#include "tsTunerParametersDVBS.h"
#include "tsTunerParametersDVBC.h"
#include "tsTunerParametersDVBT.h"
#include "tsTunerUtils.h"
#include "tsTunerArgs.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DVBTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testTunerArgs();
    void testZapFiles();
    void testTunerParams();
    void testLNB();
    void testUHF();

    CPPUNIT_TEST_SUITE(DVBTest);
    CPPUNIT_TEST(testTunerArgs);
    CPPUNIT_TEST(testZapFiles);
    CPPUNIT_TEST(testTunerParams);
    CPPUNIT_TEST(testLNB);
    CPPUNIT_TEST(testUHF);
    CPPUNIT_TEST_SUITE_END();

private:
    static void displayLNB(const ts::LNB& lnb, const ts::UString& name);
    static void testParameters(const ts::TunerParameters& params);
};

CPPUNIT_TEST_SUITE_REGISTRATION(DVBTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DVBTest::setUp()
{
}

// Test suite cleanup method.
void DVBTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void DVBTest::testTunerArgs()
{
    ts::Args args("Test tuner", "[options]", "<help intro>\n");
    ts::TunerArgs tuner_args;
    tuner_args.addHelp(args);
    utest::Out() << "DVBTest:: TunerArgs: " << std::endl << args.getHelp() << std::endl;
}

void DVBTest::testZapFiles()
{
    utest::Out() << "DVBTest: DefaultZapFile(DVB_S): " << ts::TunerArgs::DefaultZapFile(ts::DVB_S) << std::endl
                 << "DVBTest: DefaultZapFile(DVB_C): " << ts::TunerArgs::DefaultZapFile(ts::DVB_C) << std::endl
                 << "DVBTest: DefaultZapFile(DVB_T): " << ts::TunerArgs::DefaultZapFile(ts::DVB_T) << std::endl
                 << "DVBTest: DefaultZapFile(ATSC):  " << ts::TunerArgs::DefaultZapFile(ts::ATSC)  << std::endl;
}

void DVBTest::testParameters(const ts::TunerParameters& params)
{
    utest::Out() << "DVBTest: Default TunerParameters, type: " << ts::TunerTypeEnum.name(params.tunerType()) << std::endl;

    ts::TunerParametersPtr ptr(ts::TunerParameters::Factory(params.tunerType()));
    CPPUNIT_ASSERT(ptr->tunerType() == params.tunerType());

    const ts::UString zap(params.toZapFormat());
    utest::Out() << "DVBTest: Zap format: \"" << zap << "\"" << std::endl;

    const ts::UString opts(params.toPluginOptions());
    utest::Out() << "DVBTest: Options: \"" << opts << "\"" << std::endl;

    CPPUNIT_ASSERT(ptr->fromZapFormat(zap));
    CPPUNIT_ASSERT(ptr->toZapFormat() == zap);
    CPPUNIT_ASSERT(ptr->toPluginOptions() == opts);

    ts::Args args;
    ts::TunerArgs tuner_args;
    tuner_args.defineOptions(args);
    ts::UStringVector args_vec;
    opts.split(args_vec, u' ');
    CPPUNIT_ASSERT(args.analyze("", args_vec));

    ts::TunerArgs tuner;
    tuner.load(args);
    ptr = ts::TunerParameters::Factory(params.tunerType());
    CPPUNIT_ASSERT(ptr->tunerType() == params.tunerType());
    CPPUNIT_ASSERT(ptr->fromTunerArgs(tuner, args));
    CPPUNIT_ASSERT(ptr->toZapFormat() == zap);
    CPPUNIT_ASSERT(ptr->toPluginOptions() == opts);
}

void DVBTest::testTunerParams()
{
    testParameters(ts::TunerParametersDVBS());
    testParameters(ts::TunerParametersDVBC());
    testParameters(ts::TunerParametersDVBT());
}

void DVBTest::displayLNB(const ts::LNB& lnb, const ts::UString& name)
{
    utest::Out() << "DVBTest: Test LNB: " << name << std::endl
                 << "  convert to string: " << lnb << std::endl
                 << "  hasHighBand: " << lnb.hasHighBand() << std::endl
                 << "  lowFrequency: " << lnb.lowFrequency() << std::endl
                 << "  highFrequency: " << lnb.highFrequency() << std::endl
                 << "  switchFrequency: " << lnb.switchFrequency() << std::endl
                 << "  useHighBand (8 GHz): " << lnb.useHighBand(TS_UCONST64(8000000000)) << std::endl
                 << "  intermediateFrequency (8 GHz): " << lnb.intermediateFrequency(TS_UCONST64(8000000000)) << std::endl
                 << "  useHighBand (12 GHz): " << lnb.useHighBand(TS_UCONST64(12000000000)) << std::endl
                 << "  intermediateFrequency (12 GHz): " << lnb.intermediateFrequency(TS_UCONST64(12000000000)) << std::endl;
}

void DVBTest::testLNB()
{
    ts::LNB lnb1;
    displayLNB(lnb1, "universal LNB");
    CPPUNIT_ASSERT(lnb1.isValid());

    ts::LNB lnb2("9000,10000,11000");
    displayLNB(lnb2, "9000,10000,11000");
    CPPUNIT_ASSERT(lnb2.isValid());

    ts::LNB lnb3("9500");
    displayLNB(lnb3, "9500");
    CPPUNIT_ASSERT(lnb3.isValid());

    ts::LNB lnb4("9500,10000");
    displayLNB(lnb4, "9500,10000");
    CPPUNIT_ASSERT(!lnb4.isValid());

    ts::LNB lnb5("azerty");
    displayLNB(lnb5, "azerty");
    CPPUNIT_ASSERT(!lnb5.isValid());
}

void DVBTest::testUHF()
{
    CPPUNIT_ASSERT_EQUAL(uint64_t(474000000), ts::UHF::Frequency(21, 0));
    CPPUNIT_ASSERT_EQUAL(uint64_t(474166666), ts::UHF::Frequency(21, 1));
    CPPUNIT_ASSERT_EQUAL(uint64_t(561833334), ts::UHF::Frequency(32, -1));
    CPPUNIT_ASSERT_EQUAL(uint64_t(858000000), ts::UHF::Frequency(69, 0));

    CPPUNIT_ASSERT_EQUAL(21, ts::UHF::Channel(474000000));
    CPPUNIT_ASSERT_EQUAL(21, ts::UHF::Channel(474166666));
    CPPUNIT_ASSERT_EQUAL(32, ts::UHF::Channel(561833334));
    CPPUNIT_ASSERT_EQUAL(69, ts::UHF::Channel(858000000));

    CPPUNIT_ASSERT_EQUAL(0, ts::UHF::OffsetCount(474000000));
    CPPUNIT_ASSERT_EQUAL(1, ts::UHF::OffsetCount(474166666));
    CPPUNIT_ASSERT_EQUAL(-1, ts::UHF::OffsetCount(561833334));
    CPPUNIT_ASSERT_EQUAL(0, ts::UHF::OffsetCount(858000000));
}
