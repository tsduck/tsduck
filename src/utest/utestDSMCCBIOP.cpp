//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Unit test for BIOP message structures.
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsDuckContext.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class BIOPTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(MessageHeaderValid);
    TSUNIT_DECLARE_TEST(MessageHeaderInvalidMagic);
    TSUNIT_DECLARE_TEST(MessageHeaderInvalidVersion);
    TSUNIT_DECLARE_TEST(MessageHeaderInvalidByteOrder);
    TSUNIT_DECLARE_TEST(MessageHeaderBigEndian);
    TSUNIT_DECLARE_TEST(MessageHeaderSerializeDeserialize);
    TSUNIT_DECLARE_TEST(MessageHeaderTruncated);
    TSUNIT_DECLARE_TEST(MessageHeaderClear);
};

TSUNIT_REGISTER(BIOPTest);


//----------------------------------------------------------------------------
// Test cases
//----------------------------------------------------------------------------

// Test valid BIOP message header
TSUNIT_DEFINE_TEST(MessageHeaderValid)
{
    ts::BIOPMessageHeader header;

    // Check default values
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MAGIC, header.magic);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MAJOR, header.version_major);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MINOR, header.version_minor);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN, header.byte_order);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MESSAGE_TYPE_STANDARD, header.message_type);

    // Check validity
    TSUNIT_ASSERT(header.isValid());
}

// Test invalid magic number
TSUNIT_DEFINE_TEST(MessageHeaderInvalidMagic)
{
    ts::BIOPMessageHeader header;
    header.magic = 0x12345678;

    TSUNIT_ASSERT(!header.isValid());
}

// Test invalid version
TSUNIT_DEFINE_TEST(MessageHeaderInvalidVersion)
{
    ts::BIOPMessageHeader header;
    header.version_major = 0x02;

    TSUNIT_ASSERT(!header.isValid());
}

// Test invalid byte order (only 0x00 is valid per specification)
TSUNIT_DEFINE_TEST(MessageHeaderInvalidByteOrder)
{
    ts::BIOPMessageHeader header;

    // Test that 0x01 (little-endian) is now invalid
    header.byte_order = 0x01;
    TSUNIT_ASSERT(!header.isValid());

    // Test other invalid values
    header.byte_order = 0xFF;
    TSUNIT_ASSERT(!header.isValid());
}

// Test big-endian byte order
TSUNIT_DEFINE_TEST(MessageHeaderBigEndian)
{
    ts::DuckContext duck;

    // Create a buffer with a valid BIOP header (big-endian)
    uint8_t data[] = {
        0x42,
        0x49,
        0x4F,
        0x50,  // magic "BIOP"
        0x01,  // version_major
        0x00,  // version_minor
        0x00,  // byte_order (big-endian)
        0x00,  // message_type
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);  // read-only buffer for reading
    ts::BIOPMessageHeader header;

    TSUNIT_ASSERT(header.deserialize(buf));
    TSUNIT_ASSERT(header.isValid());
    TSUNIT_EQUAL(0x42494F50u, header.magic);
    TSUNIT_EQUAL(0x01, header.version_major);
    TSUNIT_EQUAL(0x00, header.version_minor);
    TSUNIT_EQUAL(0x00, header.byte_order);
    TSUNIT_EQUAL(0x00, header.message_type);
}

// Test serialize/deserialize round-trip with big-endian
TSUNIT_DEFINE_TEST(MessageHeaderSerializeDeserialize)
{
    ts::DuckContext duck;

    // Create a header with specific values
    ts::BIOPMessageHeader header1;
    header1.magic = ts::BIOPMessageHeader::BIOP_MAGIC;
    header1.version_major = 0x01;
    header1.version_minor = 0x00;
    header1.byte_order = ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN;
    header1.message_type = 0x00;

    // Serialize
    uint8_t buffer[ts::BIOPMessageHeader::HEADER_SIZE];
    ts::PSIBuffer buf1(duck, buffer, sizeof(buffer), false);  // writable buffer for writing
    TSUNIT_ASSERT(header1.serialize(buf1));

    // Deserialize
    ts::PSIBuffer buf2(duck, buffer, sizeof(buffer), true);  // read-only buffer for reading
    ts::BIOPMessageHeader header2;
    TSUNIT_ASSERT(header2.deserialize(buf2));

    // Compare
    TSUNIT_EQUAL(header1.magic, header2.magic);
    TSUNIT_EQUAL(header1.version_major, header2.version_major);
    TSUNIT_EQUAL(header1.version_minor, header2.version_minor);
    TSUNIT_EQUAL(header1.byte_order, header2.byte_order);
    TSUNIT_EQUAL(header1.message_type, header2.message_type);
    TSUNIT_ASSERT(header2.isValid());
}

// Test truncated data
TSUNIT_DEFINE_TEST(MessageHeaderTruncated)
{
    ts::DuckContext duck;

    // Create truncated data (only 8 bytes instead of 12)
    uint8_t data[] = {
        0x42,
        0x49,
        0x4F,
        0x50,  // magic "BIOP"
        0x01,  // version_major
        0x00,  // version_minor
        0x00,  // byte_order (missing message_type)
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);  // read-only buffer for reading
    ts::BIOPMessageHeader header;

    // Should fail due to insufficient data
    TSUNIT_ASSERT(!header.deserialize(buf));
    TSUNIT_ASSERT(buf.error());
}

// Test clear method
TSUNIT_DEFINE_TEST(MessageHeaderClear)
{
    ts::BIOPMessageHeader header;

    // Modify values
    header.magic = 0x12345678;
    header.version_major = 0xFF;
    header.version_minor = 0xFF;
    header.byte_order = 0xFF;
    header.message_type = 0xFF;

    // Clear and check defaults are restored
    header.clear();

    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MAGIC, header.magic);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MAJOR, header.version_major);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MINOR, header.version_minor);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN, header.byte_order);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MESSAGE_TYPE_STANDARD, header.message_type);
    TSUNIT_ASSERT(header.isValid());
}
