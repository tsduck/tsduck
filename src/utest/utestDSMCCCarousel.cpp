//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Unit tests for DSMCCCarousel group bookkeeping.
//
//----------------------------------------------------------------------------

#include "tsDSMCCCarousel.h"
#include "tsDSMCCUserToNetworkMessage.h"
#include "tsDSMCCDownloadDataMessage.h"
#include "tsDuckContext.h"
#include "tsunit.h"


class DSMCCCarouselTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ObjectCarouselSynthesizesGroupOnFirstDII);
    TSUNIT_DECLARE_TEST(DataCarouselDSIPopulatesGroups);
    TSUNIT_DECLARE_TEST(GroupCompletionFiresExactlyOnce);
    TSUNIT_DECLARE_TEST(MultiDIIPerGroupAccumulatesModules);
    TSUNIT_DECLARE_TEST(OverlappingModuleIdsAcrossGroupsAreIndependent);
};

TSUNIT_REGISTER(DSMCCCarouselTest);


namespace {
    // Build a DII with N modules on a given download_id; module ids
    // come from the caller via the `module_ids` list.
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

    // Build a DSI with a GroupInfoIndication listing the given (group_id, group_size) entries.
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
}


// Object carousel: no DSI announces groups. The first DII must lazily
// synthesize a group keyed by its download_id.
TSUNIT_DEFINE_TEST(ObjectCarouselSynthesizesGroupOnFirstDII)
{
    ts::DuckContext duck;
    ts::DSMCCCarousel carousel(duck);
    carousel.setScanBIOP(false);  // skip BIOP parsing for synthetic payloads

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


// Data carousel: a DSI's GroupInfoIndication populates _groups with
// announced_by_dsi=true, group_size, and any compatibility descriptors.
TSUNIT_DEFINE_TEST(DataCarouselDSIPopulatesGroups)
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
TSUNIT_DEFINE_TEST(GroupCompletionFiresExactlyOnce)
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
    TSUNIT_EQUAL(0u, completed_groups.size());  // not yet complete
    carousel.feedDownloadData(ddb_2);
    TSUNIT_EQUAL(1u, completed_groups.size());
    TSUNIT_EQUAL(0x000Au, completed_groups[0]);

    // Repeating the last DDB must not refire the group-complete callback.
    auto ddb_2_dup = makeDDB(0x000A, 0x0002, 0, payload);
    carousel.feedDownloadData(ddb_2_dup);
    TSUNIT_EQUAL(1u, completed_groups.size());
}


// Multiple DIIs sharing a download_id (e.g. a group whose modules span
// multiple DIIs) must accumulate module_ids — set semantics.
TSUNIT_DEFINE_TEST(MultiDIIPerGroupAccumulatesModules)
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


// Two groups carrying the same module_id must complete independently —
// the carousel-level group accounting must not cross-contaminate.
TSUNIT_DEFINE_TEST(OverlappingModuleIdsAcrossGroupsAreIndependent)
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
