//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    TSUNIT_DECLARE_TEST(GetUInt8);
    TSUNIT_DECLARE_TEST(GetUInt16BE);
    TSUNIT_DECLARE_TEST(GetUInt16LE);
    TSUNIT_DECLARE_TEST(GetUInt24BE);
    TSUNIT_DECLARE_TEST(GetUInt24LE);
    TSUNIT_DECLARE_TEST(GetUInt32BE);
    TSUNIT_DECLARE_TEST(GetUInt32LE);
    TSUNIT_DECLARE_TEST(GetUInt40BE);
    TSUNIT_DECLARE_TEST(GetUInt40LE);
    TSUNIT_DECLARE_TEST(GetUInt48BE);
    TSUNIT_DECLARE_TEST(GetUInt48LE);
    TSUNIT_DECLARE_TEST(GetUInt56BE);
    TSUNIT_DECLARE_TEST(GetUInt56LE);
    TSUNIT_DECLARE_TEST(GetUInt64BE);
    TSUNIT_DECLARE_TEST(GetUInt64LE);
    TSUNIT_DECLARE_TEST(GetInt8);
    TSUNIT_DECLARE_TEST(GetInt16BE);
    TSUNIT_DECLARE_TEST(GetInt16LE);
    TSUNIT_DECLARE_TEST(GetInt24BE);
    TSUNIT_DECLARE_TEST(GetInt24LE);
    TSUNIT_DECLARE_TEST(GetInt32BE);
    TSUNIT_DECLARE_TEST(GetInt32LE);
    TSUNIT_DECLARE_TEST(GetInt40BE);
    TSUNIT_DECLARE_TEST(GetInt40LE);
    TSUNIT_DECLARE_TEST(GetInt48BE);
    TSUNIT_DECLARE_TEST(GetInt48LE);
    TSUNIT_DECLARE_TEST(GetInt56BE);
    TSUNIT_DECLARE_TEST(GetInt56LE);
    TSUNIT_DECLARE_TEST(GetInt64BE);
    TSUNIT_DECLARE_TEST(GetInt64LE);
    TSUNIT_DECLARE_TEST(PutUInt8);
    TSUNIT_DECLARE_TEST(PutUInt16BE);
    TSUNIT_DECLARE_TEST(PutUInt16LE);
    TSUNIT_DECLARE_TEST(PutUInt24BE);
    TSUNIT_DECLARE_TEST(PutUInt24LE);
    TSUNIT_DECLARE_TEST(PutUInt32BE);
    TSUNIT_DECLARE_TEST(PutUInt32LE);
    TSUNIT_DECLARE_TEST(PutUInt64BE);
    TSUNIT_DECLARE_TEST(PutUInt64LE);
    TSUNIT_DECLARE_TEST(PutUInt48BE);
    TSUNIT_DECLARE_TEST(PutUInt48LE);
    TSUNIT_DECLARE_TEST(PutUInt56BE);
    TSUNIT_DECLARE_TEST(PutUInt56LE);
    TSUNIT_DECLARE_TEST(PutInt8);
    TSUNIT_DECLARE_TEST(PutInt16BE);
    TSUNIT_DECLARE_TEST(PutInt16LE);
    TSUNIT_DECLARE_TEST(PutInt24BE);
    TSUNIT_DECLARE_TEST(PutInt24LE);
    TSUNIT_DECLARE_TEST(PutInt32BE);
    TSUNIT_DECLARE_TEST(PutInt32LE);
    TSUNIT_DECLARE_TEST(PutInt64BE);
    TSUNIT_DECLARE_TEST(PutInt64LE);
    TSUNIT_DECLARE_TEST(PutInt40BE);
    TSUNIT_DECLARE_TEST(PutInt40LE);
    TSUNIT_DECLARE_TEST(PutInt48BE);
    TSUNIT_DECLARE_TEST(PutInt48LE);
    TSUNIT_DECLARE_TEST(PutInt56BE);
    TSUNIT_DECLARE_TEST(PutInt56LE);
    TSUNIT_DECLARE_TEST(GetIntVarBE);
    TSUNIT_DECLARE_TEST(GetIntVarLE);
    TSUNIT_DECLARE_TEST(PutIntVarBE);
    TSUNIT_DECLARE_TEST(PutIntVarLE);
    TSUNIT_DECLARE_TEST(GetIntFixBE);
    TSUNIT_DECLARE_TEST(GetIntFixLE);
    TSUNIT_DECLARE_TEST(PutIntFixBE);
    TSUNIT_DECLARE_TEST(PutIntFixLE);
    TSUNIT_DECLARE_TEST(LocatePattern);
    TSUNIT_DECLARE_TEST(LocateZeroZero);
    TSUNIT_DECLARE_TEST(Xor);
};

TSUNIT_REGISTER(MemoryTest);


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

