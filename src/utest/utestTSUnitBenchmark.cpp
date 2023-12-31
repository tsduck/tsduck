//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "utestTSUnitBenchmark.h"
#include "tsunit.h"
#include "tsSysUtils.h"
#include "tsEnvironment.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

size_t utest::TSUnitBenchmark::GetIterations(const ts::UString& env_name)
{
    size_t value = 1;
    return ts::GetEnvironment(env_name).toInteger(value, u",") && value > 0 ? value : 1;
}

utest::TSUnitBenchmark::TSUnitBenchmark::TSUnitBenchmark(const ts::UString& env_name) :
    iterations(GetIterations(env_name))
{
}


//----------------------------------------------------------------------------
// Start accumulating CPU time.
//----------------------------------------------------------------------------

void utest::TSUnitBenchmark::start()
{
    if (!_started) {
        _start = ts::GetProcessCpuTime();
        _started = true;
    }
}

void utest::TSUnitBenchmark::stop()
{
    if (_started) {
        _accumulated += ts::GetProcessCpuTime() - _start;
        _sequences++;
        _started = false;
    }
}


//----------------------------------------------------------------------------
// Report acuumulated CPU time on utest debug output.
//----------------------------------------------------------------------------

void utest::TSUnitBenchmark::report(const ts::UString& test_name)
{
    if (_started) {
        // Restart to accumulate until now.
        stop();
        start();
    }
    tsunit::Test::debug() << ts::UString::Format(u"%s: %'d sequences of %'d iterations, %'d ms",
                                                 {test_name, _sequences, iterations, _accumulated})
                          << std::endl;
}
