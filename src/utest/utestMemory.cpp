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
//  TSUnit test suite for tsMemory.h
//
//----------------------------------------------------------------------------

#include "tsMemory.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MemoryTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testMemoryBarrier();
    void testGetUInt8();
    void testGetUInt16BE();
    void testGetUInt16LE();
    void testGetUInt24BE();
    void testGetUInt24LE();
    void testGetUInt32BE();
    void testGetUInt32LE();
    void testGetUInt40BE();
    void testGetUInt40LE();
    void testGetUInt48BE();
    void testGetUInt48LE();
    void testGetUInt64BE();
    void testGetUInt64LE();
    void testGetInt8();
    void testGetInt16BE();
    void testGetInt16LE();
    void testGetInt24BE();
    void testGetInt24LE();
    void testGetInt32BE();
    void testGetInt32LE();
    void testGetInt40BE();
    void testGetInt40LE();
    void testGetInt48BE();
    void testGetInt48LE();
    void testGetInt64BE();
    void testGetInt64LE();
    void testPutUInt8();
    void testPutUInt16BE();
    void testPutUInt16LE();
    void testPutUInt24BE();
    void testPutUInt24LE();
    void testPutUInt32BE();
    void testPutUInt32LE();
    void testPutUInt64BE();
    void testPutUInt64LE();
    void testPutUInt48BE();
    void testPutUInt48LE();
    void testPutInt8();
    void testPutInt16BE();
    void testPutInt16LE();
    void testPutInt24BE();
    void testPutInt24LE();
    void testPutInt32BE();
    void testPutInt32LE();
    void testPutInt64BE();
    void testPutInt64LE();
    void testGetIntVarBE();
    void testGetIntVarLE();
    void testPutIntVarBE();
    void testPutIntVarLE();

    TSUNIT_TEST_BEGIN(MemoryTest);
    TSUNIT_TEST(testMemoryBarrier);
    TSUNIT_TEST(testGetUInt8);
    TSUNIT_TEST(testGetUInt16BE);
    TSUNIT_TEST(testGetUInt16LE);
    TSUNIT_TEST(testGetUInt24BE);
    TSUNIT_TEST(testGetUInt24LE);
    TSUNIT_TEST(testGetUInt32BE);
    TSUNIT_TEST(testGetUInt32LE);
    TSUNIT_TEST(testGetUInt40BE);
    TSUNIT_TEST(testGetUInt40LE);
    TSUNIT_TEST(testGetUInt48BE);
    TSUNIT_TEST(testGetUInt48LE);
    TSUNIT_TEST(testGetUInt64BE);
    TSUNIT_TEST(testGetUInt64LE);
    TSUNIT_TEST(testGetInt8);
    TSUNIT_TEST(testGetInt16BE);
    TSUNIT_TEST(testGetInt16LE);
    TSUNIT_TEST(testGetInt24BE);
    TSUNIT_TEST(testGetInt24LE);
    TSUNIT_TEST(testGetInt32BE);
    TSUNIT_TEST(testGetInt32LE);
    TSUNIT_TEST(testGetInt40BE);
    TSUNIT_TEST(testGetInt40LE);
    TSUNIT_TEST(testGetInt48BE);
    TSUNIT_TEST(testGetInt48LE);
    TSUNIT_TEST(testGetInt64BE);
    TSUNIT_TEST(testGetInt64LE);
    TSUNIT_TEST(testPutUInt8);
    TSUNIT_TEST(testPutUInt16BE);
    TSUNIT_TEST(testPutUInt16LE);
    TSUNIT_TEST(testPutUInt24BE);
    TSUNIT_TEST(testPutUInt24LE);
    TSUNIT_TEST(testPutUInt32BE);
    TSUNIT_TEST(testPutUInt32LE);
    TSUNIT_TEST(testPutUInt64BE);
    TSUNIT_TEST(testPutUInt64LE);
    TSUNIT_TEST(testPutUInt48BE);
    TSUNIT_TEST(testPutUInt48LE);
    TSUNIT_TEST(testPutInt8);
    TSUNIT_TEST(testPutInt16BE);
    TSUNIT_TEST(testPutInt16LE);
    TSUNIT_TEST(testPutInt24BE);
    TSUNIT_TEST(testPutInt24LE);
    TSUNIT_TEST(testPutInt32BE);
    TSUNIT_TEST(testPutInt32LE);
    TSUNIT_TEST(testPutInt64BE);
    TSUNIT_TEST(testPutInt64LE);
    TSUNIT_TEST(testGetIntVarBE);
    TSUNIT_TEST(testGetIntVarLE);
    TSUNIT_TEST(testPutIntVarBE);
    TSUNIT_TEST(testPutIntVarLE);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(MemoryTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void MemoryTest::beforeTest()
{
}

// Test suite cleanup method.
void MemoryTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Reference byte array: 256 bytes, index == value
namespace {
    const uint8_t _bytes[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
        0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
        0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
        0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
        0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    };
}

void MemoryTest::testMemoryBarrier()
{
    // There is no easy way to exhibit the proper malfunctioning of a memory barrier.
    // Here, we basically test that it compiles and does not mess up too much.

    int i = 1;
    ts::MemoryBarrier();
    i = 2;
    TSUNIT_EQUAL(2, i);
}

void MemoryTest::testGetUInt8()
{
    TSUNIT_EQUAL(0x07, ts::GetUInt8(_bytes + 0x07));
}

void MemoryTest::testGetUInt16BE()
{
    TSUNIT_EQUAL(0x2324, ts::GetUInt16BE(_bytes + 0x23));
}

void MemoryTest::testGetUInt16LE()
{
    TSUNIT_EQUAL(0x2423, ts::GetUInt16LE(_bytes + 0x23));
}

void MemoryTest::testGetUInt24BE()
{
    TSUNIT_EQUAL(0x101112, ts::GetUInt24BE(_bytes + 0x10));
    TSUNIT_EQUAL(0xCECFD0, ts::GetUInt24BE(_bytes + 0xCE));
}

void MemoryTest::testGetUInt24LE()
{
    TSUNIT_EQUAL(0x121110, ts::GetUInt24LE(_bytes + 0x10));
    TSUNIT_EQUAL(0xD0CFCE, ts::GetUInt24LE(_bytes + 0xCE));
}

void MemoryTest::testGetUInt32BE()
{
    TSUNIT_EQUAL(0x4748494A, ts::GetUInt32BE(_bytes + 0x47));
}

void MemoryTest::testGetUInt32LE()
{
    TSUNIT_EQUAL(0x4A494847, ts::GetUInt32LE(_bytes + 0x47));
}

void MemoryTest::testGetUInt40BE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x000000898A8B8C8D), ts::GetUInt40BE(_bytes + 0x89));
}

