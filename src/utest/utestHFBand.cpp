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
//  TSUnit test suite for HFBand class.
//
//----------------------------------------------------------------------------

#include "tsHFBand.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HFBandTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDefaultRegion();
    void testEmpty();
    void testEurope();
    void testUSA();
    void testVHF();

    TSUNIT_TEST_BEGIN(HFBandTest);
    TSUNIT_TEST(testDefaultRegion);
    TSUNIT_TEST(testEmpty);
    TSUNIT_TEST(testEurope);
    TSUNIT_TEST(testUSA);
    TSUNIT_TEST(testVHF);
    TSUNIT_TEST_END();

private:
    ts::Report& report();
};

TSUNIT_REGISTER(HFBandTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void HFBandTest::beforeTest()
{
}

// Test suite cleanup method.
void HFBandTest::afterTest()
{
}

ts::Report& HFBandTest::report()
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

void HFBandTest::testDefaultRegion()
{
    debug() << "HFBandTest::testDefaultRegion: default region: \"" << ts::HFBand::DefaultRegion(report()) << "\"" << std::endl;
    TSUNIT_ASSERT(!ts::HFBand::DefaultRegion(report()).empty());
}

void HFBandTest::testEmpty()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"zozoland", ts::HFBand::UHF, report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(hf->empty());
    TSUNIT_EQUAL(0, hf->channelCount());
}

void HFBandTest::testEurope()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"Europe", ts::HFBand::UHF, report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(ts::HFBand::UHF, hf->type());
    TSUNIT_EQUAL(49, hf->channelCount());
    TSUNIT_EQUAL(21, hf->firstChannel());
    TSUNIT_EQUAL(69, hf->lastChannel());
    
    TSUNIT_EQUAL(25, hf->nextChannel(24));
    TSUNIT_EQUAL(23, hf->previousChannel(24));
    TSUNIT_EQUAL(498000000, hf->frequency(24));
    TSUNIT_EQUAL(497666668, hf->frequency(24, -2));
    TSUNIT_EQUAL(498333332, hf->frequency(24, +2));
    TSUNIT_EQUAL(24, hf->channelNumber(498000000));
    TSUNIT_EQUAL(24, hf->channelNumber(497666668));
    TSUNIT_EQUAL(24, hf->channelNumber(498333332));
    TSUNIT_EQUAL(0, hf->offsetCount(498000000));
    TSUNIT_EQUAL(-2, hf->offsetCount(497666668));
    TSUNIT_EQUAL(int32_t(+2), hf->offsetCount(498333332));
    TSUNIT_ASSERT(!hf->inBand(200000000, false));
    TSUNIT_ASSERT(!hf->inBand(497666668, true));
    TSUNIT_ASSERT(hf->inBand(498000000, true));
    TSUNIT_ASSERT(hf->inBand(498333332, true));
    TSUNIT_ASSERT(hf->inBand(497666668, false));
    TSUNIT_ASSERT(hf->inBand(498000000, false));
    TSUNIT_ASSERT(hf->inBand(498333332, false));
    TSUNIT_EQUAL(8000000, hf->bandWidth(24));
    TSUNIT_EQUAL(166666, hf->offsetWidth(24));
    TSUNIT_EQUAL(-1, hf->firstOffset(24));
    TSUNIT_EQUAL(3, hf->lastOffset(24));

    TSUNIT_EQUAL(22, hf->nextChannel(21));
    TSUNIT_EQUAL(0, hf->previousChannel(21));

    TSUNIT_EQUAL(0, hf->nextChannel(69));
    TSUNIT_EQUAL(68, hf->previousChannel(69));
}

void HFBandTest::testUSA()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", ts::HFBand::UHF, report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(ts::HFBand::UHF, hf->type());
    TSUNIT_EQUAL(56, hf->channelCount());
    TSUNIT_EQUAL(14, hf->firstChannel());
    TSUNIT_EQUAL(69, hf->lastChannel());

    TSUNIT_EQUAL(25, hf->nextChannel(24));
    TSUNIT_EQUAL(23, hf->previousChannel(24));
    TSUNIT_EQUAL(533000000, hf->frequency(24));
    TSUNIT_EQUAL(533000000, hf->frequency(24, -2));
    TSUNIT_EQUAL(533000000, hf->frequency(24, +2));
    TSUNIT_EQUAL(24, hf->channelNumber(533000000));
    TSUNIT_EQUAL(0, hf->offsetCount(533000000));
    TSUNIT_EQUAL(6000000, hf->bandWidth(24));
    TSUNIT_EQUAL(0, hf->offsetWidth(24));
    TSUNIT_EQUAL(0, hf->firstOffset(24));
    TSUNIT_EQUAL(0, hf->lastOffset(24));

    TSUNIT_EQUAL(15, hf->nextChannel(14));
    TSUNIT_EQUAL(0, hf->previousChannel(14));

    TSUNIT_EQUAL(0, hf->nextChannel(69));
    TSUNIT_EQUAL(68, hf->previousChannel(69));
}

void HFBandTest::testVHF()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", ts::HFBand::VHF, report());
    TSUNIT_ASSERT(hf != nullptr);
    TSUNIT_ASSERT(!hf->empty());
    TSUNIT_EQUAL(ts::HFBand::VHF, hf->type());
    TSUNIT_EQUAL(13, hf->channelCount());
    TSUNIT_EQUAL(1, hf->firstChannel());
    TSUNIT_EQUAL(13, hf->lastChannel());

    TSUNIT_EQUAL(63000000, hf->frequency(3));
    TSUNIT_EQUAL(63000000, hf->frequency(3, -2));
    TSUNIT_EQUAL(63000000, hf->frequency(3, +2));
    TSUNIT_EQUAL(3, hf->channelNumber(63000000));
    TSUNIT_EQUAL(0, hf->offsetCount(63000000));
    TSUNIT_EQUAL(6000000, hf->bandWidth(3));
    TSUNIT_EQUAL(0, hf->offsetWidth(3));
    TSUNIT_EQUAL(0, hf->firstOffset(3));
    TSUNIT_EQUAL(0, hf->lastOffset(3));

    TSUNIT_EQUAL(2, hf->nextChannel(1));
    TSUNIT_EQUAL(0, hf->previousChannel(1));

    TSUNIT_EQUAL(5, hf->nextChannel(4));
    TSUNIT_EQUAL(3, hf->previousChannel(4));

    TSUNIT_EQUAL(6, hf->nextChannel(5));
    TSUNIT_EQUAL(4, hf->previousChannel(5));

    TSUNIT_EQUAL(0, hf->nextChannel(13));
    TSUNIT_EQUAL(12, hf->previousChannel(13));
}
