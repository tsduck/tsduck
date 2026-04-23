//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Unit test for BIOP message structures.
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsxmlDocument.h"
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
    TSUNIT_DECLARE_TEST(FileMessageParse);
    TSUNIT_DECLARE_TEST(UnsupportedKindReturnsNull);
    TSUNIT_DECLARE_TEST(DirectoryMessageParse);
    TSUNIT_DECLARE_TEST(ServiceGatewayMessageParse);
    TSUNIT_DECLARE_TEST(FileMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(DirectoryMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(ServiceGatewayMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(FromXMLDispatch);
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

// Parse a BIOP File message (DSM::File, deja.ttf). Header bytes from a real
// broadcast; content truncated to the first 20 bytes of the TrueType file,
// with message_size/messageBody_length/content_length adjusted accordingly.
TSUNIT_DEFINE_TEST(FileMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        // BIOP header
        0x42, 0x49, 0x4F, 0x50,  // magic "BIOP"
        0x01, 0x00,              // version 1.0
        0x00, 0x00,              // byte_order=BE, message_type=standard
        // message_size = 49 (adjusted for truncated content)
        0x00, 0x00, 0x00, 0x31,
        // objectKey_length = 1, objectKey = 0x02
        0x01, 0x02,
        // objectKind_length = 4, objectKind = "fil\0"
        0x00, 0x00, 0x00, 0x04,
        0x66, 0x69, 0x6C, 0x00,
        // objectInfo_length = 8, dsmFileContentSize = 756072 (0x0B8968)
        0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x89, 0x68,
        // serviceContextList_count = 0
        0x00,
        // messageBody_length = 24 (adjusted)
        0x00, 0x00, 0x00, 0x18,
        // content_length = 20 (first 20 bytes of TrueType data)
        0x00, 0x00, 0x00, 0x14,
        // content: TTF header (version 1.0, numTables=20, then "FFTM" table record)
        0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00,
        0x00, 0x04, 0x00, 0x40, 0x46, 0x46, 0x54, 0x4D,
        0x6F, 0x39, 0x9E, 0xB8,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);

    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_ASSERT(msg->header.isValid());
    TSUNIT_EQUAL("fil", msg->kindTag());
    TSUNIT_EQUAL(1u, msg->object_key.size());
    TSUNIT_EQUAL(0x02, msg->object_key[0]);
    TSUNIT_EQUAL(8u, msg->object_info.size());
    TSUNIT_EQUAL(0x0Bu, msg->object_info[5]);
    TSUNIT_EQUAL(0x89u, msg->object_info[6]);
    TSUNIT_EQUAL(0x68u, msg->object_info[7]);
    TSUNIT_EQUAL(0u, msg->service_contexts.size());

    auto* file = dynamic_cast<ts::BIOPFileMessage*>(msg.get());
    TSUNIT_ASSERT(file != nullptr);
    TSUNIT_EQUAL(20u, file->content.size());
    TSUNIT_EQUAL(0x00u, file->content[0]);
    TSUNIT_EQUAL(0x01u, file->content[1]);
    TSUNIT_EQUAL(0xB8u, file->content[19]);
    TSUNIT_ASSERT(!buf.error());
}

// Parse returns nullptr (not an error) for a kind we don't support yet.
TSUNIT_DEFINE_TEST(UnsupportedKindReturnsNull)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x18,  // message_size = 24
        0x04, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x04, 0x73, 0x74, 0x72, 0x00,  // "str\0" (stream)
        0x00, 0x00,              // objectInfo_length = 0
        0x00,                    // serviceContextList_count = 0
        0x00, 0x00, 0x00, 0x04,  // messageBody_length = 4
        0xDE, 0xAD, 0xBE, 0xEF,  // body (opaque)
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);
    TSUNIT_ASSERT(msg == nullptr);
    TSUNIT_ASSERT(!buf.error());
    // Read pointer must have advanced past the full message.
    TSUNIT_EQUAL(sizeof(data), buf.currentReadByteOffset());
}

