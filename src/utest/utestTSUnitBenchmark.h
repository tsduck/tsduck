//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        bool             _started = false;
        cn::milliseconds _start {0};        // Process CPU time on start().
        cn::milliseconds _accumulated {0};  // Accumulated CPU times.
        size_t           _sequences = 0;    // Number of sequences

        static size_t GetIterations(const ts::UString& env_name);
    };
}
