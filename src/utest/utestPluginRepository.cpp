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
    const ts::UStringList inputs(ts::PluginRepository::Instance()->inputNames());
    const ts::UStringList outputs(ts::PluginRepository::Instance()->outputNames());
    const ts::UStringList procs(ts::PluginRepository::Instance()->processorNames());

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
    ts::PluginRepository* repo = ts::PluginRepository::Instance();

    // There are embedded plugins of all types in TSDuck shared library.
    TSUNIT_ASSERT(repo->inputCount() > 0);
    TSUNIT_ASSERT(repo->outputCount() > 0);
    TSUNIT_ASSERT(repo->processorCount() > 0);

    TSUNIT_ASSERT(repo->getInput(u"null", report) != nullptr);
    TSUNIT_ASSERT(repo->getOutput(u"null", report) == nullptr);
    TSUNIT_ASSERT(repo->getProcessor(u"null", report) == nullptr);

    TSUNIT_ASSERT(repo->getInput(u"drop", report) == nullptr);
    TSUNIT_ASSERT(repo->getOutput(u"drop", report) != nullptr);
    TSUNIT_ASSERT(repo->getProcessor(u"drop", report) == nullptr);

    TSUNIT_ASSERT(repo->getInput(u"file", report) != nullptr);
    TSUNIT_ASSERT(repo->getOutput(u"file", report) != nullptr);
    TSUNIT_ASSERT(repo->getProcessor(u"file", report) != nullptr);
}

void PluginRepositoryTest::testLoaded()
{
    ts::Report& report(debugMode() ? *static_cast<ts::Report*>(&CERR) : *static_cast<ts::Report*>(&NULLREP));
    ts::PluginRepository* repo = ts::PluginRepository::Instance();

    TSUNIT_ASSERT(repo->getInput(u"merge", report) == nullptr);
    TSUNIT_ASSERT(repo->getOutput(u"merge", report) == nullptr);
    TSUNIT_ASSERT(repo->getProcessor(u"merge", report) != nullptr);
}