TSUNIT_DEFINE_TEST(GetUInt8)
{
    TSUNIT_EQUAL(0x07, ts::GetUInt8(_bytes + 0x07));
}

TSUNIT_DEFINE_TEST(GetUInt16BE)
{
    TSUNIT_EQUAL(0x2324, ts::GetUInt16BE(_bytes + 0x23));
}

TSUNIT_DEFINE_TEST(GetUInt16LE)
{
    TSUNIT_EQUAL(0x2423, ts::GetUInt16LE(_bytes + 0x23));
}

TSUNIT_DEFINE_TEST(GetUInt24BE)
{
    TSUNIT_EQUAL(0x101112, ts::GetUInt24BE(_bytes + 0x10));
    TSUNIT_EQUAL(0xCECFD0, ts::GetUInt24BE(_bytes + 0xCE));
}

TSUNIT_DEFINE_TEST(GetUInt24LE)
{
    TSUNIT_EQUAL(0x121110, ts::GetUInt24LE(_bytes + 0x10));
    TSUNIT_EQUAL(0xD0CFCE, ts::GetUInt24LE(_bytes + 0xCE));
}

TSUNIT_DEFINE_TEST(GetUInt32BE)
{
    TSUNIT_EQUAL(0x4748494A, ts::GetUInt32BE(_bytes + 0x47));
}

TSUNIT_DEFINE_TEST(GetUInt32LE)
{
    TSUNIT_EQUAL(0x4A494847, ts::GetUInt32LE(_bytes + 0x47));
}

