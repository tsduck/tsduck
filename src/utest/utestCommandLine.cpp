//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//  TSUnit test suite for CommandLine class.
//
//----------------------------------------------------------------------------

#include "tsCommandLine.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CommandLineTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testCommand();

    TSUNIT_TEST_BEGIN(CommandLineTest);
    TSUNIT_TEST(testCommand);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(CommandLineTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CommandLineTest::beforeTest()
{
}

// Test suite cleanup method.
void CommandLineTest::afterTest()
{
}


//----------------------------------------------------------------------------
// A command handler which logs its output in a string
//----------------------------------------------------------------------------

namespace {
    class TestCommand : public ts::CommandLineHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TestCommand);
    public:
        // Everything is logged here.
        ts::UString output;

        // Constructor.
        TestCommand(ts::CommandLine& cmdline) : output()
        {
            ts::Args* args = cmdline.command(this, u"cmd1");
            args->option(u"foo");

            args = cmdline.command(this, u"cmd2");
            args->option(u"bar");
        }

        // Command handler.
        virtual bool handleCommandLine(const ts::UString& command, ts::Args& args) override
        {
            output.format(u"[command:%s]", {command});
            if (command == u"cmd1") {
                output.format(u"[--foo:%s]", {args.present(u"foo")});
            }
            else if (command == u"cmd2") {
                output.format(u"[--bar:%s]", {args.present(u"bar")});
            }
            return true;
        }
    };
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void CommandLineTest::testCommand()
{
    ts::CommandLine cmdline;
    TestCommand test(cmdline);

    test.output.clear();
    TSUNIT_ASSERT(cmdline.processCommand(u"cmd1"));
    TSUNIT_EQUAL(test.output, u"[command:cmd1][--foo:false]");

    test.output.clear();
    TSUNIT_ASSERT(cmdline.processCommand(u"cmd1 --foo"));
    TSUNIT_EQUAL(test.output, u"[command:cmd1][--foo:true]");

    test.output.clear();
    TSUNIT_ASSERT(cmdline.processCommand(u"cmd2"));
    TSUNIT_EQUAL(test.output, u"[command:cmd2][--bar:false]");

    test.output.clear();
    TSUNIT_ASSERT(cmdline.processCommand(u"cmd2 --bar"));
    TSUNIT_EQUAL(test.output, u"[command:cmd2][--bar:true]");
}