// Parse a Directory message with one ncontext binding ("root" -> dir, empty IOR).
TSUNIT_DEFINE_TEST(DirectoryMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        // BIOP header
        0x42, 0x49, 0x4F, 0x50, 0x01, 0x00, 0x00, 0x00,
        // message_size = 45
        0x00, 0x00, 0x00, 0x2D,
        // objectKey_length = 4, objectKey = 00 00 00 02
        0x04, 0x00, 0x00, 0x00, 0x02,
        // objectKind_length = 4, objectKind = "dir\0"
        0x00, 0x00, 0x00, 0x04, 0x64, 0x69, 0x72, 0x00,
        // objectInfo_length = 0
        0x00, 0x00,
        // serviceContextList_count = 0
        0x00,
        // messageBody_length = 25
        0x00, 0x00, 0x00, 0x19,
        // --- body ---
        // bindings_count = 1
        0x00, 0x01,
        // Binding:
        //   nameComponents_count = 1
        0x01,
        //   id_length = 5, id = "root\0"
        0x05, 0x72, 0x6F, 0x6F, 0x74, 0x00,
        //   kind_length = 4, kind = "dir\0"
        0x04, 0x64, 0x69, 0x72, 0x00,
        //   binding_type = 0x02 (ncontext)
        0x02,
        //   IOR: type_id_length = 0, tagged_profiles_count = 0
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        //   objectInfo_length = 0
        0x00, 0x00,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);

    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL("dir", msg->kindTag());

    auto* dir = dynamic_cast<ts::BIOPDirectoryMessage*>(msg.get());
    TSUNIT_ASSERT(dir != nullptr);
    TSUNIT_EQUAL(1u, dir->bindings.size());

    const auto& b = dir->bindings[0];
    TSUNIT_EQUAL(1u, b.name.size());
    TSUNIT_EQUAL(u"root", b.name[0].idString());
    TSUNIT_EQUAL("dir", b.name[0].kindTag());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NCONTEXT, b.binding_type);
    TSUNIT_EQUAL(0u, b.ior.tagged_profiles.size());
    TSUNIT_EQUAL(0u, b.object_info.size());
    TSUNIT_ASSERT(!buf.error());
}

