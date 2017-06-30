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
//  CppUnit test suite for class ts::ConfigFile
//
//----------------------------------------------------------------------------

#include "tsConfigFile.h"
#include "tsSysUtils.h"
#include "utestCppUnitTest.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ConfigTest: public CppUnit::TestFixture
{
public:
    void setUp();
    void tearDown();
    void testFile();

    CPPUNIT_TEST_SUITE(ConfigTest);
    CPPUNIT_TEST(testFile);
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ConfigTest::setUp()
{
}

// Test suite cleanup method.
void ConfigTest::tearDown()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    void displayConfig(const ts::ConfigFile& config, const std::string& title)
    {
        utest::Out() << "*** " << title << " ***" << std::endl
                     << "Section count: " << config.sectionCount() << std::endl;

        ts::StringVector secnames;
        config.getSectionNames(secnames);

        for (size_t sec = 0; sec < secnames.size(); ++sec) {

            const ts::ConfigSection& section(config.section(secnames[sec]));
            utest::Out() << "   Section \"" << secnames[sec] << "\", entryCount() = " << section.entryCount() << std::endl;

            ts::StringVector entnames;
            section.getEntryNames(entnames);

            for (size_t ent = 0; ent < entnames.size(); ++ent) {
                size_t val_count(section.valueCount(entnames[ent]));
                utest::Out() << "      Entry \"" << entnames[ent] << "\", valueCount() = " << val_count;
                for (size_t val = 0; val < val_count; ++val) {
                    utest::Out() << ", [" << val << "] = \"" << section.value(entnames[ent], val) << "\"";
                }
                utest::Out() << std::endl;
            }
        }

        utest::Out() << "*** " << title << " (Save) ***" << std::endl << config;
    }
}

void ConfigTest::testFile()
{
    utest::Out() << "ConfigTest: DefaultFileName() = \"" << ts::ConfigFile::DefaultFileName() << "\"" << std::endl;

    // Reference configuration content.
    const std::string referenceContent(
        "# Test configuration file\n"
        "\n"
        "foo = bar\n"
        "azerty = qsdf , sdf , 23, 43\n"
        "\n"
        "[SectionBoo]\n"
        "  foo = boo\n"
        "  bar = aze\n"
        "  foo = dfv, ff\n"
        "  empty0 =\n"
        "  empty1 = \n"
        "  empty2 =  \n"
        "\n"
        "[Section222]\n"
        "\n"
        "[Section333]\n"
        "        azerty          =  qwerty       # comment\n"
        "\n");

    std::istringstream input(referenceContent);
    ts::ConfigFile config(input);
    displayConfig(config, "Default config file content");

    CPPUNIT_ASSERT_EQUAL(size_t(4), config.sectionCount());

    ts::StringVector names1;
    config.getSectionNames(names1);
    std::sort(names1.begin(), names1.end());

    ts::StringVector names2;
    ts::SplitString(names2, ", Section222, Section333, SectionBoo");
    CPPUNIT_ASSERT(names1 == names2);

    CPPUNIT_ASSERT_EQUAL(size_t(2), config[""].entryCount());
    CPPUNIT_ASSERT_EQUAL(size_t(0), config["Section222"].entryCount());
    CPPUNIT_ASSERT_EQUAL(size_t(1), config["Section333"].entryCount());
    CPPUNIT_ASSERT_EQUAL(size_t(0), config["Section444"].entryCount());
    CPPUNIT_ASSERT_EQUAL(size_t(5), config["SectionBoo"].entryCount());

    CPPUNIT_ASSERT_EQUAL(size_t(1), config[""].valueCount("foo"));
    CPPUNIT_ASSERT_EQUAL(size_t(4), config[""].valueCount("azerty"));
    CPPUNIT_ASSERT_EQUAL(size_t(0), config[""].valueCount("nonexistent"));
    CPPUNIT_ASSERT_EQUAL(size_t(2), config["SectionBoo"].valueCount("foo"));

    CPPUNIT_ASSERT_EQUAL(std::string("aze"), config["SectionBoo"].value("bar"));
    CPPUNIT_ASSERT_EQUAL(std::string("dfv"), config["SectionBoo"].value("foo"));
    CPPUNIT_ASSERT_EQUAL(std::string("dfv"), config["SectionBoo"].value("foo", 0));
    CPPUNIT_ASSERT_EQUAL(std::string("ff"),  config["SectionBoo"].value("foo", 1));
    CPPUNIT_ASSERT_EQUAL(std::string(""),    config["SectionBoo"].value("foo", 2));
    CPPUNIT_ASSERT_EQUAL(std::string("def"), config["SectionBoo"].value("foo", 2, "def"));

    CPPUNIT_ASSERT_EQUAL(43, config[""].value<int>("azerty", 3));
    CPPUNIT_ASSERT_EQUAL(23, config[""].value<int>("azerty", 2));
    CPPUNIT_ASSERT_EQUAL(0,  config[""].value<int>("azerty", 1));

    config.reset();
    displayConfig(config, "Config after Reset()");

    CPPUNIT_ASSERT_EQUAL(size_t(0), config.sectionCount());
    CPPUNIT_ASSERT_EQUAL(size_t(0), config[""].entryCount());
}
