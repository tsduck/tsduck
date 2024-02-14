//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for tsPlatform.h
//
//----------------------------------------------------------------------------

#include "tsPlatform.h"
#include "tsVersion.h"
#include "tsVersionInfo.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PlatformTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testEndianness();
    void testIntegerTypes();
    void test64bitLiterals();
    void testStringify();
    void testVersion();
    void testChrono();

    TSUNIT_TEST_BEGIN(PlatformTest);
    TSUNIT_TEST(testEndianness);
    TSUNIT_TEST(testIntegerTypes);
    TSUNIT_TEST(test64bitLiterals);
    TSUNIT_TEST(testStringify);
    TSUNIT_TEST(testVersion);
    TSUNIT_TEST(testChrono);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(PlatformTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void PlatformTest::beforeTest()
{
}

// Test suite cleanup method.
void PlatformTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: verify endianness.
void PlatformTest::testEndianness()
{
    uint32_t i = 0x01020304;
    char* c = reinterpret_cast<char*>(&i);

#if defined(TS_LITTLE_ENDIAN)
    TSUNIT_ASSERT(TS_LITTLE_ENDIAN_BOOL);
    TSUNIT_ASSERT(!TS_BIG_ENDIAN_BOOL);
    TSUNIT_EQUAL(4, c[0]);
    TSUNIT_EQUAL(3, c[1]);
    TSUNIT_EQUAL(2, c[2]);
    TSUNIT_EQUAL(1, c[3]);
#endif

#if defined(TS_BIG_ENDIAN)
    TSUNIT_ASSERT(!TS_LITTLE_ENDIAN_BOOL);
    TSUNIT_ASSERT(TS_BIG_ENDIAN_BOOL);
    TSUNIT_EQUAL(1, c[0]);
    TSUNIT_EQUAL(2, c[1]);
    TSUNIT_EQUAL(3, c[2]);
    TSUNIT_EQUAL(4, c[3]);
#endif
}

// Test case: predefined integer types.
void PlatformTest::testIntegerTypes()
{
    debug()
        << "PlatformTest: TS_ADDRESS_BITS = " << TS_ADDRESS_BITS << std::endl
        << "PlatformTest: sizeof(int) = " << sizeof(int)
        << ", sizeof(long) = " << sizeof(long)
        << ", sizeof(long long) = " << sizeof(long long)
        << ", sizeof(void*) = " << sizeof(void*) << std::endl
        << "PlatformTest: sizeof(char) = " << sizeof(char)
        << ", sizeof(unsigned char) = " << sizeof(unsigned char)
        << ", is_signed(char) = " << std::is_signed<char>::value << std::endl
        << "PlatformTest: sizeof(wchar_t) = " << sizeof(wchar_t) << std::endl
        << "PlatformTest: sizeof(fs::path::value_type) = " << sizeof(fs::path::value_type) << std::endl
        << "std::numeric_limits<int64_t>::max() = " << std::numeric_limits<int64_t>::max() << std::endl
        << "std::numeric_limits<int64_t>::digits10 = " << std::numeric_limits<int64_t>::digits10 << std::endl
        << "std::numeric_limits<double>::max() = " << std::numeric_limits<double>::max() << std::endl
        << "std::numeric_limits<double>::min() = " << std::numeric_limits<double>::min() << std::endl
        << "std::numeric_limits<double>::lowest() = " << std::numeric_limits<double>::lowest() << std::endl
        << "std::numeric_limits<double>::digits10 = " << std::numeric_limits<double>::digits10 << std::endl;

    TSUNIT_EQUAL(1, sizeof(int8_t));
    TSUNIT_EQUAL(2, sizeof(int16_t));
    TSUNIT_EQUAL(4, sizeof(int32_t));
    TSUNIT_EQUAL(8, sizeof(int64_t));

    TSUNIT_EQUAL(1, sizeof(uint8_t));
    TSUNIT_EQUAL(2, sizeof(uint16_t));
    TSUNIT_EQUAL(4, sizeof(uint32_t));
    TSUNIT_EQUAL(8, sizeof(uint64_t));

    TSUNIT_EQUAL(0, TS_ADDRESS_BITS % 8);
    TSUNIT_EQUAL(TS_ADDRESS_BITS / 8, sizeof(void*));
    TSUNIT_EQUAL(TS_ADDRESS_BITS / 8, sizeof(size_t));
    TSUNIT_EQUAL(TS_ADDRESS_BITS / 8, sizeof(std::size_t));
    TSUNIT_EQUAL(TS_ADDRESS_BITS / 8, sizeof(std::ptrdiff_t));

    TSUNIT_ASSERT(std::numeric_limits<int8_t>::is_signed);
    TSUNIT_ASSERT(std::numeric_limits<int16_t>::is_signed);
    TSUNIT_ASSERT(std::numeric_limits<int32_t>::is_signed);
    TSUNIT_ASSERT(std::numeric_limits<int64_t>::is_signed);

    TSUNIT_ASSERT(!std::numeric_limits<uint8_t>::is_signed);
    TSUNIT_ASSERT(!std::numeric_limits<uint16_t>::is_signed);
    TSUNIT_ASSERT(!std::numeric_limits<uint32_t>::is_signed);
    TSUNIT_ASSERT(!std::numeric_limits<uint64_t>::is_signed);

#if defined(TS_WINDOWS)
    TSUNIT_EQUAL(2, sizeof(fs::path::value_type));
#else
    TSUNIT_EQUAL(1, sizeof(fs::path::value_type));
#endif

    int8_t  i8  = -1;
    int16_t i16 = -1;
    int32_t i32 = -1;
    int64_t i64 = -1;

    uint8_t  ui8  = 1;
    uint16_t ui16 = 1;
    uint32_t ui32 = 1;
    uint64_t ui64 = 1;

    int8_t  ai8[10];
    int16_t ai16[10];
    int32_t ai32[10];
    int64_t ai64[10];

    TSUNIT_EQUAL(1, sizeof(i8));
    TSUNIT_EQUAL(2, sizeof(i16));
    TSUNIT_EQUAL(4, sizeof(i32));
    TSUNIT_EQUAL(8, sizeof(i64));

    TSUNIT_EQUAL(1, sizeof(ui8));
    TSUNIT_EQUAL(2, sizeof(ui16));
    TSUNIT_EQUAL(4, sizeof(ui32));
    TSUNIT_EQUAL(8, sizeof(ui64));

    TSUNIT_ASSERT(i8  < 0);
    TSUNIT_ASSERT(i16 < 0);
    TSUNIT_ASSERT(i32 < 0);
    TSUNIT_ASSERT(i64 < 0);

    TSUNIT_ASSERT(ui8  > 0);
    TSUNIT_ASSERT(ui16 > 0);
    TSUNIT_ASSERT(ui32 > 0);
    TSUNIT_ASSERT(ui64 > 0);

    TSUNIT_ASSERT(sizeof(std::intmax_t) >= sizeof(uint64_t));
    TSUNIT_ASSERT(std::is_signed<std::intmax_t>::value);

    uint8_t  aui8[10];
    uint16_t aui16[10];
    uint32_t aui32[10];
    uint64_t aui64[10];

    TSUNIT_EQUAL(1, ts::char_ptr(&ai8[1])  - ts::char_ptr(&ai8[0]));
    TSUNIT_EQUAL(2, ts::char_ptr(&ai16[1]) - ts::char_ptr(&ai16[0]));
    TSUNIT_EQUAL(4, ts::char_ptr(&ai32[1]) - ts::char_ptr(&ai32[0]));
    TSUNIT_EQUAL(8, ts::char_ptr(&ai64[1]) - ts::char_ptr(&ai64[0]));

    TSUNIT_EQUAL(1, ts::char_ptr(&aui8[1])  - ts::char_ptr(&aui8[0]));
    TSUNIT_EQUAL(2, ts::char_ptr(&aui16[1]) - ts::char_ptr(&aui16[0]));
    TSUNIT_EQUAL(4, ts::char_ptr(&aui32[1]) - ts::char_ptr(&aui32[0]));
    TSUNIT_EQUAL(8, ts::char_ptr(&aui64[1]) - ts::char_ptr(&aui64[0]));
}

// Test case: 64-bit literals
void PlatformTest::test64bitLiterals()
{
    uint64_t ui64 = 0xFEDCBA9876543210;
    int64_t i64 = 0xFEDCBA9876543210;

    TSUNIT_EQUAL(0x0FEDCBA9, uint32_t(ui64 >> 36));
    TSUNIT_EQUAL(0xFFEDCBA9, int32_t(i64 >> 36));
}

// Test case: stringification macro
void PlatformTest::testStringify()
{
#define TEST_X 1
#define TEST_P1(x) ("P1[" #x "]")
#define TEST_P2(x) ("P2[" TS_STRINGIFY(x) "]")

    TSUNIT_EQUAL(std::string("P1[TEST_X]"), std::string(TEST_P1(TEST_X)));
    TSUNIT_EQUAL(std::string("P2[1]"),      std::string(TEST_P2(TEST_X)));

#undef TEST_X
#undef TEST_P1
#undef TEST_P2
}

// Test case: version string
void PlatformTest::testVersion()
{
    debug() << "PlatformTest: GetVersion(VERSION_SHORT) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_LONG) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::LONG) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_DATE) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::DATE) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_DEKTEC) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::DEKTEC) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_NSIS) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::NSIS) << "\"" << std::endl;

    TSUNIT_EQUAL(TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR) u"-" TS_USTRINGIFY(TS_COMMIT), ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
    TSUNIT_EQUAL(ts::VersionInfo::GetVersion(), ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
    TSUNIT_ASSERT(ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) != ts::VersionInfo::GetVersion(ts::VersionInfo::Format::LONG));
    TSUNIT_ASSERT(ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) != ts::VersionInfo::GetVersion(ts::VersionInfo::Format::NSIS));
}