void MemoryTest::testGetUInt40LE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x0000008D8C8B8A89), ts::GetUInt40LE(_bytes + 0x89));
}

void MemoryTest::testGetUInt48BE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x0000898A8B8C8D8E), ts::GetUInt48BE(_bytes + 0x89));
}

void MemoryTest::testGetUInt48LE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x00008E8D8C8B8A89), ts::GetUInt48LE(_bytes + 0x89));
}

void MemoryTest::testGetUInt64BE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x898A8B8C8D8E8F90), ts::GetUInt64BE(_bytes + 0x89));
}

void MemoryTest::testGetUInt64LE()
{
    TSUNIT_EQUAL(TS_UCONST64(0x908F8E8D8C8B8A89), ts::GetUInt64LE(_bytes + 0x89));
}

void MemoryTest::testGetInt8()
{
    TSUNIT_EQUAL(3, ts::GetInt8(_bytes + 0x03));
}

void MemoryTest::testGetInt16BE()
{
    TSUNIT_EQUAL(-12593, ts::GetInt16BE(_bytes + 0xCE)); // 0xCECF
}

void MemoryTest::testGetInt16LE()
{
    TSUNIT_EQUAL(-12338, ts::GetInt16LE(_bytes + 0xCE)); // 0xCFCE
}

void MemoryTest::testGetInt24BE()
{
    TSUNIT_EQUAL(0x101112, ts::GetInt24BE(_bytes + 0x10));
    TSUNIT_EQUAL(-3223600, ts::GetInt24BE(_bytes + 0xCE)); // 0xFFCECFD0
}

void MemoryTest::testGetInt24LE()
{
    TSUNIT_EQUAL(0x121110, ts::GetInt24LE(_bytes + 0x10));
    TSUNIT_EQUAL(-3092530, ts::GetInt24LE(_bytes + 0xCE)); // 0xFFD0CFCE
}

void MemoryTest::testGetInt32BE()
{
    TSUNIT_EQUAL(-2122153084, ts::GetInt32BE(_bytes + 0x81)); // 0x81828384
}

