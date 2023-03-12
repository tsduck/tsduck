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

    void testIntegerTypes();
    void test64bitLiterals();
    void testStringify();
    void testVersion();

    TSUNIT_TEST_BEGIN(PlatformTest);
    TSUNIT_TEST(testIntegerTypes);
    TSUNIT_TEST(test64bitLiterals);
    TSUNIT_TEST(testStringify);
    TSUNIT_TEST(testVersion);
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

// These overload shall be accepted is size_t is not a predefined integer type.
#if !defined(TS_SIZE_T_IS_STDINT)
namespace {
    void Overload32(uint32_t) {}
    void Overload64(uint64_t) {}
    void OverloadSizeT(size_t) {}
}
#endif

// Test case: predefined integer types.
void PlatformTest::testIntegerTypes()
{
    // To avoid compilation warnings when not referenced.
#if !defined(TS_SIZE_T_IS_STDINT)
    Overload32(0);
    Overload64(0);
    OverloadSizeT(0);
#endif

    debug()
        << "PlatformTest: TS_ADDRESS_BITS = " << TS_ADDRESS_BITS << std::endl
        << "PlatformTest: TS_SIZE_T_IS_STDINT is"
#if !defined(TS_SIZE_T_IS_STDINT)
        << " NOT"
#endif
        << " defined" << std::endl
        << "PlatformTest: sizeof(int) = " << sizeof(int)
        << ", sizeof(long) = " << sizeof(long)
        << ", sizeof(long long) = " << sizeof(long long)
        << ", sizeof(void*) = " << sizeof(void*) << std::endl
        << "PlatformTest: sizeof(char) = " << sizeof(char)
        << ", sizeof(unsigned char) = " << sizeof(unsigned char)
        << ", is_signed(char) = " << std::is_signed<char>::value << std::endl
        << "PlatformTest: TS_WCHAR_SIZE = " << TS_WCHAR_SIZE << std::endl
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

    TSUNIT_EQUAL(sizeof(wchar_t), TS_WCHAR_SIZE);

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
    uint64_t ui64 = TS_UCONST64(0xFEDCBA9876543210);
    int64_t i64 = TS_CONST64(0xFEDCBA9876543210);

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