// Parse a ServiceGateway message with 3 nobject bindings (deja.ttf, index.html,
// rj45.gif). Binary data from a real broadcast, DSM::ServiceGateway module 0x0001.
TSUNIT_DEFINE_TEST(ServiceGatewayMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        // BIOP header
        0x42, 0x49, 0x4F, 0x50,  // magic "BIOP"
        0x01, 0x00,              // version 1.0
        0x00, 0x00,              // byte_order=BE, message_type=standard
        // message_size = 282
        0x00, 0x00, 0x01, 0x1A,
        // objectKey_length = 1, objectKey = 0x01
        0x01, 0x01,
        // objectKind_length = 4, objectKind = "srg\0"
        0x00, 0x00, 0x00, 0x04,
        0x73, 0x72, 0x67, 0x00,
        // objectInfo_length = 0
        0x00, 0x00,
        // serviceContextList_count = 0
        0x00,
        // messageBody_length = 265
        0x00, 0x00, 0x01, 0x09,
        // --- body: bindings_count = 3 ---
        0x00, 0x03,
        // ---- Binding 1: deja.ttf ----
        0x01,                                            // nameComponents_count = 1
        0x09,                                            // id_length = 9
        0x64, 0x65, 0x6A, 0x61, 0x2E, 0x74, 0x74, 0x66, 0x00,  // "deja.ttf\0"
        0x04,                                            // kind_length = 4
        0x66, 0x69, 0x6C, 0x00,                          // "fil\0"
        0x01,                                            // bindingType = nobject
        // IOR: type_id_length=4, type_id="fil\0", 1 tagged profile
        0x00, 0x00, 0x00, 0x04,
        0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        // TaggedProfile: TAG_BIOP, profile_data_length=40
        0x49, 0x53, 0x4F, 0x06,
        0x00, 0x00, 0x00, 0x28,
        0x00,                    // byte_order
        0x02,                    // liteComponents_count = 2
        // ObjectLocation: carouselId=10, moduleId=2, objectKey=0x02
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x02, 0x01, 0x00, 0x01, 0x02,
        // ConnBinder: tap(id=0, use=0x16, assoc=10, sel_type=1, txId=0x80000002, timeout=60000000)
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        // binding objectInfo (8 bytes)
        0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // ---- Binding 2: index.html ----
        0x01,                                            // nameComponents_count = 1
        0x0B,                                            // id_length = 11
        0x69, 0x6E, 0x64, 0x65, 0x78, 0x2E, 0x68, 0x74, 0x6D, 0x6C, 0x00,  // "index.html\0"
        0x04,                                            // kind_length = 4
        0x66, 0x69, 0x6C, 0x00,                          // "fil\0"
        0x01,                                            // bindingType = nobject
        // IOR: type_id="fil\0", TAG_BIOP profile
        0x00, 0x00, 0x00, 0x04,
        0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x49, 0x53, 0x4F, 0x06,
        0x00, 0x00, 0x00, 0x28,
        0x00, 0x02,
        // ObjectLocation: carouselId=10, moduleId=3, objectKey=0x03
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x03, 0x01, 0x00, 0x01, 0x03,
        // ConnBinder
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        // binding objectInfo (8 bytes)
        0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // ---- Binding 3: rj45.gif ----
        0x01,                                            // nameComponents_count = 1
        0x09,                                            // id_length = 9
        0x72, 0x6A, 0x34, 0x35, 0x2E, 0x67, 0x69, 0x66, 0x00,  // "rj45.gif\0"
        0x04,                                            // kind_length = 4
        0x66, 0x69, 0x6C, 0x00,                          // "fil\0"
        0x01,                                            // bindingType = nobject
        // IOR: type_id="fil\0", TAG_BIOP profile
        0x00, 0x00, 0x00, 0x04,
        0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x49, 0x53, 0x4F, 0x06,
        0x00, 0x00, 0x00, 0x28,
        0x00, 0x02,
        // ObjectLocation: carouselId=10, moduleId=3, objectKey=0x04
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x03, 0x01, 0x00, 0x01, 0x04,
        // ConnBinder
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        // binding objectInfo (8 bytes)
        0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);

    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_ASSERT(msg->header.isValid());
    TSUNIT_EQUAL("srg", msg->kindTag());
    TSUNIT_EQUAL(1u, msg->object_key.size());
    TSUNIT_EQUAL(0x01, msg->object_key[0]);
    TSUNIT_EQUAL(0u, msg->service_contexts.size());

    auto* sg = dynamic_cast<ts::BIOPServiceGatewayMessage*>(msg.get());
    TSUNIT_ASSERT(sg != nullptr);
    TSUNIT_EQUAL(3u, sg->bindings.size());

    // Binding 1: deja.ttf
    const auto& b0 = sg->bindings[0];
    TSUNIT_EQUAL(1u, b0.name.size());
    TSUNIT_EQUAL(u"deja.ttf", b0.name[0].idString());
    TSUNIT_EQUAL("fil", b0.name[0].kindTag());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NOBJECT, b0.binding_type);
    TSUNIT_EQUAL(1u, b0.ior.tagged_profiles.size());
    auto loc0 = b0.targetLocation();
    TSUNIT_ASSERT(loc0.has_value());
    TSUNIT_EQUAL(2u, loc0->first);
    TSUNIT_EQUAL(1u, loc0->second.size());
    TSUNIT_EQUAL(0x02, loc0->second[0]);

    // Binding 2: index.html
    const auto& b1 = sg->bindings[1];
    TSUNIT_EQUAL(u"index.html", b1.name[0].idString());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NOBJECT, b1.binding_type);
    auto loc1 = b1.targetLocation();
    TSUNIT_ASSERT(loc1.has_value());
    TSUNIT_EQUAL(3u, loc1->first);
    TSUNIT_EQUAL(0x03, loc1->second[0]);

    // Binding 3: rj45.gif
    const auto& b2 = sg->bindings[2];
    TSUNIT_EQUAL(u"rj45.gif", b2.name[0].idString());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NOBJECT, b2.binding_type);
    auto loc2 = b2.targetLocation();
    TSUNIT_ASSERT(loc2.has_value());
    TSUNIT_EQUAL(3u, loc2->first);
    TSUNIT_EQUAL(0x04, loc2->second[0]);

    TSUNIT_ASSERT(!buf.error());
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


