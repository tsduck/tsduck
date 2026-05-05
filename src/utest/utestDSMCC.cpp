//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Unit tests for DSM-CC: module assembler, carousel group tracking, BIOP
// message parsing and XML round-trips.
//
//----------------------------------------------------------------------------

#include "tsDSMCCBIOPMessage.h"
#include "tsDSMCCCarousel.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsDSMCCModuleAssembler.h"
#include "tsDSMCCNameDescriptor.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDID.h"
#include "tsDuckContext.h"
#include "tsunit.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


class DSMCCTest: public tsunit::Test
{
    // --- Assembler ---
    TSUNIT_DECLARE_TEST(Assembler_DescriptorsSurfacedFromDII);
    TSUNIT_DECLARE_TEST(Assembler_DiscoveryFiresOncePerKey);
    TSUNIT_DECLARE_TEST(Assembler_DiscoveryFiresOnceWhenDDBPrecedesDII);
    TSUNIT_DECLARE_TEST(Assembler_DiscoverySuppressedOnVersionBump);
    TSUNIT_DECLARE_TEST(Assembler_MultiGroupSameModuleIdDoesNotCollide);
    TSUNIT_DECLARE_TEST(Assembler_SingleGroupBehaviourPreserved);

    // --- Carousel ---
    TSUNIT_DECLARE_TEST(Carousel_DataCarouselDSIPopulatesGroups);
    TSUNIT_DECLARE_TEST(Carousel_GroupCompletionFiresExactlyOnce);
    TSUNIT_DECLARE_TEST(Carousel_MultiDIIPerGroupAccumulatesModules);
    TSUNIT_DECLARE_TEST(Carousel_ObjectCarouselSynthesizesGroupOnFirstDII);
    TSUNIT_DECLARE_TEST(Carousel_OverlappingModuleIdsAcrossGroupsAreIndependent);

    // --- BIOP ---
    TSUNIT_DECLARE_TEST(BIOP_DirectoryMessageParse);
    TSUNIT_DECLARE_TEST(BIOP_DirectoryMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(BIOP_FileMessageParse);
    TSUNIT_DECLARE_TEST(BIOP_FileMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(BIOP_FromXMLDispatch);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderBigEndian);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderClear);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderInvalidByteOrder);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderInvalidMagic);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderInvalidVersion);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderSerializeDeserialize);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderTruncated);
    TSUNIT_DECLARE_TEST(BIOP_MessageHeaderValid);
    TSUNIT_DECLARE_TEST(BIOP_ServiceGatewayMessageParse);
    TSUNIT_DECLARE_TEST(BIOP_ServiceGatewayMessageXMLRoundTrip);
    TSUNIT_DECLARE_TEST(BIOP_UnsupportedKindReturnsNull);
};

TSUNIT_REGISTER(DSMCCTest);


//----------------------------------------------------------------------------
// Shared helpers
//----------------------------------------------------------------------------

namespace {

    // Single-module DII (used by assembler tests).
    ts::DSMCCUserToNetworkMessage makeDII(uint32_t download_id, uint16_t module_id, uint16_t block_size, uint8_t module_version)
    {
        ts::DSMCCUserToNetworkMessage unm;
        unm.header.message_id = ts::DSMCC_MSGID_DII;
        auto& dii = unm.body.emplace<ts::DSMCCUserToNetworkMessage::DownloadInfoIndication>(&unm);
        dii.download_id = download_id;
        dii.block_size = block_size;
        auto& mod = dii.modules.newEntry();
        mod.module_id = module_id;
        mod.module_size = block_size;
        mod.module_version = module_version;
        return unm;
    }

    // Multi-module DII (used by carousel tests).
    ts::DSMCCUserToNetworkMessage makeDII(uint32_t download_id,
                                          const std::vector<uint16_t>& module_ids,
                                          uint16_t block_size = 16,
                                          uint8_t module_version = 0)
    {
        ts::DSMCCUserToNetworkMessage unm;
        unm.header.message_id = ts::DSMCC_MSGID_DII;
        auto& dii = unm.body.emplace<ts::DSMCCUserToNetworkMessage::DownloadInfoIndication>(&unm);
        dii.download_id = download_id;
        dii.block_size = block_size;
        for (uint16_t mid : module_ids) {
            auto& mod = dii.modules.newEntry();
            mod.module_id = mid;
            mod.module_size = block_size;
            mod.module_version = module_version;
        }
        return unm;
    }

