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
//  TSUnit test suite for DVB classes
//
//----------------------------------------------------------------------------

#include "tsTunerArgs.h"
#include "tsDuckContext.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsArgs.h"
#include "tsunit.h"


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
    void testAllLNB();
    void testLNB();
    void testUniversalLNB();
    void testJapanLNB();

    TSUNIT_TEST_BEGIN(DVBTest);
    TSUNIT_TEST(testTunerArgs);
    TSUNIT_TEST(testTunerParams);
    TSUNIT_TEST(testAllLNB);
    TSUNIT_TEST(testLNB);
    TSUNIT_TEST(testUniversalLNB);
    TSUNIT_TEST(testJapanLNB);
    TSUNIT_TEST_END();

private:
    ts::Report& report();
    static void displayLNB(const ts::LNB& lnb);
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

ts::Report& DVBTest::report()
{
    if (tsunit::Test::debugMode()) {
        return CERR;
    }
    else {
        return NULLREP;
    }
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void DVBTest::testTunerArgs()
{
    ts::Args args(u"Test tuner", u"[options]");
    ts::TunerArgs tuner_args;
    tuner_args.defineArgs(args, true);
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
    targs1.defineArgs(args, true);

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

void DVBTest::testAllLNB()
{
    ts::UStringList names(ts::LNB::GetAllNames(report()));
    debug() << "DVBTest::testAllLNB: " << ts::UString::Join(names, u" | ") << std::endl;
    TSUNIT_ASSERT(!names.empty());
}

void DVBTest::displayLNB(const ts::LNB& lnb)
{
    debug() << "DVBTest: Test LNB: name: \"" << lnb.name() << "\"" << std::endl
            << "    convert to string: \"" << lnb << "\"" << std::endl
            << "    valid: " << ts::UString::TrueFalse(lnb.isValid()) << std::endl
            << "    number of bands: " << lnb.bandsCount() << std::endl
            << "    polarization-controlled: " << ts::UString::TrueFalse(lnb.isPolarizationControlled()) << std::endl
            << "    legacyLowOscillatorFrequency: " << lnb.legacyLowOscillatorFrequency() << std::endl
            << "    legacyHighOscillatorFrequency: " << lnb.legacyHighOscillatorFrequency() << std::endl
            << "    legacySwitchFrequency: " << lnb.legacySwitchFrequency() << std::endl;
}

void DVBTest::testLNB()
{
    ts::LNB lnb1(u"", report());
    debug() << "DVBTest::testLNB(): default LNB:" << std::endl;
    displayLNB(lnb1);
    TSUNIT_ASSERT(lnb1.isValid());

    ts::LNB lnb2(u"9000,10000,11000", report());
    displayLNB(lnb2);
    TSUNIT_ASSERT(lnb2.isValid());

    ts::LNB lnb3(u"9500", report());
    displayLNB(lnb3);
    TSUNIT_ASSERT(lnb3.isValid());

    ts::LNB lnb4(u"9500,10000", report());
    displayLNB(lnb4);
    TSUNIT_ASSERT(!lnb4.isValid());

    ts::LNB lnb5(u"azerty", report());
    displayLNB(lnb5);
    TSUNIT_ASSERT(!lnb5.isValid());
}

void DVBTest::testUniversalLNB()
{
    ts::LNB lnb(u"universal", report());
    displayLNB(lnb);

    TSUNIT_ASSERT(lnb.isValid());
    TSUNIT_ASSERT(!lnb.isPolarizationControlled());
    TSUNIT_EQUAL(2, lnb.bandsCount());
    TSUNIT_EQUAL(TS_UCONST64(9750000000), lnb.legacyLowOscillatorFrequency());
    TSUNIT_EQUAL(TS_UCONST64(10600000000), lnb.legacyHighOscillatorFrequency());
    TSUNIT_EQUAL(TS_UCONST64(11700000000), lnb.legacySwitchFrequency());

    ts::LNB::Transposition tr;

    TSUNIT_ASSERT(lnb.transpose(tr, TS_UCONST64(11000000000), ts::POL_AUTO, report()));
    TSUNIT_EQUAL(TS_UCONST64(11000000000), tr.satellite_frequency);
    TSUNIT_EQUAL(TS_UCONST64(1250000000), tr.intermediate_frequency);
    TSUNIT_EQUAL(TS_UCONST64(9750000000), tr.oscillator_frequency);
    TSUNIT_ASSERT(!tr.stacked);
    TSUNIT_EQUAL(0, tr.band_index);

    TSUNIT_ASSERT(lnb.transpose(tr, TS_UCONST64(12000000000), ts::POL_AUTO, report()));
    TSUNIT_EQUAL(TS_UCONST64(12000000000), tr.satellite_frequency);
    TSUNIT_EQUAL(TS_UCONST64(1400000000), tr.intermediate_frequency);
    TSUNIT_EQUAL(TS_UCONST64(10600000000), tr.oscillator_frequency);
    TSUNIT_ASSERT(!tr.stacked);
    TSUNIT_EQUAL(1, tr.band_index);

    // Outside bands
    TSUNIT_ASSERT(!lnb.transpose(tr, TS_UCONST64(8000000000), ts::POL_AUTO, report()));
}

void DVBTest::testJapanLNB()
{
    ts::LNB lnb(u"japan", report());
    displayLNB(lnb);

    TSUNIT_ASSERT(lnb.isValid());
    TSUNIT_ASSERT(lnb.isPolarizationControlled());
    TSUNIT_EQUAL(2, lnb.bandsCount());
    TSUNIT_EQUAL(0, lnb.legacyLowOscillatorFrequency());
    TSUNIT_EQUAL(0, lnb.legacyHighOscillatorFrequency());
    TSUNIT_EQUAL(0, lnb.legacySwitchFrequency());

    ts::LNB::Transposition tr;

    // Channel BS-15
    TSUNIT_ASSERT(lnb.transpose(tr, TS_UCONST64(11996000000), ts::POL_RIGHT, report()));
    TSUNIT_EQUAL(TS_UCONST64(11996000000), tr.satellite_frequency);
    TSUNIT_EQUAL(TS_UCONST64(1318000000), tr.intermediate_frequency);
    TSUNIT_EQUAL(TS_UCONST64(10678000000), tr.oscillator_frequency);
    TSUNIT_ASSERT(tr.stacked);

    // Channel ND-15
    TSUNIT_ASSERT(lnb.transpose(tr, TS_UCONST64(12551000000), ts::POL_LEFT, report()));
    TSUNIT_EQUAL(TS_UCONST64(12551000000), tr.satellite_frequency);
    TSUNIT_EQUAL(TS_UCONST64(3046000000), tr.intermediate_frequency);
    TSUNIT_EQUAL(TS_UCONST64(9505000000), tr.oscillator_frequency);
    TSUNIT_ASSERT(tr.stacked);

    // Need polarization.
    TSUNIT_ASSERT(!lnb.transpose(tr, TS_UCONST64(12551000000), ts::POL_NONE, report()));

    // Outside bands
    TSUNIT_ASSERT(!lnb.transpose(tr, TS_UCONST64(11000000000), ts::POL_RIGHT, report()));
}
