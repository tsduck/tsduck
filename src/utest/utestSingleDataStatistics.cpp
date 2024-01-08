//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::SingleDataStatistics
//
//----------------------------------------------------------------------------

#include "tsSingleDataStatistics.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SingleDataStatTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testInteger();
    void testChrono();

    TSUNIT_TEST_BEGIN(SingleDataStatTest);
    TSUNIT_TEST(testInteger);
    TSUNIT_TEST(testChrono);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(SingleDataStatTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void SingleDataStatTest::beforeTest()
{
}

// Test suite cleanup method.
void SingleDataStatTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void SingleDataStatTest::testInteger()
{
    ts::SingleDataStatistics<uint32_t> stat;

    stat.feed(11);
    stat.feed(12);
    stat.feed(13);
    stat.feed(14);

    debug() << "SingleDataStatTest::testInteger(): mean: " << stat.meanString()
            << ", var: " << stat.varianceString()
            << ", dev: " << stat.standardDeviationString()
            << std::endl;

    TSUNIT_EQUAL(4, stat.count());
    TSUNIT_EQUAL(11, stat.minimum());
    TSUNIT_EQUAL(14, stat.maximum());
    TSUNIT_EQUAL(13, stat.meanRound());
    TSUNIT_EQUAL(u"12.50", stat.meanString());
    TSUNIT_EQUAL(u"  12.500", stat.meanString(8, 3));
    TSUNIT_EQUAL(u"1.67", stat.varianceString());
    TSUNIT_EQUAL(u"1.29", stat.standardDeviationString());
}

void SingleDataStatTest::testChrono()
{
    ts::SingleDataStatistics<cn::seconds> stat;

    stat.feed(cn::seconds(11));
    stat.feed(cn::milliseconds(12'000));
    stat.feed(cn::microseconds(13'000'000));
    stat.feed(cn::nanoseconds(14'000'000'000));

    debug() << "SingleDataStatTest::testChrono(): mean: " << stat.meanString()
            << ", var: " << stat.varianceString()
            << ", dev: " << stat.standardDeviationString()
            << std::endl;

    TSUNIT_EQUAL(4, stat.count());
    TSUNIT_EQUAL(11, stat.minimum().count());
    TSUNIT_EQUAL(14, stat.maximum().count());
    TSUNIT_EQUAL(13, stat.meanRound().count());
    TSUNIT_EQUAL(u"12.50", stat.meanString());
    TSUNIT_EQUAL(u"  12.500", stat.meanString(8, 3));
    TSUNIT_EQUAL(u"1.67", stat.varianceString());
    TSUNIT_EQUAL(u"1.29", stat.standardDeviationString());
}