    ts::DSMCCDownloadDataMessage makeDDB(uint32_t download_id, uint16_t module_id, uint8_t module_version, const ts::ByteBlock& payload)
    {
        ts::DSMCCDownloadDataMessage ddm;
        ddm.header.message_id = ts::DSMCC_MSGID_DDB;
        ddm.header.download_id = download_id;
        ddm.module_id = module_id;
        ddm.module_version = module_version;
        ddm.block_data = payload;
        return ddm;
    }

    ts::DSMCCUserToNetworkMessage makeDSIWithGroups(const std::vector<std::pair<uint32_t, uint32_t>>& groups)
    {
        ts::DSMCCUserToNetworkMessage unm;
        unm.header.message_id = ts::DSMCC_MSGID_DSI;
        auto& dsi = unm.body.emplace<ts::DSMCCUserToNetworkMessage::DownloadServerInitiate>();
        for (const auto& [gid, gsize] : groups) {
            auto& g = dsi.group_info.groups.emplace_back();
            g.group_id = gid;
            g.group_size = gsize;
        }
        return unm;
    }

}  // namespace


//----------------------------------------------------------------------------
// Assembler tests
//----------------------------------------------------------------------------

// A DII carrying a name_descriptor must produce a ModuleContext whose descs
// field is non-empty and from which the name round-trips correctly.
TSUNIT_DEFINE_TEST(Assembler_DescriptorsSurfacedFromDII)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    ts::DSMCCUserToNetworkMessage unm;
    unm.header.message_id = ts::DSMCC_MSGID_DII;
    auto& dii = unm.body.emplace<ts::DSMCCUserToNetworkMessage::DownloadInfoIndication>(&unm);
    dii.download_id = 0x1234;
    dii.block_size = 16;
    auto& mod = dii.modules.newEntry();
    mod.module_id = 0x0001;
    mod.module_size = 16;
    mod.module_version = 0;

    ts::DSMCCNameDescriptor name_desc;
    name_desc.name = u"hello";
    mod.descs.add(duck, name_desc);

    assembler.feedUserToNetwork(unm);

    const auto* ctx = assembler.module(0x1234, 0x0001);
    TSUNIT_ASSERT(ctx != nullptr);
    TSUNIT_ASSERT(ctx->descs.count() > 0);

    const size_t ni = ctx->descs.search(ts::DID_DSMCC_NAME);
    TSUNIT_ASSERT(ni < ctx->descs.count());
    ts::DSMCCNameDescriptor decoded(duck, ctx->descs[ni]);
    TSUNIT_ASSERT(decoded.isValid());
    TSUNIT_EQUAL(u"hello", decoded.name);
}


// Discovery callback fires exactly once per (download_id, module_id) pair on
// the first DII that announces it.
TSUNIT_DEFINE_TEST(Assembler_DiscoveryFiresOncePerKey)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    std::vector<std::pair<uint32_t, uint16_t>> discovered;
    assembler.setModuleDiscoveredHandler(
        [&](uint32_t download_id, uint16_t module_id) {
            discovered.emplace_back(download_id, module_id);
        });

    auto dii_1 = makeDII(0x000A, 0x0001, 16, 0);
    auto dii_2 = makeDII(0x000A, 0x0002, 16, 0);
    auto dii_3 = makeDII(0x000B, 0x0001, 16, 0);

    assembler.feedUserToNetwork(dii_1);
    assembler.feedUserToNetwork(dii_2);
    assembler.feedUserToNetwork(dii_3);
    assembler.feedUserToNetwork(dii_1);  // re-feed must not re-discover

    TSUNIT_EQUAL(3u, discovered.size());
    TSUNIT_EQUAL(0x000Au, discovered[0].first);
    TSUNIT_EQUAL(0x0001u, discovered[0].second);
    TSUNIT_EQUAL(0x000Au, discovered[1].first);
    TSUNIT_EQUAL(0x0002u, discovered[1].second);
    TSUNIT_EQUAL(0x000Bu, discovered[2].first);
    TSUNIT_EQUAL(0x0001u, discovered[2].second);
}


