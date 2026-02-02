//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::SystemRandomGenerator
//
//----------------------------------------------------------------------------

#include "tsSystemRandomGenerator.h"
#include "tsBetterSystemRandomGenerator.h"
#include "tsByteBlock.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class SystemRandomGeneratorTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(SystemRandomGenerator);
#if !defined(TS_NO_EXTERNAL_TESTS)
    TSUNIT_DECLARE_TEST(BetterSystemRandomGenerator);
#endif
    TSUNIT_DECLARE_TEST(Range);

private:
    void testRandom(ts::RandomGenerator& prng);
    void testRandomRange(ts::RandomGenerator& prng, int64_t min, int64_t max);
};

TSUNIT_REGISTER(SystemRandomGeneratorTest);


//----------------------------------------------------------------------------
// Test a PRNG.
//----------------------------------------------------------------------------

void SystemRandomGeneratorTest::testRandom(ts::RandomGenerator& prng)
{
    // System PRNG are supposed to be immediately ready
    TSUNIT_ASSERT(prng.ready());

    // But make sure they accept to be seeded anyway
    ts::ByteBlock seed(256);
    TSUNIT_ASSERT(prng.seed(&seed[0], seed.size()));

    // Now, it is difficult to "test" random generator.
    // We use the following scenario:
    // - Pre-fill two 1000-byte vectors with zeroes.
    // - Generate random data over them.
    // - Check that they are not identical.
    // - Check that no more than 10% of the bytes are zero
    // The later condition is a bit arbitrary. Statistically, the proportion
    // of zeroes is 3.9% (1/256) but this is only statistics.

    ts::ByteBlock data1(1000, 0);
    ts::ByteBlock data2(1000, 0);

    TSUNIT_ASSERT(data1 == data2);
    TSUNIT_EQUAL(data1.size(), size_t(std::count(data1.begin(), data1.end(), 0)));
    TSUNIT_EQUAL(data2.size(), size_t(std::count(data2.begin(), data2.end(), 0)));

    TSUNIT_ASSERT(prng.read(&data1[0], data1.size()));
    TSUNIT_ASSERT(prng.read(&data2[0], data2.size()));

    const size_t zero1 = std::count(data1.begin(), data1.end(), 0);
    const size_t zero2 = std::count(data2.begin(), data2.end(), 0);

    debug() << prng.name() << ": zeroes over " << data1.size() << " bytes: " << zero1 << ", " << zero2 << std::endl;

    TSUNIT_ASSERT(zero1 < data1.size() / 10);
    TSUNIT_ASSERT(zero2 < data2.size() / 10);
    TSUNIT_ASSERT(data1 != data2);
}


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(SystemRandomGenerator)
{
    ts::SystemRandomGenerator gen;
    testRandom(gen);
}

#if !defined(TS_NO_EXTERNAL_TESTS)
TSUNIT_DEFINE_TEST(BetterSystemRandomGenerator)
{
    testRandom(ts::BetterSystemRandomGenerator::Instance());
}
#endif // TS_NO_EXTERNAL_TESTS

void SystemRandomGeneratorTest::testRandomRange(ts::RandomGenerator& prng, int64_t min, int64_t max)
{
    TSUNIT_ASSERT(prng.ready());
    int64_t val = 0;

    for (int i = 0; i < 100; i++) {
        TSUNIT_ASSERT(prng.readInt(val, min, max));
        if (val < min || val > max) {
            debug() << "SystemRandomGeneratorTest: min: " << min << ", max: " << max << ", value: " << val << std::endl;
        }
        TSUNIT_ASSERT(val >= min);
        TSUNIT_ASSERT(val <= max);
    }
}

TSUNIT_DEFINE_TEST(Range)
{
    ts::SystemRandomGenerator gen;
    testRandomRange(gen, 1000, 1200);
    testRandomRange(gen, -1100, -1000);
    testRandomRange(gen, -0x7FFFFFFFFFFFFF00, 0x7FFFFFFFFFFFFF00);
    testRandomRange(gen, 747, 747);
    testRandomRange(gen, -380, -380);

    int val = 0;
    TSUNIT_ASSERT(!gen.readInt(val, 5, 1));
}
