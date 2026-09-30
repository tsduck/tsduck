//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class SRTArgs
//
//----------------------------------------------------------------------------

#include "tsSRTArgs.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SRTTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(URL);
};

TSUNIT_REGISTER(SRTTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(URL)
{
    ts::SRTArgs args;


}