// DDB arriving before its DII goes into the orphan buffer. When the DII
// finally arrives, discovery must fire exactly once (from the DII path).
TSUNIT_DEFINE_TEST(Assembler_DiscoveryFiresOnceWhenDDBPrecedesDII)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    size_t discovery_count = 0;
    assembler.setModuleDiscoveredHandler(
        [&](uint32_t, uint16_t) { ++discovery_count; });

    const uint16_t block_size = 16;
    const ts::ByteBlock payload(block_size, 0xCC);

    auto ddb = makeDDB(0x000A, 0x0001, 0, payload);
    assembler.feedDownloadData(ddb);  // orphaned

    auto dii = makeDII(0x000A, 0x0001, block_size, 0);
    assembler.feedUserToNetwork(dii);

    TSUNIT_EQUAL(1u, discovery_count);
}


// A version bump on an already-known module must not re-fire discovery —
// the module has already been counted by the carousel-level group accounting.
TSUNIT_DEFINE_TEST(Assembler_DiscoverySuppressedOnVersionBump)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    size_t discovery_count = 0;
    assembler.setModuleDiscoveredHandler(
        [&](uint32_t, uint16_t) { ++discovery_count; });

    auto dii_v0 = makeDII(0x000A, 0x0001, 16, 0);
    auto dii_v1 = makeDII(0x000A, 0x0001, 16, 1);

    assembler.feedUserToNetwork(dii_v0);
    assembler.feedUserToNetwork(dii_v1);

    TSUNIT_EQUAL(1u, discovery_count);
}


// Two DIIs with the same module_id but different download_ids must produce
// two distinct completions without cross-contaminating payloads.
TSUNIT_DEFINE_TEST(Assembler_MultiGroupSameModuleIdDoesNotCollide)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    std::vector<std::pair<uint32_t, uint16_t>> completed;
    std::vector<ts::ByteBlock>                 payloads;
    assembler.setModuleCompletedHandler(
        [&](const ts::DSMCCModuleAssembler::ModuleContext& ctx) {
            completed.emplace_back(ctx.download_id, ctx.module_id);
            payloads.push_back(ctx.payload);
        });

    const uint16_t block_size = 16;
    const ts::ByteBlock payload_A(block_size, 0xAA);
    const ts::ByteBlock payload_B(block_size, 0xBB);

    auto dii_A = makeDII(0x0000000A, 0x0001, block_size, 0);
    auto dii_B = makeDII(0x0000000B, 0x0001, block_size, 0);
    assembler.feedUserToNetwork(dii_A);
    assembler.feedUserToNetwork(dii_B);

    auto ddb_A = makeDDB(0x0000000A, 0x0001, 0, payload_A);
    auto ddb_B = makeDDB(0x0000000B, 0x0001, 0, payload_B);
    assembler.feedDownloadData(ddb_A);
    assembler.feedDownloadData(ddb_B);

    TSUNIT_EQUAL(2u, completed.size());
    TSUNIT_EQUAL(0x0000000Au, completed[0].first);
    TSUNIT_EQUAL(0x0001u, completed[0].second);
    TSUNIT_EQUAL(0x0000000Bu, completed[1].first);
    TSUNIT_EQUAL(0x0001u, completed[1].second);
    TSUNIT_EQUAL(block_size, payloads[0].size());
    TSUNIT_EQUAL(block_size, payloads[1].size());
    TSUNIT_EQUAL(0xAAu, payloads[0][0]);
    TSUNIT_EQUAL(0xBBu, payloads[1][0]);
}