void MemoryTest::testGetInt32LE()
{
    TSUNIT_EQUAL(-2071756159, ts::GetInt32LE(_bytes + 0x81)); // 0x84838281
}

void MemoryTest::testGetInt40BE()
{
    TSUNIT_EQUAL(TS_CONST64(-219885416496), ts::GetInt40BE(_bytes + 0xCC)); // 0xCCCDCECFD0
}

void MemoryTest::testGetInt40LE()
{
    TSUNIT_EQUAL(TS_CONST64(-202671993396), ts::GetInt40LE(_bytes + 0xCC)); // 0xD0CFCECDCC
}

void MemoryTest::testGetInt48BE()
{
    TSUNIT_EQUAL(TS_CONST64(-56290666622767), ts::GetInt48BE(_bytes + 0xCC)); // 0xCCCDCECFD0D1
}

void MemoryTest::testGetInt48LE()
{
    TSUNIT_EQUAL(TS_CONST64(-50780206871092), ts::GetInt48LE(_bytes + 0xCC)); // 0xD1D0CFCECDCC
}

void MemoryTest::testGetInt64BE()
{
    TSUNIT_EQUAL(TS_CONST64(-3689065127789604141), ts::GetInt64BE(_bytes + 0xCC)); // 0xCCCDCECFD0D1D2D3
}

void MemoryTest::testGetInt64LE()
{
    TSUNIT_EQUAL(TS_CONST64(-3183251291827679796), ts::GetInt64LE(_bytes + 0xCC)); // 0xD3D2D1D0CFCECDCC
}

void MemoryTest::testPutUInt8()
{
    uint8_t out[16];
    ts::PutUInt8(out, 0x78);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x78, 1));
}

void MemoryTest::testPutUInt16BE()
{
    uint8_t out[16];
    ts::PutUInt16BE(out, 0x898A);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 2));
}

void MemoryTest::testPutUInt16LE()
{
    uint8_t out[16];
    ts::PutUInt16LE(out, 0x8A89);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 2));
}

void MemoryTest::testPutUInt24BE()
{
    uint8_t out[16];
    ts::PutUInt24BE(out, 0x898A8B);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 3));
}

void MemoryTest::testPutUInt24LE()
{
    uint8_t out[16];
    ts::PutUInt24LE(out, 0x8B8A89);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 3));
}

void MemoryTest::testPutUInt32BE()
{
    uint8_t out[16];
    ts::PutUInt32BE(out, 0x56575859);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x56, 4));
}

void MemoryTest::testPutUInt32LE()
{
    uint8_t out[16];
    ts::PutUInt32LE(out, 0x59585756);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x56, 4));
}

void MemoryTest::testPutUInt64BE()
{
    uint8_t out[16];
    ts::PutUInt64BE(out, TS_UCONST64(0x898A8B8C8D8E8F90));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 8));
}

void MemoryTest::testPutUInt64LE()
{
    uint8_t out[16];
    ts::PutUInt64LE(out, TS_UCONST64(0x908F8E8D8C8B8A89));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 8));
}


void MemoryTest::testPutUInt48BE()
{
    uint8_t out[16];
    ts::PutUInt48BE(out, TS_UCONST64(0x0000898A8B8C8D8E));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 6));
}

void MemoryTest::testPutUInt48LE()
{
    uint8_t out[16];
    ts::PutUInt48LE(out, TS_UCONST64(0x00008E8D8C8B8A89));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 6));
}


void MemoryTest::testPutInt8()
{
    uint8_t out[16];
    ts::PutInt8(out, -2);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xFE, 1));
}

void MemoryTest::testPutInt16BE()
{
    uint8_t out[16];
    ts::PutInt16BE(out, -12593); // 0xCECF
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCE, 2));
}

void MemoryTest::testPutInt16LE()
{
    uint8_t out[16];
    ts::PutInt16LE(out, -12338); // 0xCFCE
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCE, 2));
}

void MemoryTest::testPutInt24BE()
{
    uint8_t out[16];
    ts::PutInt24BE(out, -3223600); // 0xFFCECFD0
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCE, 3));
    ts::PutInt24BE(out, 0x101112);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x10, 3));
}

void MemoryTest::testPutInt24LE()
{
    uint8_t out[16];
    ts::PutInt24LE(out, -3092530); // 0xFFD0CFCE
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCE, 3));
    ts::PutInt24LE(out, 0x121110);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x10, 3));
}

