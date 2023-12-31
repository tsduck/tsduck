//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for ts::PluginRepository.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsNullReport.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PluginRepositoryTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testRegistrations();
    void testEmbedded();
    void testLoaded();

    TSUNIT_TEST_BEGIN(PluginRepositoryTest);
    TSUNIT_TEST(testRegistrations);
    TSUNIT_TEST(testEmbedded);
    TSUNIT_TEST(testLoaded);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(PluginRepositoryTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void PluginRepositoryTest::beforeTest()
{
}

// Test suite cleanup method.
void PluginRepositoryTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void PluginRepositoryTest::testRegistrations()
{
    const ts::UStringList inputs(ts::PluginRepository::Instance().inputNames());
    const ts::UStringList outputs(ts::PluginRepository::Instance().outputNames());
    const ts::UStringList procs(ts::PluginRepository::Instance().processorNames());

    debug() << "PluginRepositoryTest::testRegistrations: input names: " << ts::UString::Join(inputs) << std::endl
            << "PluginRepositoryTest::testRegistrations: output names: " << ts::UString::Join(outputs) << std::endl
            << "PluginRepositoryTest::testRegistrations: processor names: " << ts::UString::Join(procs) << std::endl;

    TSUNIT_ASSERT(!inputs.empty());
    TSUNIT_ASSERT(!outputs.empty());
    TSUNIT_ASSERT(!procs.empty());
    TSUNIT_ASSERT(ts::UString(u"null").isContainedSimilarIn(inputs));
    TSUNIT_ASSERT(ts::UString(u"file").isContainedSimilarIn(inputs));
    TSUNIT_ASSERT(ts::UString(u"file").isContainedSimilarIn(outputs));
    TSUNIT_ASSERT(ts::UString(u"file").isContainedSimilarIn(procs));
    TSUNIT_ASSERT(ts::UString(u"drop").isContainedSimilarIn(outputs));
}

void PluginRepositoryTest::testEmbedded()
{
    ts::Report& report(debugMode() ? *static_cast<ts::Report*>(&CERR) : *static_cast<ts::Report*>(&NULLREP));
    ts::PluginRepository& repo(ts::PluginRepository::Instance());

    // There are embedded plugins of all types in TSDuck shared library.
    TSUNIT_ASSERT(repo.inputCount() > 0);
    TSUNIT_ASSERT(repo.outputCount() > 0);
    TSUNIT_ASSERT(repo.processorCount() > 0);

    TSUNIT_ASSERT(repo.getInput(u"null", report) != nullptr);
    TSUNIT_ASSERT(repo.getOutput(u"null", report) == nullptr);
    TSUNIT_ASSERT(repo.getProcessor(u"null", report) == nullptr);

    TSUNIT_ASSERT(repo.getInput(u"drop", report) == nullptr);
    TSUNIT_ASSERT(repo.getOutput(u"drop", report) != nullptr);
    TSUNIT_ASSERT(repo.getProcessor(u"drop", report) == nullptr);

    TSUNIT_ASSERT(repo.getInput(u"file", report) != nullptr);
    TSUNIT_ASSERT(repo.getOutput(u"file", report) != nullptr);
    TSUNIT_ASSERT(repo.getProcessor(u"file", report) != nullptr);
}

void PluginRepositoryTest::testLoaded()
{
    ts::Report& report(debugMode() ? *static_cast<ts::Report*>(&CERR) : *static_cast<ts::Report*>(&NULLREP));
    ts::PluginRepository& repo(ts::PluginRepository::Instance());

    TSUNIT_ASSERT(repo.getInput(u"merge", report) == nullptr);
    TSUNIT_ASSERT(repo.getOutput(u"merge", report) == nullptr);
    TSUNIT_ASSERT(repo.getProcessor(u"merge", report) != nullptr);
}