// Single-group stream: one download_id, three modules. Exercises the
// (download_id, module_id) map in its common-case shape.
TSUNIT_DEFINE_TEST(Assembler_SingleGroupBehaviourPreserved)
{
    ts::DuckContext duck;
    ts::DSMCCModuleAssembler assembler(duck);

    size_t count = 0;
    assembler.setModuleCompletedHandler(
        [&](const ts::DSMCCModuleAssembler::ModuleContext& ctx) {
            TSUNIT_EQUAL(0x1234u, ctx.download_id);
            ++count;
        });

    const uint32_t dlid = 0x1234;
    const uint16_t block_size = 8;
    const ts::ByteBlock pad(block_size, 0x00);

    for (uint16_t m = 1; m <= 3; ++m) {
        auto dii = makeDII(dlid, m, block_size, 0);
        assembler.feedUserToNetwork(dii);
        auto ddb = makeDDB(dlid, m, 0, pad);
        assembler.feedDownloadData(ddb);
    }

    TSUNIT_EQUAL(3u, count);
}


//----------------------------------------------------------------------------
// Carousel tests
//----------------------------------------------------------------------------

// Data carousel: a DSI's GroupInfoIndication populates _groups with
// announced_by_dsi=true, group_size, and any compatibility descriptors.
TSUNIT_DEFINE_TEST(Carousel_DataCarouselDSIPopulatesGroups)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);

    auto dsi = makeDSIWithGroups({{0x80000002, 0x58860}, {0x80000003, 0x12345}});
    carousel.feedUserToNetwork(dsi);

    const auto& groups = carousel.groups();
    TSUNIT_EQUAL(2u, groups.size());

    const auto& g_a = groups.at(0x80000002);
    TSUNIT_EQUAL(0x80000002u, g_a.download_id);
    TSUNIT_EQUAL(0x58860u, g_a.group_size);
    TSUNIT_ASSERT(g_a.announced_by_dsi);

    const auto& g_b = groups.at(0x80000003);
    TSUNIT_EQUAL(0x12345u, g_b.group_size);
    TSUNIT_ASSERT(g_b.announced_by_dsi);
}


// The group-completion callback fires exactly once when all known modules
// reach COMPLETE — even if more DDBs arrive afterwards.
TSUNIT_DEFINE_TEST(Carousel_GroupCompletionFiresExactlyOnce)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);

    std::vector<uint32_t> completed_groups;
    carousel.setGroupCompletedHandler(
        [&](const ts::DSMCCCarousel::GroupContext& gctx) {
            completed_groups.push_back(gctx.download_id);
        });

    const uint16_t block_size = 16;
    const ts::ByteBlock payload(block_size, 0xAA);

    auto dii = makeDII(0x000A, {0x0001, 0x0002}, block_size);
    carousel.feedUserToNetwork(dii);

    auto ddb_1 = makeDDB(0x000A, 0x0001, 0, payload);
    auto ddb_2 = makeDDB(0x000A, 0x0002, 0, payload);
    carousel.feedDownloadData(ddb_1);
    TSUNIT_EQUAL(0u, completed_groups.size());
    carousel.feedDownloadData(ddb_2);
    TSUNIT_EQUAL(1u, completed_groups.size());
    TSUNIT_EQUAL(0x000Au, completed_groups[0]);

    auto ddb_2_dup = makeDDB(0x000A, 0x0002, 0, payload);
    carousel.feedDownloadData(ddb_2_dup);
    TSUNIT_EQUAL(1u, completed_groups.size());
}


// Multiple DIIs sharing a download_id must accumulate module_ids — set
// semantics.
TSUNIT_DEFINE_TEST(Carousel_MultiDIIPerGroupAccumulatesModules)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);

    auto dii_a = makeDII(0x000A, {0x0001, 0x0002});
    auto dii_b = makeDII(0x000A, {0x0003});
    carousel.feedUserToNetwork(dii_a);
    carousel.feedUserToNetwork(dii_b);

    const auto& g = carousel.groups().at(0x000A);
    TSUNIT_EQUAL(3u, g.module_ids.size());
    TSUNIT_ASSERT(g.module_ids.count(0x0001) == 1);
    TSUNIT_ASSERT(g.module_ids.count(0x0002) == 1);
    TSUNIT_ASSERT(g.module_ids.count(0x0003) == 1);
}


