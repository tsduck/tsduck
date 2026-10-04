//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::tr101290::Analyzer
//
//----------------------------------------------------------------------------

#include "tstr101290Analyzer.h"
#include "tsDuckContext.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TR101290Test: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(PCRDiff);
};

TSUNIT_REGISTER(TR101290Test);


//----------------------------------------------------------------------------
// Test case: non-regression for issue #1749
//----------------------------------------------------------------------------

// Code adapted from issue #1749.
// Feed 10 packets with PCR increments of 40 ms. Add 'step_ms' in PCR of 6th packet.
namespace {
    void StepPCR(cn::milliseconds step_ms, size_t pcr_error, size_t disc_error)
    {
        ts::DuckContext duck;
        ts::tr101290::Analyzer analyzer(duck);
        const ts::PCR increment = cn::duration_cast<ts::PCR>(cn::milliseconds(40));
        const ts::PCR step = cn::duration_cast<ts::PCR>(step_ms);
        ts::PCR clock = ts::PCR::zero();
        ts::PCR pcr_pid = ts::PCR(1'000'000);  // arbitrary value
        uint8_t cc = 0;

        ts::TSPacket pkt(ts::NullPacket);
        pkt.setPID(0x100);

        for (int i = 0; i < 10; ++i) {
            if (i == 5) {
                pcr_pid += step - increment;
            }
            pkt.setCC(cc++);
            pkt.setPCR(pcr_pid.count(), true);
            analyzer.feedPacket(clock, pkt);
            pcr_pid += increment;
            clock += increment;
        }

        ts::tr101290::Counters c;
        analyzer.getCounters(c);
        tsunit::Test::debug() << "TR101290Test, StepPCR: step: " << ts::UString::Chrono(step_ms)
                              << ", PCR error: " << c[ts::tr101290::PCR_error]
                              << ", disc error: " << c[ts::tr101290::PCR_discontinuity_indicator_error]
                              << std::endl;

        TSUNIT_EQUAL(pcr_error, c[ts::tr101290::PCR_error]);
        TSUNIT_EQUAL(disc_error, c[ts::tr101290::PCR_discontinuity_indicator_error]);
    }
}

TSUNIT_DEFINE_TEST(PCRDiff)
{
    // [[2.3.b]] PCR_discontinuity_indicator_error: The difference between two consecutive PCR values (PCRi+1 – PCRi) is outside the range of 0...100 ms without the discontinuity_indicator set.
    // List of PCR offsets in ms are listed in comment at end of line. Errors are flagged with "*".

    StepPCR(cn::milliseconds(+40),  0, 0);  // 0 40 80 120 160 200  240 280 320 360 (regular)
    StepPCR(cn::milliseconds(+99),  0, 0);  // 0 40 80 120 160 259  299 339 379 419
    StepPCR(cn::milliseconds(+1),   0, 0);  // 0 40 80 120 160 161  201 241 281 321
    StepPCR(cn::milliseconds(-1),   1, 1);  // 0 40 80 120 160 159* 199 239 279 319
    StepPCR(cn::milliseconds(-10),  1, 1);  // 0 40 80 120 160 150* 190 230 270 310
    StepPCR(cn::milliseconds(-99),  1, 1);  // 0 40 80 120 160  61* 101 141 181 221
    StepPCR(cn::milliseconds(-101), 1, 1);  // 0 40 80 120 160  59*  99 139 179 219
    StepPCR(cn::milliseconds(+200), 1, 1);  // 0 40 80 120 160 360* 400 440 480 520
    StepPCR(cn::milliseconds(+101), 1, 1);  // 0 40 80 120 160 261* 301 341 381 421
}
