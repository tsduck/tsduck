//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Unit tests for DSMCCModuleAssembler (group-scoped keying).
//
//----------------------------------------------------------------------------

#include "tsDSMCCModuleAssembler.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsDSMCCNameDescriptor.h"
#include "tsDID.h"
#include "tsDuckContext.h"
#include "tsunit.h"


class DSMCCModuleAssemblerTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(MultiGroupSameModuleIdDoesNotCollide);
    TSUNIT_DECLARE_TEST(SingleGroupBehaviourPreserved);
    TSUNIT_DECLARE_TEST(DiscoveryFiresOncePerKey);
    TSUNIT_DECLARE_TEST(DiscoverySuppressedOnVersionBump);
    TSUNIT_DECLARE_TEST(DiscoveryFiresOnceWhenDDBPrecedesDII);
    TSUNIT_DECLARE_TEST(DescriptorsSurfacedFromDII);
};

TSUNIT_REGISTER(DSMCCModuleAssemblerTest);


namespace {
    // Build a minimal DII announcing one module, on a given download_id and
    // block_size. `module_size` is fixed to `block_size` so exactly one DDB
    // completes the module.
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

    // Build a DDB carrying the full module payload.
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
}


// Two DIIs with the same module_id but different download_ids must produce
// two distinct completions — previously the second would have overwritten
// the first in `_modules`.
TSUNIT_DEFINE_TEST(MultiGroupSameModuleIdDoesNotCollide)
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

    // Two groups, same module_id.
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

    // Payloads must not be cross-contaminated.
    TSUNIT_EQUAL(block_size, payloads[0].size());
    TSUNIT_EQUAL(block_size, payloads[1].size());
    TSUNIT_EQUAL(0xAAu, payloads[0][0]);
    TSUNIT_EQUAL(0xBBu, payloads[1][0]);
}


// Discovery callback fires exactly once per (download_id, module_id) pair on
// the first DII that announces it.
TSUNIT_DEFINE_TEST(DiscoveryFiresOncePerKey)
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
    // Re-feeding the same DII must not re-discover.
    assembler.feedUserToNetwork(dii_1);

    TSUNIT_EQUAL(3u, discovered.size());
    TSUNIT_EQUAL(0x000Au, discovered[0].first);
    TSUNIT_EQUAL(0x0001u, discovered[0].second);
    TSUNIT_EQUAL(0x000Au, discovered[1].first);
    TSUNIT_EQUAL(0x0002u, discovered[1].second);
    TSUNIT_EQUAL(0x000Bu, discovered[2].first);
    TSUNIT_EQUAL(0x0001u, discovered[2].second);
}


// A version bump on an already-known module must not re-fire discovery —
// the module has already been counted by the carousel-level group accounting.
TSUNIT_DEFINE_TEST(DiscoverySuppressedOnVersionBump)
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


// DDB arriving before its DII goes into the orphan buffer. When the DII
// finally arrives, discovery must fire exactly once (from the DII path),
// not also from the orphan replay path.
TSUNIT_DEFINE_TEST(DiscoveryFiresOnceWhenDDBPrecedesDII)
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


// A DII carrying a name_descriptor must produce a ModuleContext whose descs
// field is non-empty and from which the name round-trips correctly.
TSUNIT_DEFINE_TEST(DescriptorsSurfacedFromDII)
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


// Single-group stream: one download_id, three modules. Exercises the
// (download_id, module_id) map in its common-case shape.
TSUNIT_DEFINE_TEST(SingleGroupBehaviourPreserved)
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