TSUNIT_DEFINE_TEST(GetUInt40BE)
{
    TSUNIT_EQUAL(0x000000898A8B8C8D, ts::GetUInt40BE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt40LE)
{
    TSUNIT_EQUAL(0x0000008D8C8B8A89, ts::GetUInt40LE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt48BE)
{
    TSUNIT_EQUAL(0x0000898A8B8C8D8E, ts::GetUInt48BE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt48LE)
{
    TSUNIT_EQUAL(0x00008E8D8C8B8A89, ts::GetUInt48LE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt56BE)
{
    TSUNIT_EQUAL(0x00898A8B8C8D8E8F, ts::GetUInt56BE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt56LE)
{
    TSUNIT_EQUAL(0x008F8E8D8C8B8A89, ts::GetUInt56LE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt64BE)
{
    TSUNIT_EQUAL(0x898A8B8C8D8E8F90, ts::GetUInt64BE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetUInt64LE)
{
    TSUNIT_EQUAL(0x908F8E8D8C8B8A89, ts::GetUInt64LE(_bytes + 0x89));
}

TSUNIT_DEFINE_TEST(GetInt8)
{
    TSUNIT_EQUAL(3, ts::GetInt8(_bytes + 0x03));
}

TSUNIT_DEFINE_TEST(GetInt16BE)
{
    TSUNIT_EQUAL(-12593, ts::GetInt16BE(_bytes + 0xCE)); // 0xCECF
}

TSUNIT_DEFINE_TEST(GetInt16LE)
{
    TSUNIT_EQUAL(-12338, ts::GetInt16LE(_bytes + 0xCE)); // 0xCFCE
}

TSUNIT_DEFINE_TEST(GetInt24BE)
{
    TSUNIT_EQUAL(0x101112, ts::GetInt24BE(_bytes + 0x10));
    TSUNIT_EQUAL(-3223600, ts::GetInt24BE(_bytes + 0xCE)); // 0xFFCECFD0
}

TSUNIT_DEFINE_TEST(GetInt24LE)
{
    TSUNIT_EQUAL(0x121110, ts::GetInt24LE(_bytes + 0x10));
    TSUNIT_EQUAL(-3092530, ts::GetInt24LE(_bytes + 0xCE)); // 0xFFD0CFCE
}

TSUNIT_DEFINE_TEST(GetInt32BE)
{
    TSUNIT_EQUAL(-2122153084, ts::GetInt32BE(_bytes + 0x81)); // 0x81828384
}

TSUNIT_DEFINE_TEST(GetInt32LE)
{
    TSUNIT_EQUAL(-2071756159, ts::GetInt32LE(_bytes + 0x81)); // 0x84838281
}

TSUNIT_DEFINE_TEST(GetInt40BE)
{
    TSUNIT_EQUAL(-219885416496, ts::GetInt40BE(_bytes + 0xCC)); // 0xCCCDCECFD0
}

TSUNIT_DEFINE_TEST(GetInt40LE)
{
    TSUNIT_EQUAL(-202671993396, ts::GetInt40LE(_bytes + 0xCC)); // 0xD0CFCECDCC
}

TSUNIT_DEFINE_TEST(GetInt48BE)
{
    TSUNIT_EQUAL(-56290666622767, ts::GetInt48BE(_bytes + 0xCC)); // 0xCCCDCECFD0D1
}

TSUNIT_DEFINE_TEST(GetInt48LE)
{
    TSUNIT_EQUAL(-50780206871092, ts::GetInt48LE(_bytes + 0xCC)); // 0xD1D0CFCECDCC
}

TSUNIT_DEFINE_TEST(GetInt56BE)
{
    TSUNIT_EQUAL(-14410410655428142, ts::GetInt56BE(_bytes + 0xCC)); // 0xCCCDCECFD0D1D2
}

TSUNIT_DEFINE_TEST(GetInt56LE)
{
    TSUNIT_EQUAL(-12717154158850612, ts::GetInt56LE(_bytes + 0xCC)); // 0xD2D1D0CFCECDCC
}

TSUNIT_DEFINE_TEST(GetInt64BE)
{
    TSUNIT_EQUAL(-3689065127789604141, ts::GetInt64BE(_bytes + 0xCC)); // 0xCCCDCECFD0D1D2D3
}

TSUNIT_DEFINE_TEST(GetInt64LE)
{
    TSUNIT_EQUAL(-3183251291827679796, ts::GetInt64LE(_bytes + 0xCC)); // 0xD3D2D1D0CFCECDCC
}

TSUNIT_DEFINE_TEST(PutUInt8)
{
    uint8_t out[16];
    ts::PutUInt8(out, 0x78);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x78, 1));
}

TSUNIT_DEFINE_TEST(PutUInt16BE)
{
    uint8_t out[16];
    ts::PutUInt16BE(out, 0x898A);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
}

TSUNIT_DEFINE_TEST(PutUInt16LE)
{
    uint8_t out[16];
    ts::PutUInt16LE(out, 0x8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
}

TSUNIT_DEFINE_TEST(PutUInt24BE)
{
    uint8_t out[16];
    ts::PutUInt24BE(out, 0x898A8B);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
}

TSUNIT_DEFINE_TEST(PutUInt24LE)
{
    uint8_t out[16];
    ts::PutUInt24LE(out, 0x8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
}

TSUNIT_DEFINE_TEST(PutUInt32BE)
{
    uint8_t out[16];
    ts::PutUInt32BE(out, 0x56575859);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
}

TSUNIT_DEFINE_TEST(PutUInt32LE)
{
    uint8_t out[16];
    ts::PutUInt32LE(out, 0x59585756);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
}

TSUNIT_DEFINE_TEST(PutUInt64BE)
{
    uint8_t out[16];
    ts::PutUInt64BE(out, 0x898A8B8C8D8E8F90);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(PutUInt64LE)
{
    uint8_t out[16];
    ts::PutUInt64LE(out, 0x908F8E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(PutUInt48BE)
{
    uint8_t out[16];
    ts::PutUInt48BE(out, 0x0000898A8B8C8D8E);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
}

TSUNIT_DEFINE_TEST(PutUInt48LE)
{
    uint8_t out[16];
    ts::PutUInt48LE(out, 0x00008E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
}

TSUNIT_DEFINE_TEST(PutUInt56BE)
{
    uint8_t out[16];
    ts::PutUInt56BE(out, 0x00898A8B8C8D8E8F);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 7));
}

TSUNIT_DEFINE_TEST(PutUInt56LE)
{
    uint8_t out[16];
    ts::PutUInt56LE(out, 0x008F8E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 7));
}

TSUNIT_DEFINE_TEST(PutInt8)
{
    uint8_t out[16];
    ts::PutInt8(out, -2);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xFE, 1));
}

TSUNIT_DEFINE_TEST(PutInt16BE)
{
    uint8_t out[16];
    ts::PutInt16BE(out, -12593); // 0xCECF
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCE, 2));
}

TSUNIT_DEFINE_TEST(PutInt16LE)
{
    uint8_t out[16];
    ts::PutInt16LE(out, -12338); // 0xCFCE
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCE, 2));
}

TSUNIT_DEFINE_TEST(PutInt24BE)
{
    uint8_t out[16];
    ts::PutInt24BE(out, -3223600); // 0xFFCECFD0
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCE, 3));
    ts::PutInt24BE(out, 0x101112);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x10, 3));
}

TSUNIT_DEFINE_TEST(PutInt24LE)
{
    uint8_t out[16];
    ts::PutInt24LE(out, -3092530); // 0xFFD0CFCE
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCE, 3));
    ts::PutInt24LE(out, 0x121110);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x10, 3));
}

TSUNIT_DEFINE_TEST(PutInt32BE)
{
    uint8_t out[16];
    ts::PutInt32BE(out, -2122153084); // 0x81828384
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x81, 4));
}

TSUNIT_DEFINE_TEST(PutInt32LE)
{
    uint8_t out[16];
    ts::PutInt32LE(out, -2071756159); // 0x84838281
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x81, 4));
}

