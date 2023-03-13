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

#include "utestTSUnitBenchmark.h"
#include "tsunit.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

size_t utest::TSUnitBenchmark::GetIterations(const ts::UString& env_name)
{
    size_t value = 1;
    return ts::GetEnvironment(env_name).toInteger(value, u",") && value > 0 ? value : 1;
}

utest::TSUnitBenchmark::TSUnitBenchmark::TSUnitBenchmark(const ts::UString& env_name) :
    iterations(GetIterations(env_name)),
    _started(false),
    _start(0),
    _accumulated(0),
    _sequences(0)
{
}


//----------------------------------------------------------------------------
// Start accumulating CPU time.
//----------------------------------------------------------------------------

void utest::TSUnitBenchmark::start()
{
    if (!_started) {
        ts::ProcessMetrics pm;
        ts::GetProcessMetrics(pm);
        _start = pm.cpu_time;
        _started = true;
    }
}

void utest::TSUnitBenchmark::stop()
{
    if (_started) {
        ts::ProcessMetrics pm;
        ts::GetProcessMetrics(pm);
        _accumulated += pm.cpu_time - _start;
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
