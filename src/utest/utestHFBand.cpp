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
//  CppUnit test suite for HFBand class.
//
//----------------------------------------------------------------------------

#include "tsHFBand.h"
#include "tsCerrReport.h"
#include "tsNullReport.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HFBandTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testDefaultRegion();
    void testEmpty();
    void testEurope();
    void testUSA();
    void testVHF();

    CPPUNIT_TEST_SUITE(HFBandTest);
    CPPUNIT_TEST(testDefaultRegion);
    CPPUNIT_TEST(testEmpty);
    CPPUNIT_TEST(testEurope);
    CPPUNIT_TEST(testUSA);
    CPPUNIT_TEST(testVHF);
    CPPUNIT_TEST_SUITE_END();

private:
    ts::Report& report();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HFBandTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void HFBandTest::setUp()
{
}

// Test suite cleanup method.
void HFBandTest::tearDown()
{
}

ts::Report& HFBandTest::report()
{
    if (utest::DebugMode()) {
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
    utest::Out() << "HFBandTest::testDefaultRegion: default region: \"" << ts::HFBand::DefaultRegion(report()) << "\"" << std::endl;
    CPPUNIT_ASSERT(!ts::HFBand::DefaultRegion(report()).empty());
}

void HFBandTest::testEmpty()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"zozoland", ts::HFBand::UHF, report());
    CPPUNIT_ASSERT(hf != nullptr);
    CPPUNIT_ASSERT(hf->empty());
    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->channelCount());
}

void HFBandTest::testEurope()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"Europe", ts::HFBand::UHF, report());
    CPPUNIT_ASSERT(hf != nullptr);
    CPPUNIT_ASSERT(!hf->empty());
    CPPUNIT_ASSERT_EQUAL(ts::HFBand::BandType(ts::HFBand::UHF), hf->type());
    CPPUNIT_ASSERT_EQUAL(uint32_t(49), hf->channelCount());
    CPPUNIT_ASSERT_EQUAL(uint32_t(21), hf->firstChannel());
    CPPUNIT_ASSERT_EQUAL(uint32_t(69), hf->lastChannel());
    
    CPPUNIT_ASSERT_EQUAL(uint32_t(25), hf->nextChannel(24));
    CPPUNIT_ASSERT_EQUAL(uint32_t(23), hf->previousChannel(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(498000000), hf->frequency(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(497666668), hf->frequency(24, -2));
    CPPUNIT_ASSERT_EQUAL(uint64_t(498333332), hf->frequency(24, +2));
    CPPUNIT_ASSERT_EQUAL(uint32_t(24), hf->channelNumber(498000000));
    CPPUNIT_ASSERT_EQUAL(uint32_t(24), hf->channelNumber(497666668));
    CPPUNIT_ASSERT_EQUAL(uint32_t(24), hf->channelNumber(498333332));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->offsetCount(498000000));
    CPPUNIT_ASSERT_EQUAL(int32_t(-2), hf->offsetCount(497666668));
    CPPUNIT_ASSERT_EQUAL(int32_t(+2), hf->offsetCount(498333332));
    CPPUNIT_ASSERT(!hf->inBand(200000000, false));
    CPPUNIT_ASSERT(!hf->inBand(497666668, true));
    CPPUNIT_ASSERT(hf->inBand(498000000, true));
    CPPUNIT_ASSERT(hf->inBand(498333332, true));
    CPPUNIT_ASSERT(hf->inBand(497666668, false));
    CPPUNIT_ASSERT(hf->inBand(498000000, false));
    CPPUNIT_ASSERT(hf->inBand(498333332, false));
    CPPUNIT_ASSERT_EQUAL(uint64_t(8000000), hf->bandWidth(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(166666), hf->offsetWidth(24));
    CPPUNIT_ASSERT_EQUAL(int32_t(-1), hf->firstOffset(24));
    CPPUNIT_ASSERT_EQUAL(int32_t(3), hf->lastOffset(24));

    CPPUNIT_ASSERT_EQUAL(uint32_t(22), hf->nextChannel(21));
    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->previousChannel(21));

    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->nextChannel(69));
    CPPUNIT_ASSERT_EQUAL(uint32_t(68), hf->previousChannel(69));
}

