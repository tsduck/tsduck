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
//!
//!  @file
//!  Thread wrapper for TSUnit.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"

namespace utest {
    //!
    //! TSUnit wrapper for thread main code.
    //!
    //! TSUnit is not designed for multi-threading. Any assertion failure in a thread
    //! produces unspecified results, typically a crash of the application, and there
    //! is no error message about the failing display. This class is a wrapper
    //! around the main code of a thread. In case of assertion failure, a TSUnit
    //! error is displayed and the application properly exits.
    //!
    class TSUnitThread : public ts::Thread
    {
        TS_NOCOPY(TSUnitThread);
    public:
        //!
        //! Default constructor.
        //!
        TSUnitThread();

        //!
        //! Destructor.
        //!
        virtual ~TSUnitThread() override;

        //!
        //! Constructor from specified attributes.
        //! @param [in] attributes The set of attributes.
        //!
        TSUnitThread(const ts::ThreadAttributes& attributes);

        //!
        //! Actual test code (thread main code).
        //!
        virtual void test() = 0;

        // Implementation of thread interface.
        virtual void main() override;
    };
}
