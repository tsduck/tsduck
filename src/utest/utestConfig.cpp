//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(DefaultFile);
    TSUNIT_DECLARE_TEST(File);
};

TSUNIT_REGISTER(ConfigTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    void displayConfig(const ts::ConfigFile& config, const std::string& title)
    {
        tsunit::Test::debug() << "*** " << title << " ***" << std::endl
            << "Section count: " << config.size() << std::endl;

        ts::UStringVector secnames;
        config.getSectionNames(secnames);

        for (size_t sec = 0; sec < secnames.size(); ++sec) {

            const ts::ConfigSection& section(config.section(secnames[sec]));
            tsunit::Test::debug() << "   Section \"" << secnames[sec] << "\", entryCount() = " << section.size() << std::endl;

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

TSUNIT_DEFINE_TEST(DefaultFile)
{
    debug() << "ConfigTest: DefaultFileName() = \"" << ts::ConfigFile::DefaultFileName().string() << "\"" << std::endl
            << "ConfigTest: DefaultFileName(UNIX_STYLE) = \"" << ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE).string() << "\"" << std::endl
            << "ConfigTest: DefaultFileName(WINDOWS_STYLE) = \"" << ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE).string() << "\"" << std::endl;

    TSUNIT_ASSERT(!ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE).empty());
    TSUNIT_ASSERT(!ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE).empty());
    TSUNIT_ASSERT(ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE) !=
                  ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE));
    TSUNIT_ASSERT(ts::ConfigFile::DefaultFileName() == ts::ConfigFile::DefaultFileName(ts::ConfigFile::UNIX_STYLE) ||
                  ts::ConfigFile::DefaultFileName() == ts::ConfigFile::DefaultFileName(ts::ConfigFile::WINDOWS_STYLE));
}

TSUNIT_DEFINE_TEST(File)
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

    TSUNIT_EQUAL(4, config.size());

    ts::UStringVector names1;
    config.getSectionNames(names1);
    std::sort(names1.begin(), names1.end());

    ts::UStringVector names2;
    ts::UString(u", Section222, Section333, SectionBoo").split(names2);
    TSUNIT_ASSERT(names1 == names2);

    TSUNIT_EQUAL(2, config[u""].size());
    TSUNIT_EQUAL(0, config[u"Section222"].size());
    TSUNIT_EQUAL(1, config[u"Section333"].size());
    TSUNIT_EQUAL(0, config[u"Section444"].size());
    TSUNIT_EQUAL(5, config[u"SectionBoo"].size());

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

    config.clear();
    displayConfig(config, "Config after Reset()");

    TSUNIT_EQUAL(0, config.size());
    TSUNIT_EQUAL(0, config[u""].size());
}
