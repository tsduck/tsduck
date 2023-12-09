//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "utestTSUnitThread.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

utest::TSUnitThread::TSUnitThread(const ts::ThreadAttributes& attributes) :
    ts::Thread(attributes)
{
}

utest::TSUnitThread::~TSUnitThread()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// TSUnit wrapper for thread main code.
//----------------------------------------------------------------------------

void utest::TSUnitThread::main()
{
    try {
        // Execute the real test.
        test();
    }
    catch (const std::exception& e) {
        std::cerr << std::endl
                  << "*** Terminating exception in a thread, aborting" << std::endl
                  << "*** " << e.what() << std::endl
                  << std::endl;
        // Exit application.
        std::exit(EXIT_FAILURE);
    }
    catch (...) {
        std::cerr << std::endl
                  << "*** Unknown kind of exception in a thread, aborting" << std::endl
                  << std::endl;
        // Exit application.
        std::exit(EXIT_FAILURE);
    }
}