// XML round-trip for BIOPFileMessage (DSM::File, deja.ttf structure).
TSUNIT_DEFINE_TEST(FileMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    ts::BIOPFileMessage orig;
    orig.object_key = {0x02};
    orig.object_kind = {0x66, 0x69, 0x6C, 0x00};  // "fil\0"
    orig.object_info = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x89, 0x68};
    orig.content = {
        0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00,
        0x00, 0x04, 0x00, 0x40, 0x46, 0x46, 0x54, 0x4D,
        0x6F, 0x39, 0x9E, 0xB8,
    };

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    orig.toXML(duck, root);

    const ts::xml::Element* xmsg = root->findFirstChild(u"BIOP_message", true);
    TSUNIT_ASSERT(xmsg != nullptr);

    auto restored = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(restored != nullptr);
    TSUNIT_EQUAL("fil", restored->kindTag());
    TSUNIT_ASSERT(orig.object_key == restored->object_key);
    TSUNIT_ASSERT(orig.object_info == restored->object_info);
    TSUNIT_ASSERT(restored->header.isValid());

    auto* file = dynamic_cast<ts::BIOPFileMessage*>(restored.get());
    TSUNIT_ASSERT(file != nullptr);
    TSUNIT_EQUAL(orig.content.size(), file->content.size());
    TSUNIT_ASSERT(orig.content == file->content);
}


// XML round-trip for BIOPDirectoryMessage.
TSUNIT_DEFINE_TEST(DirectoryMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    ts::BIOPDirectoryMessage orig;
    orig.object_key = {0x00, 0x00, 0x00, 0x02};
    orig.object_kind = {0x64, 0x69, 0x72, 0x00};  // "dir\0"

    ts::BIOPBinding b;
    ts::BIOPNameComponent nc;
    nc.id = {0x72, 0x6F, 0x6F, 0x74, 0x00};  // "root\0"
    nc.kind = {0x64, 0x69, 0x72, 0x00};       // "dir\0"
    b.name.push_back(std::move(nc));
    b.binding_type = ts::BIOPBinding::BINDING_TYPE_NCONTEXT;
    orig.bindings.push_back(std::move(b));

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    orig.toXML(duck, root);

    const ts::xml::Element* xmsg = root->findFirstChild(u"BIOP_message", true);
    TSUNIT_ASSERT(xmsg != nullptr);

    auto restored = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(restored != nullptr);
    TSUNIT_EQUAL("dir", restored->kindTag());

    auto* dir = dynamic_cast<ts::BIOPDirectoryMessage*>(restored.get());
    TSUNIT_ASSERT(dir != nullptr);
    TSUNIT_EQUAL(1u, dir->bindings.size());
    TSUNIT_EQUAL(u"root", dir->bindings[0].name[0].idString());
    TSUNIT_EQUAL("dir", dir->bindings[0].name[0].kindTag());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NCONTEXT, dir->bindings[0].binding_type);
}


