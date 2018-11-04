//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  CppUnit test suite for HLS classes.
//
//----------------------------------------------------------------------------

#include "tshlsPlayList.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class HLSTest: public CppUnit::TestFixture
{
public:
    HLSTest();

    virtual void setUp() override;
    virtual void tearDown() override;

    void testMasterPlaylist();

    CPPUNIT_TEST_SUITE(HLSTest);
    CPPUNIT_TEST(testMasterPlaylist);
    CPPUNIT_TEST_SUITE_END();

private:
    int _previousSeverity;
};

CPPUNIT_TEST_SUITE_REGISTRATION(HLSTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
HLSTest::HLSTest() :
    _previousSeverity(0)
{
}

// Test suite initialization method.
void HLSTest::setUp()
{
    _previousSeverity = CERR.maxSeverity();
    if (utest::DebugMode()) {
        CERR.setMaxSeverity(ts::Severity::Debug);
    }
}

// Test suite cleanup method.
void HLSTest::tearDown()
{
    CERR.setMaxSeverity(_previousSeverity);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void HLSTest::testMasterPlaylist()
{
    // Test file downloaded from TSDuck web site.
    // Copied from Apple test file at
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_ts/master.m3u8

    ts::hls::PlayList pl;
    CPPUNIT_ASSERT(pl.loadURL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", true));
    CPPUNIT_ASSERT(pl.isValid());
    CPPUNIT_ASSERT_EQUAL(ts::hls::MASTER_PLAYLIST, pl.type());
    CPPUNIT_ASSERT_EQUAL(6, pl.version());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/master.m3u8", pl.url());
    CPPUNIT_ASSERT_USTRINGS_EQUAL(u"https://tsduck.io/download/test/hls/img_bipbop_adv_example_ts/foo.bar", pl.buildURL(u"foo.bar"));
}