// Object carousel: no DSI announces groups. The first DII must lazily
// synthesize a group keyed by its download_id.
TSUNIT_DEFINE_TEST(Carousel_ObjectCarouselSynthesizesGroupOnFirstDII)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);

    auto dii = makeDII(0x12345678, {0x0001, 0x0002});
    carousel.feedUserToNetwork(dii);

    const auto& groups = carousel.groups();
    TSUNIT_EQUAL(1u, groups.size());
    const auto it = groups.find(0x12345678);
    TSUNIT_ASSERT(it != groups.end());
    TSUNIT_EQUAL(0x12345678u, it->second.download_id);
    TSUNIT_ASSERT(!it->second.announced_by_dsi);
    TSUNIT_EQUAL(2u, it->second.module_ids.size());
    TSUNIT_EQUAL(0u, it->second.modules_complete);
}


// Two groups carrying the same module_id must complete independently.
TSUNIT_DEFINE_TEST(Carousel_OverlappingModuleIdsAcrossGroupsAreIndependent)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);

    std::vector<uint32_t> completed_groups;
    carousel.setGroupCompletedHandler(
        [&](const ts::DSMCCCarousel::GroupContext& gctx) {
            completed_groups.push_back(gctx.download_id);
        });

    const uint16_t block_size = 16;
    const ts::ByteBlock payload(block_size, 0xAA);

    carousel.feedUserToNetwork(makeDII(0x000A, {0x0001}, block_size));
    carousel.feedUserToNetwork(makeDII(0x000B, {0x0001}, block_size));

    auto ddb_a = makeDDB(0x000A, 0x0001, 0, payload);
    auto ddb_b = makeDDB(0x000B, 0x0001, 0, payload);
    carousel.feedDownloadData(ddb_a);
    carousel.feedDownloadData(ddb_b);

    TSUNIT_EQUAL(2u, completed_groups.size());
    TSUNIT_EQUAL(0x000Au, completed_groups[0]);
    TSUNIT_EQUAL(0x000Bu, completed_groups[1]);
}


//----------------------------------------------------------------------------
// BIOP tests
//----------------------------------------------------------------------------

// Parse a Directory message with one ncontext binding ("root" -> dir, empty IOR).
TSUNIT_DEFINE_TEST(BIOP_DirectoryMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x2D,
        0x04, 0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x04, 0x64, 0x69, 0x72, 0x00,
        0x00, 0x00,
        0x00,
        0x00, 0x00, 0x00, 0x19,
        0x00, 0x01,
        0x01,
        0x05, 0x72, 0x6F, 0x6F, 0x74, 0x00,
        0x04, 0x64, 0x69, 0x72, 0x00,
        0x02,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);

    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL("dir", msg->kindTag());

    auto* dir = dynamic_cast<ts::BIOPBindingListMessage*>(msg.get());
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


// XML round-trip for a BIOP Directory message.
TSUNIT_DEFINE_TEST(BIOP_DirectoryMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    ts::BIOPBindingListMessage orig;
    orig.object_key = {0x00, 0x00, 0x00, 0x02};
    orig.object_kind = {0x64, 0x69, 0x72, 0x00};

    ts::BIOPBinding b;
    ts::BIOPNameComponent nc;
    nc.id = {0x72, 0x6F, 0x6F, 0x74, 0x00};
    nc.kind = {0x64, 0x69, 0x72, 0x00};
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

    auto* dir = dynamic_cast<ts::BIOPBindingListMessage*>(restored.get());
    TSUNIT_ASSERT(dir != nullptr);
    TSUNIT_EQUAL(1u, dir->bindings.size());
    TSUNIT_EQUAL(u"root", dir->bindings[0].name[0].idString());
    TSUNIT_EQUAL("dir", dir->bindings[0].name[0].kindTag());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NCONTEXT, dir->bindings[0].binding_type);
}


