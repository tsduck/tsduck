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
#include "tsDuckContext.h"
#include "tsunit.h"


class DSMCCModuleAssemblerTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(MultiGroupSameModuleIdDoesNotCollide);
    TSUNIT_DECLARE_TEST(SingleGroupBehaviourPreserved);
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