void MemoryTest::testPutInt32BE()
{
    uint8_t out[16];
    ts::PutInt32BE(out, -2122153084); // 0x81828384
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x81, 4));
}

void MemoryTest::testPutInt32LE()
{
    uint8_t out[16];
    ts::PutInt32LE(out, -2071756159); // 0x84838281
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x81, 4));
}

void MemoryTest::testPutInt64BE()
{
    uint8_t out[16];
    ts::PutInt64BE(out, TS_CONST64(-3689065127789604141)); // 0xCCCDCECFD0D1D2D3
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCC, 8));
}

void MemoryTest::testPutInt64LE()
{
    uint8_t out[16];
    ts::PutInt64LE(out, TS_CONST64(-3183251291827679796)); // 0xD3D2D1D0CFCECDCC
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0xCC, 8));
}

void MemoryTest::testGetIntVarBE()
{
    TSUNIT_EQUAL(0x07, ts::GetIntVarBE<uint8_t>(_bytes + 0x07, 1));
    TSUNIT_EQUAL(0x2324, ts::GetIntVarBE<uint16_t>(_bytes + 0x23, 2));
    TSUNIT_EQUAL(0x101112, ts::GetIntVarBE<uint32_t>(_bytes + 0x10, 3));
    TSUNIT_EQUAL(0xCECFD0, ts::GetIntVarBE<uint32_t>(_bytes + 0xCE, 3));
    TSUNIT_EQUAL(0x4748494A, ts::GetIntVarBE<uint32_t>(_bytes + 0x47, 4));
    TSUNIT_EQUAL(TS_UCONST64(0x000000898A8B8C8D), ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 5));
    TSUNIT_EQUAL(TS_UCONST64(0x0000898A8B8C8D8E), ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 6));
    TSUNIT_EQUAL(TS_UCONST64(0x898A8B8C8D8E8F90), ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 8));
}

void MemoryTest::testGetIntVarLE()
{
    TSUNIT_EQUAL(0x07, ts::GetIntVarLE<uint8_t>(_bytes + 0x07, 1));
    TSUNIT_EQUAL(0x2423, ts::GetIntVarLE<uint16_t>(_bytes + 0x23, 2));
    TSUNIT_EQUAL(0x121110, ts::GetIntVarLE<uint32_t>(_bytes + 0x10, 3));
    TSUNIT_EQUAL(0xD0CFCE, ts::GetIntVarLE<uint32_t>(_bytes + 0xCE, 3));
    TSUNIT_EQUAL(0x4A494847, ts::GetIntVarLE<uint32_t>(_bytes + 0x47, 4));
    TSUNIT_EQUAL(TS_UCONST64(0x0000008D8C8B8A89), ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 5));
    TSUNIT_EQUAL(TS_UCONST64(0x00008E8D8C8B8A89), ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 6));
    TSUNIT_EQUAL(TS_UCONST64(0x908F8E8D8C8B8A89), ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 8));
}

void MemoryTest::testPutIntVarBE()
{
    uint8_t out[16];
    ts::PutIntVarBE(out, 1, 0x78);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x78, 1));
    ts::PutIntVarBE(out, 2, 0x898A);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 2));
    ts::PutIntVarBE(out, 3, 0x898A8B);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 3));
    ts::PutIntVarBE(out, 4, 0x56575859);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x56, 4));
    ts::PutIntVarBE(out, 6, TS_UCONST64(0x0000898A8B8C8D8E));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 6));
    ts::PutIntVarBE(out, 8, TS_UCONST64(0x898A8B8C8D8E8F90));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 8));
}

void MemoryTest::testPutIntVarLE()
{
    uint8_t out[16];
    ts::PutIntVarLE(out, 1, 0x78);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x78, 1));
    ts::PutIntVarLE(out, 2, 0x8A89);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 2));
    ts::PutIntVarLE(out, 3, 0x8B8A89);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 3));
    ts::PutIntVarLE(out, 4, 0x59585756);
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x56, 4));
    ts::PutIntVarLE(out, 6, TS_UCONST64(0x00008E8D8C8B8A89));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 6));
    ts::PutIntVarLE(out, 8, TS_UCONST64(0x908F8E8D8C8B8A89));
    TSUNIT_EQUAL(0, ::memcmp(out, _bytes + 0x89, 8));
}