// Parse a BIOP File message. Header bytes from a real broadcast; content
// truncated to the first 20 bytes, sizes adjusted accordingly.
TSUNIT_DEFINE_TEST(BIOP_FileMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x31,
        0x01, 0x02,
        0x00, 0x00, 0x00, 0x04,
        0x66, 0x69, 0x6C, 0x00,
        0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x89, 0x68,
        0x00,
        0x00, 0x00, 0x00, 0x18,
        0x00, 0x00, 0x00, 0x14,
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


// XML round-trip for BIOPFileMessage.
TSUNIT_DEFINE_TEST(BIOP_FileMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    ts::BIOPFileMessage orig;
    orig.object_key = {0x02};
    orig.object_kind = {0x66, 0x69, 0x6C, 0x00};
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


// FromXML returns nullptr for unsupported object_kind.
TSUNIT_DEFINE_TEST(BIOP_FromXMLDispatch)
{
    ts::DuckContext duck;

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    ts::xml::Element* xmsg = root->addElement(u"BIOP_message");
    xmsg->setAttribute(u"object_kind", u"str");

    auto msg = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(msg == nullptr);
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderBigEndian)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50,
        0x01, 0x00, 0x00, 0x00,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    ts::BIOPMessageHeader header;

    TSUNIT_ASSERT(header.deserialize(buf));
    TSUNIT_ASSERT(header.isValid());
    TSUNIT_EQUAL(0x42494F50u, header.magic);
    TSUNIT_EQUAL(0x01, header.version_major);
    TSUNIT_EQUAL(0x00, header.version_minor);
    TSUNIT_EQUAL(0x00, header.byte_order);
    TSUNIT_EQUAL(0x00, header.message_type);
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderClear)
{
    ts::BIOPMessageHeader header;

    header.magic = 0x12345678;
    header.version_major = 0xFF;
    header.version_minor = 0xFF;
    header.byte_order = 0xFF;
    header.message_type = 0xFF;

    header.clear();

    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MAGIC, header.magic);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MAJOR, header.version_major);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MINOR, header.version_minor);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN, header.byte_order);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MESSAGE_TYPE_STANDARD, header.message_type);
    TSUNIT_ASSERT(header.isValid());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderInvalidByteOrder)
{
    ts::BIOPMessageHeader header;

    header.byte_order = 0x01;
    TSUNIT_ASSERT(!header.isValid());

    header.byte_order = 0xFF;
    TSUNIT_ASSERT(!header.isValid());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderInvalidMagic)
{
    ts::BIOPMessageHeader header;
    header.magic = 0x12345678;
    TSUNIT_ASSERT(!header.isValid());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderInvalidVersion)
{
    ts::BIOPMessageHeader header;
    header.version_major = 0x02;
    TSUNIT_ASSERT(!header.isValid());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderSerializeDeserialize)
{
    ts::DuckContext duck;

    ts::BIOPMessageHeader header1;
    header1.magic = ts::BIOPMessageHeader::BIOP_MAGIC;
    header1.version_major = 0x01;
    header1.version_minor = 0x00;
    header1.byte_order = ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN;
    header1.message_type = 0x00;

    uint8_t buffer[ts::BIOPMessageHeader::HEADER_SIZE];
    ts::PSIBuffer buf1(duck, buffer, sizeof(buffer), false);
    TSUNIT_ASSERT(header1.serialize(buf1));

    ts::PSIBuffer buf2(duck, buffer, sizeof(buffer), true);
    ts::BIOPMessageHeader header2;
    TSUNIT_ASSERT(header2.deserialize(buf2));

    TSUNIT_EQUAL(header1.magic, header2.magic);
    TSUNIT_EQUAL(header1.version_major, header2.version_major);
    TSUNIT_EQUAL(header1.version_minor, header2.version_minor);
    TSUNIT_EQUAL(header1.byte_order, header2.byte_order);
    TSUNIT_EQUAL(header1.message_type, header2.message_type);
    TSUNIT_ASSERT(header2.isValid());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderTruncated)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50,
        0x01, 0x00, 0x00,  // missing message_type
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    ts::BIOPMessageHeader header;

    TSUNIT_ASSERT(!header.deserialize(buf));
    TSUNIT_ASSERT(buf.error());
}


