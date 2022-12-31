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
//  TSUnit test suite for class ts::ConfigFile
//
//----------------------------------------------------------------------------

#include "tsConfigFile.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ConfigTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testDefaultFile();
    void testFile();

    TSUNIT_TEST_BEGIN(ConfigTest);
    TSUNIT_TEST(testDefaultFile);
    TSUNIT_TEST(testFile);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ConfigTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ConfigTest::beforeTest()
{
}

// Test suite cleanup method.
void ConfigTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    void displayConfig(const ts::ConfigFile& config, const std::string& title)
    {
        tsunit::Test::debug() << "*** " << title << " ***" << std::endl
            << "Section count: " << config.sectionCount() << std::endl;

        ts::UStringVector secnames;
        config.getSectionNames(secnames);

        for (size_t sec = 0; sec < secnames.size(); ++sec) {

            const ts::ConfigSection& section(config.section(secnames[sec]));
            tsunit::Test::debug() << "   Section \"" << secnames[sec] << "\", entryCount() = " << section.entryCount() << std::endl;

            ts::UStringVector entnames;
            section.getEntryNames(entnames);

            for (size_t ent = 0; ent < entnames.size(); ++ent) {
                size_t val_count(section.valueCount(entnames[ent]));
                tsunit::Test::debug() << "      Entry \"" << entnames[ent] << "\", valueCount() = " << val_count;
                for (size_t val = 0; val < val_count; ++val) {
                    tsunit::Test::debug() << ", [" << val << "] = \"" << section.value(entnames[ent], val) << "\"";
                }
                tsunit::Test::debug() << std::endl;
            }
        }

        tsunit::Test::debug() << "*** " << title << " (Save) ***" << std::endl << config;
    }
}

void ConfigTest::testDefaultFile()
{
    debug() << "ConfigTest: DefaultFileName() = \"" << ts::ConfigFile::DefaultFileName() << "\"" << std::endl
            << "ConfigTest: DefaultFileName(UNIX_STYLE) = \"" << ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE) << "\"" << std::endl
            << "ConfigTest: DefaultFileName(WINDOWS_STYLE) = \"" << ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE) << "\"" << std::endl;

    TSUNIT_ASSERT(!ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE).empty());
    TSUNIT_ASSERT(!ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE).empty());
    TSUNIT_ASSERT(ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE) !=
                  ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE));
    TSUNIT_ASSERT(ts::ConfigFile::DefaultFileName() == ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE) ||
                  ts::ConfigFile::DefaultFileName() == ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE));
}

void ConfigTest::testFile()
{
    // Reference configuration content.
    const std::string referenceContent(
        "# Test configuration file\n"
        "\n"
        "azerty = qsdf \n"
        "foo = bar\n"
        "azerty = sdf \n"
        "azerty=23\n"
        "azerty =43  \n"
        "\n"
        "[SectionBoo]\n"
        "  bar = aze\n"
        "  foo =dfv\n"
        "  empty0 =\n"
        "  foo =  ff\n"
        "  empty1 = \n"
        "  empty2 =  \n"
        "\n"
        "[Section222]\n"
        "\n"
        "[Section333]\n"
        "      # comment\n"
        "        azerty          =  qwe\\\n"
        "rty  \n"
        "\n");

    std::istringstream input(referenceContent);
    ts::ConfigFile config(input);
    displayConfig(config, "Default config file content");

    TSUNIT_EQUAL(4, config.sectionCount());

    ts::UStringVector names1;
    config.getSectionNames(names1);
    std::sort(names1.begin(), names1.end());

    ts::UStringVector names2;
    ts::UString(u", Section222, Section333, SectionBoo").split(names2);
    TSUNIT_ASSERT(names1 == names2);

    TSUNIT_EQUAL(2, config[u""].entryCount());
    TSUNIT_EQUAL(0, config[u"Section222"].entryCount());
    TSUNIT_EQUAL(1, config[u"Section333"].entryCount());
    TSUNIT_EQUAL(0, config[u"Section444"].entryCount());
    TSUNIT_EQUAL(5, config[u"SectionBoo"].entryCount());

    TSUNIT_EQUAL(1, config[u""].valueCount(u"foo"));
    TSUNIT_EQUAL(4, config[u""].valueCount(u"azerty"));
    TSUNIT_EQUAL(0, config[u""].valueCount(u"nonexistent"));
    TSUNIT_EQUAL(2, config[u"SectionBoo"].valueCount(u"foo"));

    TSUNIT_EQUAL(u"aze", config[u"SectionBoo"].value(u"bar"));
    TSUNIT_EQUAL(u"dfv", config[u"SectionBoo"].value(u"foo"));
    TSUNIT_EQUAL(u"dfv", config[u"SectionBoo"].value(u"foo", 0));
    TSUNIT_EQUAL(u"ff",  config[u"SectionBoo"].value(u"foo", 1));
    TSUNIT_EQUAL(u"",    config[u"SectionBoo"].value(u"foo", 2));
    TSUNIT_EQUAL(u"def", config[u"SectionBoo"].value(u"foo", 2, u"def"));

    TSUNIT_EQUAL(43, config[u""].value<int>(u"azerty", 3));
    TSUNIT_EQUAL(23, config[u""].value<int>(u"azerty", 2));
    TSUNIT_EQUAL(0,  config[u""].value<int>(u"azerty", 1));

    config.reset();
    displayConfig(config, "Config after Reset()");

    TSUNIT_EQUAL(0, config.sectionCount());
    TSUNIT_EQUAL(0, config[u""].entryCount());
}