// Test case: std::chrono::duration
void PlatformTest::testChrono()
{
    debug() << "PlatformTest: cn::nanoseconds: " << sizeof(cn::nanoseconds) << " bytes, rep: " << sizeof(cn::nanoseconds::rep) << " bytes" << std::endl
            << "PlatformTest: cn::microseconds: " << sizeof(cn::microseconds) << " bytes, rep: " << sizeof(cn::microseconds::rep) << " bytes" << std::endl
            << "PlatformTest: cn::milliseconds: " << sizeof(cn::milliseconds) << " bytes, rep: " << sizeof(cn::milliseconds::rep) << " bytes" << std::endl
            << "PlatformTest: cn::seconds: " << sizeof(cn::seconds) << " bytes, rep: " << sizeof(cn::seconds::rep) << " bytes" << std::endl
            << "PlatformTest: cn::minutes: " << sizeof(cn::minutes) << " bytes, rep: " << sizeof(cn::minutes::rep) << " bytes" << std::endl
            << "PlatformTest: cn::hours: " << sizeof(cn::hours) << " bytes, rep: " << sizeof(cn::hours::rep) << " bytes" << std::endl
            << "PlatformTest: cn::days: " << sizeof(cn::days) << " bytes, rep: " << sizeof(cn::days::rep) << " bytes" << std::endl
            << "PlatformTest: cn::weeks: " << sizeof(cn::weeks) << " bytes, rep: " << sizeof(cn::weeks::rep) << " bytes" << std::endl
            << "PlatformTest: cn::months: " << sizeof(cn::months) << " bytes, rep: " << sizeof(cn::months::rep) << " bytes" << std::endl
            << "PlatformTest: cn::years: " << sizeof(cn::years) << " bytes, rep: " << sizeof(cn::years::rep) << " bytes" << std::endl;

    TSUNIT_ASSERT(8 * sizeof(cn::nanoseconds::rep) >= 64);
    TSUNIT_ASSERT(8 * sizeof(cn::microseconds::rep) >= 55);
    TSUNIT_ASSERT(8 * sizeof(cn::milliseconds::rep) >= 45);
    TSUNIT_ASSERT(8 * sizeof(cn::seconds::rep) >= 35);
    TSUNIT_ASSERT(8 * sizeof(cn::minutes::rep) >= 29);
    TSUNIT_ASSERT(8 * sizeof(cn::hours::rep) >= 23);
    TSUNIT_ASSERT(8 * sizeof(cn::days::rep) >= 25);
    TSUNIT_ASSERT(8 * sizeof(cn::weeks::rep) >= 22);
    TSUNIT_ASSERT(8 * sizeof(cn::months::rep) >= 20);
    TSUNIT_ASSERT(8 * sizeof(cn::years::rep) >= 17);

    // Check if optimal implementation.
    TSUNIT_ASSUME(sizeof(cn::nanoseconds) == sizeof(cn::nanoseconds::rep));
    TSUNIT_ASSUME(sizeof(cn::microseconds) == sizeof(cn::microseconds::rep));
    TSUNIT_ASSUME(sizeof(cn::milliseconds) == sizeof(cn::milliseconds::rep));
    TSUNIT_ASSUME(sizeof(cn::seconds) == sizeof(cn::seconds::rep));
    TSUNIT_ASSUME(sizeof(cn::minutes) == sizeof(cn::minutes::rep));
    TSUNIT_ASSUME(sizeof(cn::hours) == sizeof(cn::hours::rep));
    TSUNIT_ASSUME(sizeof(cn::days) == sizeof(cn::days::rep));
    TSUNIT_ASSUME(sizeof(cn::weeks) == sizeof(cn::weeks::rep));
    TSUNIT_ASSUME(sizeof(cn::months) == sizeof(cn::months::rep));
    TSUNIT_ASSUME(sizeof(cn::years) == sizeof(cn::years::rep));
}
