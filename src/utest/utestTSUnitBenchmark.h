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
//!  Benchmark support for TSUnit.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace utest {
    //!
    //! TSUnit support for benchmarking individual tests.
    //!
    class TSUnitBenchmark
    {
        TS_NOCOPY(TSUnitBenchmark);
    public:
        //!
        //! Constructor.
        //! @param [in] env_name Environment name containing the number of iterations.
        //!
        TSUnitBenchmark(const ts::UString& env_name = ts::UString());

        //!
        //! Number of iterations.
        //! Default is 1 if environment name not specified or not defined.
        //!
        const size_t iterations;

        //!
        //! Start accumulating CPU time.
        //!
        void start();

        //!
        //! Stop accumulating CPU time.
        //!
        void stop();

        //!
        //! Report acuumulated CPU time on utest debug output.
        //! @param [in] test_name Test name.
        //!
        void report(const ts::UString& test_name);

    private:
        bool            _started;
        ts::MilliSecond _start;        // Process CPU time on start().
        ts::MilliSecond _accumulated;  // Accumulated CPU times.
        size_t          _sequences;    // Number of sequences

        static size_t GetIterations(const ts::UString& env_name);
    };
}
