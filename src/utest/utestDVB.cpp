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
//  TSUnit test suite for DVB classes
//
//----------------------------------------------------------------------------

#include "tsTunerArgs.h"
#include "tsDuckContext.h"
#include "tsArgs.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DVBTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testTunerArgs();
    void testTunerParams();
    void testLNB();

    TSUNIT_TEST_BEGIN(DVBTest);
    TSUNIT_TEST(testTunerArgs);
    TSUNIT_TEST(testTunerParams);
    TSUNIT_TEST(testLNB);
    TSUNIT_TEST_END();

private:
    static void displayLNB(const ts::LNB& lnb, const ts::UString& name);
    static void testParameters(ts::DeliverySystem delsys);
};

TSUNIT_REGISTER(DVBTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void DVBTest::beforeTest()
{
}

// Test suite cleanup method.
void DVBTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void DVBTest::testTunerArgs()
{
    ts::Args args(u"Test tuner", u"[options]");
    ts::TunerArgs tuner_args;
    tuner_args.defineArgs(args);
    debug() << "DVBTest:: TunerArgs: " << std::endl << args.getHelpText(ts::Args::HELP_FULL) << std::endl;
}

void DVBTest::testParameters(ts::DeliverySystem delsys)
{
    debug() << "DVBTest: Default TunerParameters, type: " << ts::DeliverySystemEnum.name(delsys) << std::endl;

    ts::ModulationArgs params;
    params.delivery_system = delsys;
    params.frequency = 1000000;
    params.setDefaultValues();
    TSUNIT_ASSERT(params.hasModulationArgs());

    const ts::UString opts(params.toPluginOptions());
    debug() << "DVBTest: Options: \"" << opts << "\"" << std::endl;

    ts::Args args;
    ts::TunerArgs targs1;
    targs1.defineArgs(args);

    ts::UStringVector args_vec;
    opts.split(args_vec, u' ');
    TSUNIT_ASSERT(args.analyze(u"", args_vec));

    ts::DuckContext duck;
    TSUNIT_ASSERT(targs1.loadArgs(duck, args));
    TSUNIT_ASSERT(targs1.toPluginOptions() == opts);
}

void DVBTest::testTunerParams()
{
    testParameters(ts::DS_DVB_S);
    testParameters(ts::DS_DVB_C);
    testParameters(ts::DS_DVB_T);
    testParameters(ts::DS_ATSC);
}

void DVBTest::displayLNB(const ts::LNB& lnb, const ts::UString& name)
{
    debug() << "DVBTest: Test LNB: " << name << std::endl
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
    displayLNB(lnb1, u"universal LNB");
    TSUNIT_ASSERT(lnb1.isValid());

    ts::LNB lnb2(u"9000,10000,11000");
    displayLNB(lnb2, u"9000,10000,11000");
    TSUNIT_ASSERT(lnb2.isValid());

    ts::LNB lnb3(u"9500");
    displayLNB(lnb3, u"9500");
    TSUNIT_ASSERT(lnb3.isValid());

    ts::LNB lnb4(u"9500,10000");
    displayLNB(lnb4, u"9500,10000");
    TSUNIT_ASSERT(!lnb4.isValid());

    ts::LNB lnb5(u"azerty");
    displayLNB(lnb5, u"azerty");
    TSUNIT_ASSERT(!lnb5.isValid());
}
