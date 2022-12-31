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

    void testSingleDataStatistics();

    TSUNIT_TEST_BEGIN(SingleDataStatTest);
    TSUNIT_TEST(testSingleDataStatistics);
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

void SingleDataStatTest::testSingleDataStatistics()
{
    ts::SingleDataStatistics<uint32_t> stat;

    stat.feed(11);
    stat.feed(12);
    stat.feed(13);
    stat.feed(14);

    debug() << "SingleDataStatTest::testSingleDataStatistics(): mean: " << stat.meanString()
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
