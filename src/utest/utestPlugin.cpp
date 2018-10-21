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
//  CppUnit test suite for class ts::PluginSharedLibrary
//
//----------------------------------------------------------------------------

#include "tsPluginSharedLibrary.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PluginTest: public CppUnit::TestFixture
{
public:
    virtual void setUp() override;
    virtual void tearDown() override;

    void testInput();
    void testOutput();
    void testProcessor();

    CPPUNIT_TEST_SUITE(PluginTest);
    CPPUNIT_TEST(testInput);
    CPPUNIT_TEST(testOutput);
    CPPUNIT_TEST(testProcessor);
    CPPUNIT_TEST_SUITE_END();

private:
    static void display(const ts::PluginSharedLibrary& lib);
};

CPPUNIT_TEST_SUITE_REGISTRATION(PluginTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void PluginTest::setUp()
{
}

// Test suite cleanup method.
void PluginTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void PluginTest::display(const ts::PluginSharedLibrary& lib)
{
    utest::Out() << "* File: " << lib.fileName() << std::endl
                 << "  isLoaded: " << lib.isLoaded() << std::endl
                 << "  input: " << ts::UString::YesNo(lib.new_input != nullptr) << std::endl
                 << "  output: " << ts::UString::YesNo(lib.new_output != nullptr) << std::endl
                 << "  processor: " << ts::UString::YesNo(lib.new_processor != nullptr) << std::endl;
}

void PluginTest::testInput()
{
    ts::PluginSharedLibrary plugin(u"null");
    display(plugin);

    CPPUNIT_ASSERT(plugin.isLoaded());
    CPPUNIT_ASSERT(plugin.new_input != nullptr);
    CPPUNIT_ASSERT(plugin.new_output == nullptr);
    CPPUNIT_ASSERT(plugin.new_processor == nullptr);
}

void PluginTest::testOutput()
{
    ts::PluginSharedLibrary plugin(u"drop");
    display(plugin);

    CPPUNIT_ASSERT(plugin.isLoaded());
    CPPUNIT_ASSERT(plugin.new_input == nullptr);
    CPPUNIT_ASSERT(plugin.new_output != nullptr);
    CPPUNIT_ASSERT(plugin.new_processor == nullptr);
}

void PluginTest::testProcessor()
{
    ts::PluginSharedLibrary plugin(u"skip");
    display(plugin);

    CPPUNIT_ASSERT(plugin.isLoaded());
    CPPUNIT_ASSERT(plugin.new_input == nullptr);
    CPPUNIT_ASSERT(plugin.new_output == nullptr);
    CPPUNIT_ASSERT(plugin.new_processor != nullptr);
}
