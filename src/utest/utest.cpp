//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Unitary tests driver program.
//
//  Description:
//    The main program exits with a success status if all tests passed and
//    a failure status otherwise.
//
//  Maintenance note:
//    There is no need to modify this code when a new test suite is added
//    (a new source file in the same directory). Each test suite is
//    automatically registered using the macro TSUNIT_REGISTER (see files).
//
//----------------------------------------------------------------------------

#include "tsunit.h"

int main(int argc, char* argv[])
{
    tsunit::Main test(argc, argv);
    return test.run();
}
