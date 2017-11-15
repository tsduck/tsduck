//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#include "utestCppUnitThread.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

utest::CppUnitThread::CppUnitThread() :
    ts::Thread()
{
}

utest::CppUnitThread::CppUnitThread(const ts::ThreadAttributes& attributes) :
    ts::Thread(attributes)
{
}


//----------------------------------------------------------------------------
// CppUnit wrapper for thread main code.
//----------------------------------------------------------------------------

void utest::CppUnitThread::main()
{
    try {
        // Execute the real test.
        test();
    }
    catch (CPPUNIT_NS::Exception& e) {
        const CPPUNIT_NS::SourceLine src(e.sourceLine());
        const CPPUNIT_NS::Message msg(e.message());
        std::cerr << std::endl
                  << "*** CPPUNIT assertion failure in a thread, aborting" << std::endl;
        if (src.isValid()) {
            std::cerr << "line " << src.lineNumber() << ", file " << src.fileName() << std::endl;
        }
        std::cerr << msg.shortDescription() << std::endl
                  << msg.details() << std::endl << std::flush;
        // Exit application.
        ::exit(EXIT_FAILURE);
    }
}