void HFBandTest::testUSA()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", ts::HFBand::UHF, report());
    CPPUNIT_ASSERT(hf != nullptr);
    CPPUNIT_ASSERT(!hf->empty());
    CPPUNIT_ASSERT_EQUAL(ts::HFBand::BandType(ts::HFBand::UHF), hf->type());
    CPPUNIT_ASSERT_EQUAL(uint32_t(56), hf->channelCount());
    CPPUNIT_ASSERT_EQUAL(uint32_t(14), hf->firstChannel());
    CPPUNIT_ASSERT_EQUAL(uint32_t(69), hf->lastChannel());

    CPPUNIT_ASSERT_EQUAL(uint32_t(25), hf->nextChannel(24));
    CPPUNIT_ASSERT_EQUAL(uint32_t(23), hf->previousChannel(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(533000000), hf->frequency(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(533000000), hf->frequency(24, -2));
    CPPUNIT_ASSERT_EQUAL(uint64_t(533000000), hf->frequency(24, +2));
    CPPUNIT_ASSERT_EQUAL(uint32_t(24), hf->channelNumber(533000000));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->offsetCount(533000000));
    CPPUNIT_ASSERT_EQUAL(uint64_t(6000000), hf->bandWidth(24));
    CPPUNIT_ASSERT_EQUAL(uint64_t(0), hf->offsetWidth(24));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->firstOffset(24));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->lastOffset(24));

    CPPUNIT_ASSERT_EQUAL(uint32_t(15), hf->nextChannel(14));
    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->previousChannel(14));

    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->nextChannel(69));
    CPPUNIT_ASSERT_EQUAL(uint32_t(68), hf->previousChannel(69));
}

void HFBandTest::testVHF()
{
    const ts::HFBand* hf = ts::HFBand::GetBand(u"USA", ts::HFBand::VHF, report());
    CPPUNIT_ASSERT(hf != nullptr);
    CPPUNIT_ASSERT(!hf->empty());
    CPPUNIT_ASSERT_EQUAL(ts::HFBand::BandType(ts::HFBand::VHF), hf->type());
    CPPUNIT_ASSERT_EQUAL(uint32_t(13), hf->channelCount());
    CPPUNIT_ASSERT_EQUAL(uint32_t(1), hf->firstChannel());
    CPPUNIT_ASSERT_EQUAL(uint32_t(13), hf->lastChannel());

    CPPUNIT_ASSERT_EQUAL(uint64_t(63000000), hf->frequency(3));
    CPPUNIT_ASSERT_EQUAL(uint64_t(63000000), hf->frequency(3, -2));
    CPPUNIT_ASSERT_EQUAL(uint64_t(63000000), hf->frequency(3, +2));
    CPPUNIT_ASSERT_EQUAL(uint32_t(3), hf->channelNumber(63000000));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->offsetCount(63000000));
    CPPUNIT_ASSERT_EQUAL(uint64_t(6000000), hf->bandWidth(3));
    CPPUNIT_ASSERT_EQUAL(uint64_t(0), hf->offsetWidth(3));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->firstOffset(3));
    CPPUNIT_ASSERT_EQUAL(int32_t(0), hf->lastOffset(3));

    CPPUNIT_ASSERT_EQUAL(uint32_t(2), hf->nextChannel(1));
    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->previousChannel(1));

    CPPUNIT_ASSERT_EQUAL(uint32_t(5), hf->nextChannel(4));
    CPPUNIT_ASSERT_EQUAL(uint32_t(3), hf->previousChannel(4));

    CPPUNIT_ASSERT_EQUAL(uint32_t(6), hf->nextChannel(5));
    CPPUNIT_ASSERT_EQUAL(uint32_t(4), hf->previousChannel(5));

    CPPUNIT_ASSERT_EQUAL(uint32_t(0), hf->nextChannel(13));
    CPPUNIT_ASSERT_EQUAL(uint32_t(12), hf->previousChannel(13));
}
