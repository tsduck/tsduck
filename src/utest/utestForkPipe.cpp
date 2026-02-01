//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::ForkPipe.
//
//----------------------------------------------------------------------------

#include "tsForkPipe.h"
#include "tsCerrReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class ForkPipeTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ListFiles);
};

TSUNIT_REGISTER(ForkPipeTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ListFiles)
{
#if defined(TS_WINDOWS)
    static const ts::UString ls(u"dir");
#else
    static const ts::UString ls(u"ls -l");
#endif

    ts::UString output;

    TSUNIT_ASSERT(ts::ForkPipe::GetOutput(output, ls, CERR, true));
    TSUNIT_ASSERT(!output.empty());

    debug() << "ForkPipeTest: output of ls: \"" << output << "\"" << std::endl;
}
