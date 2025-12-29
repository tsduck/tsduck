//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(StackSize);
    TSUNIT_DECLARE_TEST(DeleteWhenTerminated);
    TSUNIT_DECLARE_TEST(Priority);
};

TSUNIT_REGISTER(ThreadAttributesTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(StackSize)
{
    ts::ThreadAttributes attr;
    TSUNIT_ASSERT(attr.getStackSize() == 0); // default value
    TSUNIT_ASSERT(attr.setStackSize(123456).getStackSize() == 123456);
}

TSUNIT_DEFINE_TEST(DeleteWhenTerminated)
{
    ts::ThreadAttributes attr;
    TSUNIT_ASSERT(attr.getDeleteWhenTerminated() == false); // default value
    TSUNIT_ASSERT(attr.setDeleteWhenTerminated(true).getDeleteWhenTerminated() == true);
    TSUNIT_ASSERT(attr.setDeleteWhenTerminated(false).getDeleteWhenTerminated() == false);
}

TSUNIT_DEFINE_TEST(Priority)
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