TSUNIT_DEFINE_TEST(BIOP_MessageHeaderValid)
{
    ts::BIOPMessageHeader header;

    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MAGIC, header.magic);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MAJOR, header.version_major);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_VERSION_MINOR, header.version_minor);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_BYTE_ORDER_BIG_ENDIAN, header.byte_order);
    TSUNIT_EQUAL(ts::BIOPMessageHeader::BIOP_MESSAGE_TYPE_STANDARD, header.message_type);
    TSUNIT_ASSERT(header.isValid());
}


// Parse a ServiceGateway message with 3 nobject bindings. Binary data from a
// real broadcast, DSM::ServiceGateway module 0x0001.
TSUNIT_DEFINE_TEST(BIOP_ServiceGatewayMessageParse)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50,
        0x01, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x01, 0x1A,
        0x01, 0x01,
        0x00, 0x00, 0x00, 0x04,
        0x73, 0x72, 0x67, 0x00,
        0x00, 0x00,
        0x00,
        0x00, 0x00, 0x01, 0x09,
        0x00, 0x03,
        // Binding 1: deja.ttf
        0x01,
        0x09, 0x64, 0x65, 0x6A, 0x61, 0x2E, 0x74, 0x74, 0x66, 0x00,
        0x04, 0x66, 0x69, 0x6C, 0x00,
        0x01,
        0x00, 0x00, 0x00, 0x04, 0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x49, 0x53, 0x4F, 0x06, 0x00, 0x00, 0x00, 0x28,
        0x00, 0x02,
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x02, 0x01, 0x00, 0x01, 0x02,
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Binding 2: index.html
        0x01,
        0x0B, 0x69, 0x6E, 0x64, 0x65, 0x78, 0x2E, 0x68, 0x74, 0x6D, 0x6C, 0x00,
        0x04, 0x66, 0x69, 0x6C, 0x00,
        0x01,
        0x00, 0x00, 0x00, 0x04, 0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x49, 0x53, 0x4F, 0x06, 0x00, 0x00, 0x00, 0x28,
        0x00, 0x02,
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x03, 0x01, 0x00, 0x01, 0x03,
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Binding 3: rj45.gif
        0x01,
        0x09, 0x72, 0x6A, 0x34, 0x35, 0x2E, 0x67, 0x69, 0x66, 0x00,
        0x04, 0x66, 0x69, 0x6C, 0x00,
        0x01,
        0x00, 0x00, 0x00, 0x04, 0x66, 0x69, 0x6C, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x49, 0x53, 0x4F, 0x06, 0x00, 0x00, 0x00, 0x28,
        0x00, 0x02,
        0x49, 0x53, 0x4F, 0x50, 0x0A,
        0x00, 0x00, 0x00, 0x0A, 0x00, 0x03, 0x01, 0x00, 0x01, 0x04,
        0x49, 0x53, 0x4F, 0x40, 0x12,
        0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x0A,
        0x0A, 0x00, 0x01, 0x80, 0x00, 0x00, 0x02, 0x03, 0x93, 0x87, 0x00,
        0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);

    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_ASSERT(msg->header.isValid());
    TSUNIT_EQUAL("srg", msg->kindTag());
    TSUNIT_EQUAL(1u, msg->object_key.size());
    TSUNIT_EQUAL(0x01, msg->object_key[0]);
    TSUNIT_EQUAL(0u, msg->service_contexts.size());

    auto* sg = dynamic_cast<ts::BIOPBindingListMessage*>(msg.get());
    TSUNIT_ASSERT(sg != nullptr);
    TSUNIT_EQUAL(3u, sg->bindings.size());

    const auto& b0 = sg->bindings[0];
    TSUNIT_EQUAL(u"deja.ttf", b0.name[0].idString());
    TSUNIT_EQUAL("fil", b0.name[0].kindTag());
    TSUNIT_EQUAL(ts::BIOPBinding::BINDING_TYPE_NOBJECT, b0.binding_type);
    auto loc0 = b0.targetLocation();
    TSUNIT_ASSERT(loc0.has_value());
    TSUNIT_EQUAL(2u, loc0->first);
    TSUNIT_EQUAL(0x02, loc0->second[0]);

    const auto& b1 = sg->bindings[1];
    TSUNIT_EQUAL(u"index.html", b1.name[0].idString());
    auto loc1 = b1.targetLocation();
    TSUNIT_ASSERT(loc1.has_value());
    TSUNIT_EQUAL(3u, loc1->first);
    TSUNIT_EQUAL(0x03, loc1->second[0]);

    const auto& b2 = sg->bindings[2];
    TSUNIT_EQUAL(u"rj45.gif", b2.name[0].idString());
    auto loc2 = b2.targetLocation();
    TSUNIT_ASSERT(loc2.has_value());
    TSUNIT_EQUAL(3u, loc2->first);
    TSUNIT_EQUAL(0x04, loc2->second[0]);

    TSUNIT_ASSERT(!buf.error());
}