TSUNIT_DEFINE_TEST(PutInt40BE)
{
    uint8_t out[16];
    ts::PutInt40BE(out, -219885416496); // 0xCCCDCECFD0
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 5));
}

TSUNIT_DEFINE_TEST(PutInt40LE)
{
    uint8_t out[16];
    ts::PutInt40LE(out, -202671993396); // 0xD0CFCECDCC
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 5));
}

TSUNIT_DEFINE_TEST(PutInt48BE)
{
    uint8_t out[16];
    ts::PutInt48BE(out, -56290666622767); // 0xCCCDCECFD0D1
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 6));
}

TSUNIT_DEFINE_TEST(PutInt48LE)
{
    uint8_t out[16];
    ts::PutInt48LE(out, -50780206871092); // 0xD1D0CFCECDCC
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 6));
}

TSUNIT_DEFINE_TEST(PutInt56BE)
{
    uint8_t out[16];
    ts::PutInt56BE(out, -14410410655428142); // 0xCCCDCECFD0D1D2
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 7));
}

TSUNIT_DEFINE_TEST(PutInt56LE)
{
    uint8_t out[16];
    ts::PutInt56LE(out, -12717154158850612); // 0xD2D1D0CFCECDCC
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 7));
}

TSUNIT_DEFINE_TEST(PutInt64BE)
{
    uint8_t out[16];
    ts::PutInt64BE(out, -3689065127789604141); // 0xCCCDCECFD0D1D2D3
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 8));
}

TSUNIT_DEFINE_TEST(PutInt64LE)
{
    uint8_t out[16];
    ts::PutInt64LE(out, -3183251291827679796); // 0xD3D2D1D0CFCECDCC
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0xCC, 8));
}

