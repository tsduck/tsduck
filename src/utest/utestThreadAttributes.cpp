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
//  TSUnit test suite for class ts::ThreadAttributes
//
//  Note on Linux: The standard test suite will run with the default
//  scheduling policy for which there is only one possible priority.
//  To test operating system priority values in a wider range of priorities,
//  try the following command:
//
//  $ sudo chrt -f 20 utest -d
//  ....
//  ThreadAttributesTest: GetMinimumPriority() = 1
//  ThreadAttributesTest: GetNormalPriority()  = 50
//  ThreadAttributesTest: GetMaximumPriority() = 99
//  ....
//
//----------------------------------------------------------------------------

#include "tsThreadAttributes.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ThreadAttributesTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testStackSize();
    void testDeleteWhenTerminated();
    void testPriority();

    TSUNIT_TEST_BEGIN(ThreadAttributesTest);
    TSUNIT_TEST(testStackSize);
    TSUNIT_TEST(testDeleteWhenTerminated);
    TSUNIT_TEST(testPriority);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(ThreadAttributesTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void ThreadAttributesTest::beforeTest()
{
}

// Test suite cleanup method.
void ThreadAttributesTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

void ThreadAttributesTest::testStackSize()
{
    ts::ThreadAttributes attr;
    TSUNIT_ASSERT(attr.getStackSize() == 0); // default value
    TSUNIT_ASSERT(attr.setStackSize(123456).getStackSize() == 123456);
}

void ThreadAttributesTest::testDeleteWhenTerminated()
{
    ts::ThreadAttributes attr;
    TSUNIT_ASSERT(attr.getDeleteWhenTerminated() == false); // default value
    TSUNIT_ASSERT(attr.setDeleteWhenTerminated(true).getDeleteWhenTerminated() == true);
    TSUNIT_ASSERT(attr.setDeleteWhenTerminated(false).getDeleteWhenTerminated() == false);
}

void ThreadAttributesTest::testPriority()
{
    debug()
        << "ThreadAttributesTest: GetMinimumPriority() = " << ts::ThreadAttributes::GetMinimumPriority() << std::endl
        << "ThreadAttributesTest: GetNormalPriority()  = " << ts::ThreadAttributes::GetNormalPriority() << std::endl
        << "ThreadAttributesTest: GetMaximumPriority() = " << ts::ThreadAttributes::GetMaximumPriority() << std::endl;

    TSUNIT_ASSERT(ts::ThreadAttributes::GetMinimumPriority() <= ts::ThreadAttributes::GetNormalPriority());
    TSUNIT_ASSERT(ts::ThreadAttributes::GetNormalPriority() <= ts::ThreadAttributes::GetMaximumPriority());

    ts::ThreadAttributes attr;
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetNormalPriority()); // default value

    attr.setPriority (ts::ThreadAttributes::GetMinimumPriority());
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetMinimumPriority());

    attr.setPriority (ts::ThreadAttributes::GetMinimumPriority() - 1);
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetMinimumPriority());

    attr.setPriority (ts::ThreadAttributes::GetMaximumPriority());
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetMaximumPriority());

    attr.setPriority (ts::ThreadAttributes::GetMaximumPriority() + 1);
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetMaximumPriority());

    attr.setPriority (ts::ThreadAttributes::GetNormalPriority());
    TSUNIT_ASSERT(attr.getPriority() == ts::ThreadAttributes::GetNormalPriority());
}