// XML round-trip for BIOPServiceGatewayMessage (3 bindings with IOR).
TSUNIT_DEFINE_TEST(ServiceGatewayMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    // Helper: build a nobject binding with a TAG_BIOP IOR pointing to (moduleId, objectKey).
    auto makeBinding = [](const ts::ByteBlock& id, uint16_t module_id, uint8_t obj_key) {
        ts::BIOPBinding b;
        ts::BIOPNameComponent nc;
        nc.id = id;
        nc.kind = {0x66, 0x69, 0x6C, 0x00};  // "fil\0"
        b.name.push_back(std::move(nc));
        b.binding_type = ts::BIOPBinding::BINDING_TYPE_NOBJECT;
        b.ior.type_id = {0x66, 0x69, 0x6C, 0x00};
        ts::DSMCCTaggedProfile tp;
        tp.profile_id_tag = ts::DSMCC_TAG_BIOP;
        ts::DSMCCLiteComponent loc;
        loc.component_id_tag = ts::DSMCC_TAG_OBJECT_LOCATION;
        loc.carousel_id = 10;
        loc.module_id = module_id;
        loc.object_key_data = {obj_key};
        tp.lite_components.push_back(std::move(loc));
        b.ior.tagged_profiles.push_back(std::move(tp));
        b.object_info = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        return b;
    };

    ts::BIOPServiceGatewayMessage orig;
    orig.object_key = {0x01};
    orig.object_kind = {0x73, 0x72, 0x67, 0x00};  // "srg\0"
    orig.bindings.push_back(makeBinding({0x64, 0x65, 0x6A, 0x61, 0x2E, 0x74, 0x74, 0x66, 0x00}, 2, 0x02));  // deja.ttf
    orig.bindings.push_back(makeBinding({0x69, 0x6E, 0x64, 0x65, 0x78, 0x2E, 0x68, 0x74, 0x6D, 0x6C, 0x00}, 3, 0x03));  // index.html
    orig.bindings.push_back(makeBinding({0x72, 0x6A, 0x34, 0x35, 0x2E, 0x67, 0x69, 0x66, 0x00}, 3, 0x04));  // rj45.gif

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    orig.toXML(duck, root);

    const ts::xml::Element* xmsg = root->findFirstChild(u"BIOP_message", true);
    TSUNIT_ASSERT(xmsg != nullptr);

    auto restored = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(restored != nullptr);
    TSUNIT_EQUAL("srg", restored->kindTag());
    TSUNIT_ASSERT(orig.object_key == restored->object_key);

    auto* sg = dynamic_cast<ts::BIOPServiceGatewayMessage*>(restored.get());
    TSUNIT_ASSERT(sg != nullptr);
    TSUNIT_EQUAL(3u, sg->bindings.size());

    TSUNIT_EQUAL(u"deja.ttf", sg->bindings[0].name[0].idString());
    auto loc0 = sg->bindings[0].targetLocation();
    TSUNIT_ASSERT(loc0.has_value());
    TSUNIT_EQUAL(2u, loc0->first);
    TSUNIT_EQUAL(0x02, loc0->second[0]);

    TSUNIT_EQUAL(u"index.html", sg->bindings[1].name[0].idString());
    auto loc1 = sg->bindings[1].targetLocation();
    TSUNIT_ASSERT(loc1.has_value());
    TSUNIT_EQUAL(3u, loc1->first);
    TSUNIT_EQUAL(0x03, loc1->second[0]);

    TSUNIT_EQUAL(u"rj45.gif", sg->bindings[2].name[0].idString());
    auto loc2 = sg->bindings[2].targetLocation();
    TSUNIT_ASSERT(loc2.has_value());
    TSUNIT_EQUAL(3u, loc2->first);
    TSUNIT_EQUAL(0x04, loc2->second[0]);
}


// FromXML returns nullptr for unsupported object_kind.
TSUNIT_DEFINE_TEST(FromXMLDispatch)
{
    ts::DuckContext duck;

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    ts::xml::Element* xmsg = root->addElement(u"BIOP_message");
    xmsg->setAttribute(u"object_kind", u"str");

    auto msg = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(msg == nullptr);
}