TSUNIT_DEFINE_TEST(GetIntVarBE)
{
    TSUNIT_EQUAL(0x07, ts::GetIntVarBE<uint8_t>(_bytes + 0x07, 1));
    TSUNIT_EQUAL(0x2324, ts::GetIntVarBE<uint16_t>(_bytes + 0x23, 2));
    TSUNIT_EQUAL(0x101112, ts::GetIntVarBE<uint32_t>(_bytes + 0x10, 3));
    TSUNIT_EQUAL(0xCECFD0, ts::GetIntVarBE<uint32_t>(_bytes + 0xCE, 3));
    TSUNIT_EQUAL(0x4748494A, ts::GetIntVarBE<uint32_t>(_bytes + 0x47, 4));
    TSUNIT_EQUAL(0x000000898A8B8C8D, ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 5));
    TSUNIT_EQUAL(0x0000898A8B8C8D8E, ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 6));
    TSUNIT_EQUAL(0x0088898A8B8C8D8E, ts::GetIntVarBE<uint64_t>(_bytes + 0x88, 7));
    TSUNIT_EQUAL(0x898A8B8C8D8E8F90, ts::GetIntVarBE<uint64_t>(_bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(GetIntFixBE)
{
    TSUNIT_EQUAL(0x07, (ts::GetIntFixBE<1, uint8_t>(_bytes + 0x07)));
    TSUNIT_EQUAL(0x2324, (ts::GetIntFixBE<2, uint16_t>(_bytes + 0x23)));
    TSUNIT_EQUAL(0x101112, (ts::GetIntFixBE<3, uint32_t>(_bytes + 0x10)));
    TSUNIT_EQUAL(0xCECFD0, (ts::GetIntFixBE<3, uint32_t>(_bytes + 0xCE)));
    TSUNIT_EQUAL(0x4748494A, (ts::GetIntFixBE<4, uint32_t>(_bytes + 0x47)));
    TSUNIT_EQUAL(0x000000898A8B8C8D, (ts::GetIntFixBE<5, uint64_t>(_bytes + 0x89)));
    TSUNIT_EQUAL(0x0000898A8B8C8D8E, (ts::GetIntFixBE<6, uint64_t>(_bytes + 0x89)));
    TSUNIT_EQUAL(0x0088898A8B8C8D8E, (ts::GetIntFixBE<7, uint64_t>(_bytes + 0x88)));
    TSUNIT_EQUAL(0x898A8B8C8D8E8F90, (ts::GetIntFixBE<8, uint64_t>(_bytes + 0x89)));
}

TSUNIT_DEFINE_TEST(GetIntVarLE)
{
    TSUNIT_EQUAL(0x07, ts::GetIntVarLE<uint8_t>(_bytes + 0x07, 1));
    TSUNIT_EQUAL(0x2423, ts::GetIntVarLE<uint16_t>(_bytes + 0x23, 2));
    TSUNIT_EQUAL(0x121110, ts::GetIntVarLE<uint32_t>(_bytes + 0x10, 3));
    TSUNIT_EQUAL(0xD0CFCE, ts::GetIntVarLE<uint32_t>(_bytes + 0xCE, 3));
    TSUNIT_EQUAL(0x4A494847, ts::GetIntVarLE<uint32_t>(_bytes + 0x47, 4));
    TSUNIT_EQUAL(0x0000008D8C8B8A89, ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 5));
    TSUNIT_EQUAL(0x00008E8D8C8B8A89, ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 6));
    TSUNIT_EQUAL(0x008E8D8C8B8A8988, ts::GetIntVarLE<uint64_t>(_bytes + 0x88, 7));
    TSUNIT_EQUAL(0x908F8E8D8C8B8A89, ts::GetIntVarLE<uint64_t>(_bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(GetIntFixLE)
{
    TSUNIT_EQUAL(0x07, (ts::GetIntFixLE<1, uint8_t>(_bytes + 0x07)));
    TSUNIT_EQUAL(0x2423, (ts::GetIntFixLE<2, uint16_t>(_bytes + 0x23)));
    TSUNIT_EQUAL(0x121110, (ts::GetIntFixLE<3, uint32_t>(_bytes + 0x10)));
    TSUNIT_EQUAL(0xD0CFCE, (ts::GetIntFixLE<3, uint32_t>(_bytes + 0xCE)));
    TSUNIT_EQUAL(0x4A494847, (ts::GetIntFixLE<4, uint32_t>(_bytes + 0x47)));
    TSUNIT_EQUAL(0x0000008D8C8B8A89, (ts::GetIntFixLE<5, uint64_t>(_bytes + 0x89)));
    TSUNIT_EQUAL(0x00008E8D8C8B8A89, (ts::GetIntFixLE<6, uint64_t>(_bytes + 0x89)));
    TSUNIT_EQUAL(0x008E8D8C8B8A8988, (ts::GetIntFixLE<7, uint64_t>(_bytes + 0x88)));
    TSUNIT_EQUAL(0x908F8E8D8C8B8A89, (ts::GetIntFixLE<8, uint64_t>(_bytes + 0x89)));
}

TSUNIT_DEFINE_TEST(PutIntVarBE)
{
    uint8_t out[16];
    ts::PutIntVarBE(out, 1, 0x78);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x78, 1));
    ts::PutIntVarBE(out, 2, 0x898A);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
    ts::PutIntVarBE(out, 3, 0x898A8B);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
    ts::PutIntVarBE(out, 4, 0x56575859);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
    ts::PutIntVarBE(out, 5, 0x5556575859);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x55, 5));
    ts::PutIntVarBE(out, 6, 0x0000898A8B8C8D8E);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
    ts::PutIntVarBE(out, 7, 0x0088898A8B8C8D8E);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x88, 7));
    ts::PutIntVarBE(out, 8, 0x898A8B8C8D8E8F90);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(PutIntFixBE)
{
    uint8_t out[16];
    ts::PutIntFixBE<1>(out, 0x78);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x78, 1));
    ts::PutIntFixBE<2>(out, 0x898A);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
    ts::PutIntFixBE<3>(out, 0x898A8B);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
    ts::PutIntFixBE<4>(out, 0x56575859);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
    ts::PutIntFixBE<5>(out, 0x5556575859);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x55, 5));
    ts::PutIntFixBE<6>(out, 0x0000898A8B8C8D8E);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
    ts::PutIntFixBE<7>(out, 0x0088898A8B8C8D8E);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x88, 7));
    ts::PutIntFixBE<8>(out, 0x898A8B8C8D8E8F90);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(PutIntVarLE)
{
    uint8_t out[16];
    ts::PutIntVarLE(out, 1, 0x78);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x78, 1));
    ts::PutIntVarLE(out, 2, 0x8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
    ts::PutIntVarLE(out, 3, 0x8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
    ts::PutIntVarLE(out, 4, 0x59585756);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
    ts::PutIntVarLE(out, 5, 0x5958575655);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x55, 5));
    ts::PutIntVarLE(out, 6, 0x00008E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
    ts::PutIntVarLE(out, 7, 0x008E8D8C8B8A8988);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x88, 7));
    ts::PutIntVarLE(out, 8, 0x908F8E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

TSUNIT_DEFINE_TEST(PutIntFixLE)
{
    uint8_t out[16];
    ts::PutIntFixLE<1>(out, 0x78);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x78, 1));
    ts::PutIntFixLE<2>(out, 0x8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 2));
    ts::PutIntFixLE<3>(out, 0x8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 3));
    ts::PutIntFixLE<4>(out, 0x59585756);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x56, 4));
    ts::PutIntFixLE<5>(out, 0x5958575655);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x55, 5));
    ts::PutIntFixLE<6>(out, 0x00008E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 6));
    ts::PutIntFixLE<7>(out, 0x008E8D8C8B8A8988);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x88, 7));
    ts::PutIntFixLE<8>(out, 0x908F8E8D8C8B8A89);
    TSUNIT_EQUAL(0, ts::MemCompare(out, _bytes + 0x89, 8));
}

