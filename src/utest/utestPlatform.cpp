//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    TSUNIT_DECLARE_TEST(Endianness);
    TSUNIT_DECLARE_TEST(IntegerTypes);
    TSUNIT_DECLARE_TEST(64bitLiterals);
    TSUNIT_DECLARE_TEST(Stringify);
    TSUNIT_DECLARE_TEST(Version);
    TSUNIT_DECLARE_TEST(Chrono);
    TSUNIT_DECLARE_TEST(SharedPtr);
};

TSUNIT_REGISTER(PlatformTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Test case: verify endianness.
TSUNIT_DEFINE_TEST(Endianness)
{
    uint32_t i = 0x01020304;
    char* c = reinterpret_cast<char*>(&i);

    if constexpr (std::endian::native == std::endian::little) {
        TSUNIT_EQUAL(4, c[0]);
        TSUNIT_EQUAL(3, c[1]);
        TSUNIT_EQUAL(2, c[2]);
        TSUNIT_EQUAL(1, c[3]);
    }
    else if constexpr (std::endian::native == std::endian::big) {
        TSUNIT_EQUAL(1, c[0]);
        TSUNIT_EQUAL(2, c[1]);
        TSUNIT_EQUAL(3, c[2]);
        TSUNIT_EQUAL(4, c[3]);
    }
    else {
        TSUNIT_FAIL("unknown endian");
    }
}

// Test case: predefined integer types.
TSUNIT_DEFINE_TEST(IntegerTypes)
{
    debug()
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
TSUNIT_DEFINE_TEST(64bitLiterals)
{
    uint64_t ui64 = 0xFEDCBA9876543210;
    int64_t i64 = 0xFEDCBA9876543210;

    TSUNIT_EQUAL(0x0FEDCBA9, uint32_t(ui64 >> 36));
    TSUNIT_EQUAL(0xFFEDCBA9, int32_t(i64 >> 36));
}

// Test case: stringification macro
TSUNIT_DEFINE_TEST(Stringify)
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
TSUNIT_DEFINE_TEST(Version)
{
    debug() << "PlatformTest: GetVersion(VERSION_SHORT) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_LONG) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::LONG) << "\"" << std::endl
            << "PlatformTest: GetVersion(VERSION_DATE) = \"" << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::DATE) << "\"" << std::endl;

    TSUNIT_EQUAL(TS_USTRINGIFY(TS_VERSION_MAJOR) u"." TS_USTRINGIFY(TS_VERSION_MINOR) u"-" TS_USTRINGIFY(TS_COMMIT), ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
    TSUNIT_EQUAL(ts::VersionInfo::GetVersion(), ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT));
    TSUNIT_ASSERT(ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) != ts::VersionInfo::GetVersion(ts::VersionInfo::Format::LONG));
}

// Test case: std::chrono::duration
TSUNIT_DEFINE_TEST(Chrono)
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

// Test case: std::share_ptr
TSUNIT_DEFINE_TEST(SharedPtr)
{
    using Ptr = std::shared_ptr<uint64_t>;
    Ptr ptr1(new uint64_t(0x0123456789ABCDEF));

    if (debugMode()) {
        // May trigger memory errors depending on implementations.
        debug() << "PlatformTest: sizeof shared_ptr: " << sizeof(ptr1) << " bytes" << std::endl
                << "   share_ptr instance: " << ts::UString::Dump(&ptr1, sizeof(ptr1), ts::UString::SINGLE_LINE) << std::endl;
        void** base = reinterpret_cast<void**>(&ptr1);
        const size_t count = sizeof(ptr1) / sizeof(void*);
        for (size_t i = 0; i < count; i++) {
            debug() << "    ptr[" << i << "]: " << ts::UString::Format(u"%0*X", 2*sizeof(void*), ptrdiff_t(base[i]));
            if (base[i] != nullptr) {
                    debug() << " -> " << ts::UString::Dump(base[i], 2 * sizeof(void*), ts::UString::SINGLE_LINE);
            }
            debug() << std::endl;
        }
    }

    TSUNIT_ASSERT(ptr1 != nullptr);
    TSUNIT_EQUAL(1, ptr1.use_count());

    Ptr ptr2;
    TSUNIT_ASSERT(ptr2 == nullptr);
    TSUNIT_EQUAL(1, ptr1.use_count());
    TSUNIT_EQUAL(0, ptr2.use_count());

    ptr2 = ptr1;
    TSUNIT_EQUAL(2, ptr1.use_count());
    TSUNIT_EQUAL(2, ptr2.use_count());

    if (debugMode()) {
        // May trigger memory errors depending on implementations.
        debug() << "PlatformTest: sizeof shared_ptr: " << sizeof(ptr1) << " bytes" << std::endl
                << "   share_ptr instance: " << ts::UString::Dump(&ptr1, sizeof(ptr1), ts::UString::SINGLE_LINE) << std::endl;
        void** base = reinterpret_cast<void**>(&ptr1);
        const size_t count = sizeof(ptr1) / sizeof(void*);
        for (size_t i = 0; i < count; i++) {
            debug() << "    ptr[" << i << "]: " << ts::UString::Format(u"%0*X", 2*sizeof(void*), ptrdiff_t(base[i]));
            if (base[i] != nullptr) {
                debug() << " -> " << ts::UString::Dump(base[i], 2 * sizeof(void*), ts::UString::SINGLE_LINE);
            }
            debug() << std::endl;
        }
    }
}