// XML round-trip for a BIOP ServiceGateway message (3 bindings with IOR).
TSUNIT_DEFINE_TEST(BIOP_ServiceGatewayMessageXMLRoundTrip)
{
    ts::DuckContext duck;

    auto makeBinding = [](const ts::ByteBlock& id, uint16_t module_id, uint8_t obj_key) {
        ts::BIOPBinding b;
        ts::BIOPNameComponent nc;
        nc.id = id;
        nc.kind = {0x66, 0x69, 0x6C, 0x00};
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

    ts::BIOPBindingListMessage orig;
    orig.object_key = {0x01};
    orig.object_kind = {0x73, 0x72, 0x67, 0x00};
    orig.bindings.push_back(makeBinding({0x64, 0x65, 0x6A, 0x61, 0x2E, 0x74, 0x74, 0x66, 0x00}, 2, 0x02));
    orig.bindings.push_back(makeBinding({0x69, 0x6E, 0x64, 0x65, 0x78, 0x2E, 0x68, 0x74, 0x6D, 0x6C, 0x00}, 3, 0x03));
    orig.bindings.push_back(makeBinding({0x72, 0x6A, 0x34, 0x35, 0x2E, 0x67, 0x69, 0x66, 0x00}, 3, 0x04));

    ts::xml::Document doc;
    ts::xml::Element* root = doc.initialize(u"test");
    orig.toXML(duck, root);

    const ts::xml::Element* xmsg = root->findFirstChild(u"BIOP_message", true);
    TSUNIT_ASSERT(xmsg != nullptr);

    auto restored = ts::BIOPMessage::FromXML(duck, xmsg);
    TSUNIT_ASSERT(restored != nullptr);
    TSUNIT_EQUAL("srg", restored->kindTag());
    TSUNIT_ASSERT(orig.object_key == restored->object_key);

    auto* sg = dynamic_cast<ts::BIOPBindingListMessage*>(restored.get());
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


// Parse returns nullptr (not an error) for a kind we don't support yet.
// The read pointer must advance past the full message.
TSUNIT_DEFINE_TEST(BIOP_UnsupportedKindReturnsNull)
{
    ts::DuckContext duck;

    uint8_t data[] = {
        0x42, 0x49, 0x4F, 0x50, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x18,
        0x04, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x04, 0x73, 0x74, 0x72, 0x00,
        0x00, 0x00,
        0x00,
        0x00, 0x00, 0x00, 0x04,
        0xDE, 0xAD, 0xBE, 0xEF,
    };

    ts::PSIBuffer buf(duck, data, sizeof(data), true);
    auto msg = ts::BIOPMessage::Parse(buf);
    TSUNIT_ASSERT(msg == nullptr);
    TSUNIT_ASSERT(!buf.error());
    TSUNIT_EQUAL(sizeof(data), buf.currentReadByteOffset());
}