namespace {
    const uint8_t data1[] = {
        0x17, 0x2D, 0x29, 0x45, 0xD3, 0x4B, 0xE8, 0x6B, 0xBD, 0x38, 0x2A, 0x7C, 0xDB, 0x88, 0x53, 0x67,
        0x5B, 0x34, 0xCC, 0x2A, 0xB2, 0x00, 0x00, 0x01, 0xC0, 0xA6, 0x29, 0xFB, 0xD5, 0x83, 0x8A, 0x26,
        0xF1, 0x16, 0x55, 0x95, 0xCE, 0x38, 0x88, 0xBF, 0x0B, 0xCD, 0xD3, 0x03, 0x0B, 0xDC, 0x66, 0xC8,
        0xDE, 0x1B, 0x2A, 0x3B, 0x6E, 0x4A, 0xB7, 0xF3, 0x07, 0x89, 0x33, 0x54, 0x9E, 0xEE, 0x67, 0x27,
    };
    const uint8_t data2[] = {
        0x00, 0x00, 0x07, 0x34, 0x7A, 0xCD, 0x79, 0xEF, 0x7A, 0xF2, 0xA8, 0x7B, 0x11, 0xBF, 0xDD, 0x7B,
        0x8A, 0x1E, 0xB2, 0xD0, 0xA3, 0xE7, 0x14, 0xE4, 0x03, 0x8D, 0x22, 0xD4, 0x32, 0x81, 0xAF, 0x83,
        0x45, 0xD9, 0x6F, 0x2D, 0x7A, 0x2E, 0x2A, 0x3F, 0x58, 0xB6, 0x98, 0x86, 0x75, 0xF2, 0xBE, 0xCA,
        0xA1, 0x20, 0x6E, 0x36, 0x9B, 0x42, 0x40, 0x3B, 0x04, 0x04, 0xC5, 0x36, 0xF6, 0x00, 0x00, 0x0C,
    };
}

TSUNIT_DEFINE_TEST(LocatePattern)
{
    TSUNIT_ASSERT(ts::LocatePattern(data1, sizeof(data1), data2 + 7, 6) == nullptr);
    TSUNIT_ASSERT(ts::LocatePattern(data1, sizeof(data1), data1 + 7, 6) == data1 + 7);
}

TSUNIT_DEFINE_TEST(LocateZeroZero)
{
    TSUNIT_ASSERT(ts::LocateZeroZero(data1, sizeof(data1), 7) == nullptr);
    TSUNIT_ASSERT(ts::LocateZeroZero(data1, sizeof(data1), 1) == data1 + 21);
    TSUNIT_ASSERT(ts::LocateZeroZero(data2, sizeof(data2), 7) == data2);
    TSUNIT_ASSERT(ts::LocateZeroZero(data2, sizeof(data2), 12) == data2 + 61);
    TSUNIT_ASSERT(ts::LocateZeroZero(data2, sizeof(data2) - 1, 12) == nullptr);
}

TSUNIT_DEFINE_TEST(Xor)
{
    static const uint8_t src1[] = {
        0x8F, 0xE4, 0x48, 0xE8, 0xA1, 0xFA, 0x28, 0xBE, 0xCF, 0x25, 0x62, 0x09, 0x89, 0x2D, 0x56, 0x88,
        0xAA, 0x3D, 0xB2, 0x98, 0x10, 0x58, 0x9D, 0x55, 0x4E, 0x63, 0xC7, 0xD5, 0x8E, 0x9A, 0xC3, 0x9F,
        0x66, 0x14, 0x9E, 0x03, 0xCB, 0x6F, 0xDC, 0x64, 0x27, 0xEF, 0x24, 0xBA, 0xCB, 0xDC, 0x68, 0x45,
        0xD9, 0x5F, 0xAA, 0x19, 0xFA, 0xA1, 0x90, 0x35, 0xBC, 0x73, 0x3D, 0x37, 0xE2, 0xB8, 0x8A, 0x86,
        0x0E, 0xC3, 0xF2, 0xBC, 0xF4, 0xAF, 0xF9, 0x38, 0x2E, 0xF8, 0xFC, 0x39, 0x61, 0x8C, 0x2C, 0x3F,
        0x4E, 0x9E, 0x8F, 0x61, 0x61, 0xEC, 0x7C, 0x96, 0x42, 0x12, 0x8B, 0x70, 0x0D, 0xDA, 0x91, 0x2D,
        0x6F, 0x9A, 0x3C, 0x33, 0xA4, 0x60, 0xF7, 0x53, 0x00, 0xAE, 0x65, 0xE8, 0x50, 0x84, 0x14, 0x9F,
        0x26, 0x0A, 0x7A, 0x11, 0xEF, 0x4B, 0x50, 0x5B, 0x57, 0xE0, 0x2E, 0xEA, 0xBD, 0x8E, 0xF5, 0xF4,
        0x6D, 0x02, 0x2D, 0x6E, 0xFB, 0xAD, 0xAE, 0x3F, 0xBB, 0xD1, 0xA3, 0xEF, 0xFF, 0xC1, 0x0F, 0xAD,
        0xD1, 0x79, 0x29, 0x29, 0x26, 0xCC, 0x0F, 0xAD, 0xB4, 0xF1, 0xA1, 0x82, 0x21, 0xA8, 0xFB, 0x13,
        0x59, 0x51, 0xB6, 0x9C, 0x3B, 0x74, 0x27, 0xD2, 0xD9, 0xDF, 0xD3, 0x37, 0x8E, 0xA0, 0x48, 0x48,
        0xDE, 0xAB, 0x05, 0x75, 0x26, 0x20, 0x11, 0xD3, 0x85, 0x40, 0xB2, 0x49, 0x34, 0x71, 0x50, 0xDF,
        0x6E, 0x0C, 0x1D, 0x7D, 0x31, 0x33, 0xFC, 0x32, 0xE6, 0x6A, 0xB9, 0x95, 0x93, 0x85, 0x27, 0xDF,
        0x87, 0x79, 0x46, 0x6A, 0x67, 0xE1, 0x65, 0x5D, 0x4D, 0x81, 0xFC, 0xFC, 0x07, 0xF9, 0x53, 0xD9,
        0xB7, 0x6C, 0x8E, 0x82, 0xF4, 0x26, 0x56, 0xDA, 0x7A, 0x71, 0xAA, 0xC3, 0xBC,
    };
    static const uint8_t src2[] = {
        0x8D, 0xE8, 0x45, 0x19, 0x8D, 0xE3, 0x15, 0xD8, 0x99, 0x36, 0x2C, 0xDE, 0x15, 0x17, 0x6B, 0x07,
        0x26, 0xFE, 0x6D, 0x88, 0x77, 0xF1, 0x01, 0x5F, 0xCE, 0x74, 0xE9, 0xF1, 0xA5, 0x34, 0xA6, 0xB8,
        0xF7, 0xF7, 0xC2, 0xDD, 0x1B, 0x4E, 0x6B, 0xF2, 0xA3, 0x0E, 0x85, 0x73, 0x9B, 0xE3, 0x62, 0x5B,
        0x65, 0xAC, 0x47, 0x9D, 0xE8, 0x36, 0x16, 0x78, 0x5E, 0x32, 0x1E, 0x75, 0xDE, 0xDB, 0x16, 0x38,
        0xF6, 0x98, 0xB4, 0x72, 0x28, 0x3E, 0x55, 0x24, 0x5D, 0x84, 0x50, 0x10, 0xAE, 0xFA, 0x2C, 0xF8,
        0xAF, 0x8D, 0x07, 0x3E, 0x47, 0xB6, 0x7F, 0x5E, 0x1C, 0x77, 0x17, 0x50, 0x10, 0x9D, 0x67, 0xEC,
        0xE7, 0xBD, 0x2A, 0xFC, 0x1B, 0x6E, 0x4B, 0xA0, 0xF8, 0xEA, 0xF4, 0x72, 0xCC, 0x6A, 0x0E, 0x6E,
        0x8E, 0xD0, 0x8E, 0x00, 0xEE, 0x19, 0x7F, 0x3C, 0xDE, 0xFF, 0x00, 0xF8, 0xA7, 0x9E, 0x4C, 0xCC,
        0x67, 0x36, 0xDE, 0x81, 0x27, 0x90, 0x5C, 0x75, 0x80, 0x21, 0x58, 0x29, 0x34, 0x87, 0x6B, 0x03,
        0xCF, 0x2C, 0xFF, 0x4B, 0x51, 0x0E, 0x4A, 0xA4, 0x0C, 0x46, 0x29, 0x95, 0x0C, 0xAF, 0xA2, 0x0C,
        0xE1, 0x77, 0x86, 0x33, 0x66, 0x15, 0xBC, 0x33, 0xBA, 0x8F, 0x15, 0xF4, 0x68, 0x08, 0x7F, 0x94,
        0x8E, 0x54, 0xB5, 0x27, 0x65, 0xA1, 0x81, 0x6C, 0xC5, 0x2A, 0xC1, 0xB2, 0x7D, 0x9F, 0x9A, 0x1C,
        0x74, 0xB2, 0x33, 0xC4, 0xCB, 0xE4, 0xCE, 0x4E, 0xDF, 0xEC, 0x56, 0x5A, 0x9D, 0xBB, 0x53, 0x10,
        0x97, 0x03, 0x56, 0x36, 0x03, 0x67, 0x3E, 0xC0, 0x03, 0xC0, 0x60, 0xF8, 0x0F, 0xF3, 0xCA, 0x79,
        0xCC, 0xE5, 0x24, 0x5B, 0x74, 0x5D, 0x1D, 0x67, 0x1C, 0x4D, 0xDC, 0xF9, 0x4D,
    };
    static const uint8_t rxor[] = {
        0x02, 0x0C, 0x0D, 0xF1, 0x2C, 0x19, 0x3D, 0x66, 0x56, 0x13, 0x4E, 0xD7, 0x9C, 0x3A, 0x3D, 0x8F,
        0x8C, 0xC3, 0xDF, 0x10, 0x67, 0xA9, 0x9C, 0x0A, 0x80, 0x17, 0x2E, 0x24, 0x2B, 0xAE, 0x65, 0x27,
        0x91, 0xE3, 0x5C, 0xDE, 0xD0, 0x21, 0xB7, 0x96, 0x84, 0xE1, 0xA1, 0xC9, 0x50, 0x3F, 0x0A, 0x1E,
        0xBC, 0xF3, 0xED, 0x84, 0x12, 0x97, 0x86, 0x4D, 0xE2, 0x41, 0x23, 0x42, 0x3C, 0x63, 0x9C, 0xBE,
        0xF8, 0x5B, 0x46, 0xCE, 0xDC, 0x91, 0xAC, 0x1C, 0x73, 0x7C, 0xAC, 0x29, 0xCF, 0x76, 0x00, 0xC7,
        0xE1, 0x13, 0x88, 0x5F, 0x26, 0x5A, 0x03, 0xC8, 0x5E, 0x65, 0x9C, 0x20, 0x1D, 0x47, 0xF6, 0xC1,
        0x88, 0x27, 0x16, 0xCF, 0xBF, 0x0E, 0xBC, 0xF3, 0xF8, 0x44, 0x91, 0x9A, 0x9C, 0xEE, 0x1A, 0xF1,
        0xA8, 0xDA, 0xF4, 0x11, 0x01, 0x52, 0x2F, 0x67, 0x89, 0x1F, 0x2E, 0x12, 0x1A, 0x10, 0xB9, 0x38,
        0x0A, 0x34, 0xF3, 0xEF, 0xDC, 0x3D, 0xF2, 0x4A, 0x3B, 0xF0, 0xFB, 0xC6, 0xCB, 0x46, 0x64, 0xAE,
        0x1E, 0x55, 0xD6, 0x62, 0x77, 0xC2, 0x45, 0x09, 0xB8, 0xB7, 0x88, 0x17, 0x2D, 0x07, 0x59, 0x1F,
        0xB8, 0x26, 0x30, 0xAF, 0x5D, 0x61, 0x9B, 0xE1, 0x63, 0x50, 0xC6, 0xC3, 0xE6, 0xA8, 0x37, 0xDC,
        0x50, 0xFF, 0xB0, 0x52, 0x43, 0x81, 0x90, 0xBF, 0x40, 0x6A, 0x73, 0xFB, 0x49, 0xEE, 0xCA, 0xC3,
        0x1A, 0xBE, 0x2E, 0xB9, 0xFA, 0xD7, 0x32, 0x7C, 0x39, 0x86, 0xEF, 0xCF, 0x0E, 0x3E, 0x74, 0xCF,
        0x10, 0x7A, 0x10, 0x5C, 0x64, 0x86, 0x5B, 0x9D, 0x4E, 0x41, 0x9C, 0x04, 0x08, 0x0A, 0x99, 0xA0,
        0x7B, 0x89, 0xAA, 0xD9, 0x80, 0x7B, 0x4B, 0xBD, 0x66, 0x3C, 0x76, 0x3A, 0xF1,
    };
    uint8_t res[sizeof(rxor)];

    ts::MemXor(res, src1 + 7, src2 + 7, 1);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 7, 1));

    ts::MemXor(res, src1 + 11, src2 + 11, 2);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 11, 2));

    ts::MemXor(res, src1 + 15, src2 + 15, 3);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 15, 3));

    ts::MemXor(res, src1 + 21, src2 + 21, 4);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 21, 4));

    ts::MemXor(res, src1 + 47, src2 + 47, 5);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 47, 5));

    ts::MemXor(res, src1 + 54, src2 + 54, 6);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 54, 6));

    ts::MemXor(res, src1 + 61, src2 + 61, 7);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 61, 7));

    ts::MemXor(res, src1 + 81, src2 + 81, 8);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 81, 8));

    ts::MemXor(res, src1 + 93, src2 + 93, 9);
    TSUNIT_EQUAL(0, ::memcmp(res, rxor + 93, 9));

    ts::MemXor(res, src1, src2, sizeof(src1));
    TSUNIT_EQUAL(0, ::memcmp(res, rxor, sizeof(rxor)));
}
