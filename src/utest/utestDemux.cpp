//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for demux classes
//
//----------------------------------------------------------------------------

#include "tsSectionDemux.h"
#include "tsStandaloneTableDemux.h"
#include "tsOneShotPacketizer.h"
#include "tsDuckContext.h"
#include "tsTSPacket.h"
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsNIT.h"
#include "tsBAT.h"
#include "tsTOT.h"
#include "tsTDT.h"
#include "tsunit.h"

#include "tables/psi_bat_cplus_packets.h"
#include "tables/psi_bat_cplus_sections.h"
#include "tables/psi_bat_tvnum_packets.h"
#include "tables/psi_bat_tvnum_sections.h"
#include "tables/psi_cat_r3_packets.h"
#include "tables/psi_cat_r3_sections.h"
#include "tables/psi_cat_r6_packets.h"
#include "tables/psi_cat_r6_sections.h"
#include "tables/psi_nit_tntv23_packets.h"
#include "tables/psi_nit_tntv23_sections.h"
#include "tables/psi_pat_r4_packets.h"
#include "tables/psi_pat_r4_sections.h"
#include "tables/psi_pmt_planete_packets.h"
#include "tables/psi_pmt_planete_sections.h"
#include "tables/psi_sdt_r3_packets.h"
#include "tables/psi_sdt_r3_sections.h"
#include "tables/psi_tdt_tnt_packets.h"
#include "tables/psi_tdt_tnt_sections.h"
#include "tables/psi_tot_tnt_packets.h"
#include "tables/psi_tot_tnt_sections.h"
#include "tables/psi_pmt_hevc_packets.h"
#include "tables/psi_pmt_hevc_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class DemuxTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(PAT);
    TSUNIT_DECLARE_TEST(CATR3);
    TSUNIT_DECLARE_TEST(CATR6);
    TSUNIT_DECLARE_TEST(PMT);
    TSUNIT_DECLARE_TEST(SDT);
    TSUNIT_DECLARE_TEST(NIT);
    TSUNIT_DECLARE_TEST(BATTvNumeric);
    TSUNIT_DECLARE_TEST(BATCanalPlus);
    TSUNIT_DECLARE_TEST(TDT);
    TSUNIT_DECLARE_TEST(TOT);
    TSUNIT_DECLARE_TEST(HEVC);

private:
    // Compare a table with the list of reference sections
    bool checkSections(const char* test_name, const char* table_name, const ts::BinaryTable& table, const uint8_t* ref_sections, size_t ref_sections_size);

    // Compare a vector of packets with the list of reference packets
    bool checkPackets(const char* test_name, const char* table_name, const ts::TSPacketVector& packets, const uint8_t* ref_packets, size_t ref_packets_size);

    // Unitary test for one table.
    void testTable(const char* name, const uint8_t* ref_packets, size_t ref_packets_size, const uint8_t* ref_sections, size_t ref_sections_size);
};

TSUNIT_REGISTER(DemuxTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Compare a table with the list of reference sections
bool DemuxTest::checkSections(const char* test_name, const char* table_name, const ts::BinaryTable& table, const uint8_t* ref_sections, size_t ref_sections_size)
{
    // First, compute and compare total size of the table
    size_t total_size(0);
    for (size_t si = 0; si < table.sectionCount(); ++si) {
        total_size += table.sectionAt(si)->size();
    }
    if (total_size != ref_sections_size) {
        debug() << "DemuxTest: " << test_name << ", " << table_name
                     << ": total size of " << table.sectionCount() << " sections is "
                     << total_size << " bytes, expected " << ref_sections_size << " bytes"
                     << std::endl
                     << "DemuxTest: Reference sections:" << std::endl
                     << ts::UString::Dump(ref_sections, ref_sections_size, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2)
                     << "DemuxTest: " << table_name << std::endl;
        for (size_t si = 0; si < table.sectionCount(); ++si) {
            const ts::Section& sect(*table.sectionAt(si));
            debug() << "DemuxTest: " << ts::UString::Dump(sect.content(), sect.size(), ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2);
        }
        return false;
    }

    // Then compare contents of sections
    size_t sections_offset(0);
    for (size_t si = 0; si < table.sectionCount(); ++si) {
        const ts::Section& sect(*table.sectionAt(si));
        const uint8_t* ref = ref_sections + sections_offset;
        const uint8_t* sec = sect.content();
        size_t size = sect.size();
        sections_offset += size;
        for (size_t i = 0; i < size; ++i) {
            if (sec[i] != ref[i]) {
                debug() << "DemuxTest: " << test_name << ", " << table_name
                             << ": difference at offset " << i << " in section " << si
                             << std::endl
                             << "DemuxTest: Reference section:" << std::endl
                             << ts::UString::Dump(ref, size, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2)
                             << "DemuxTest: " << table_name << std::endl
                             << ts::UString::Dump(sec, size, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2);
                return false;
            }
        }
    }
    return true;
}

// Compare a vector of packets with the list of reference packets
bool DemuxTest::checkPackets(const char* test_name, const char* table_name, const ts::TSPacketVector& packets, const uint8_t* ref_packets, size_t ref_packets_size)
{
    // First, compute and compare total size of the table
    if (packets.size() != ref_packets_size / ts::PKT_SIZE) {
        debug() << "DemuxTest: " << test_name << ", " << table_name
                     << ": rebuilt " << packets.size() << " packets, expected " << (ref_packets_size / ts::PKT_SIZE)
                     << std::endl
                     << "DemuxTest: Reference packets:" << std::endl
                     << ts::UString::Dump(ref_packets, ref_packets_size, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2)
                     << "* " << table_name << ":" << std::endl
                     << ts::UString::Dump(packets[0].b, packets.size() * ts::PKT_SIZE, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2);
        return false;
    }

    // Then compare contents of packets
    for (size_t pi = 0; pi < packets.size(); ++pi) {
        const uint8_t* ref = ref_packets + pi * ts::PKT_SIZE;
        const uint8_t* pkt = packets[pi].b;
        for (size_t i = 0; i < ts::PKT_SIZE; ++i) {
            if (pkt[i] != ref[i]) {
                debug() << "DemuxTest: " << test_name << ", " << table_name
                             << ": difference at offset " << i << " in packet " << pi
                             << std::endl
                             << "DemuxTest: Reference packet:" << std::endl
                             << ts::UString::Dump(ref, ts::PKT_SIZE, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2)
                             << "DemuxTest: " << table_name << ":" << std::endl
                             << ts::UString::Dump(pkt, ts::PKT_SIZE, ts::UString::HEXA | ts::UString::OFFSET | ts::UString::ASCII, 2);
                return false;
            }
        }
    }
    return true;
}

// Unitary test for one table.
void DemuxTest::testTable(const char* name, const uint8_t* ref_packets, size_t ref_packets_size, const uint8_t* ref_sections, size_t ref_sections_size)
{
    ts::DuckContext duck;

    TSUNIT_ASSERT(ref_packets_size % ts::PKT_SIZE == 0);
    debug() << "DemuxTest: Testing " << name << std::endl;

    // Analyze TS packets. We expect only one table

    const ts::TSPacket* ref_pkt = reinterpret_cast<const ts::TSPacket*>(ref_packets);
    ts::StandaloneTableDemux demux(duck, ts::AllPIDs());

    for (size_t pi = 0; pi < ref_packets_size / ts::PKT_SIZE; ++pi) {
        demux.feedPacket(ref_pkt[pi]);
    }
    TSUNIT_EQUAL(1, demux.tableCount());

    // Compare contents of reference sections and demuxed sections.

    const ts::BinaryTable& table1(*demux.tableAt(0));
    debug() << "DemuxTest: " << ts::UString::Format(u"  PID %n", table1.sourcePID()) << std::endl;
    TSUNIT_ASSERT(checkSections(name, "demuxed table", table1, ref_sections, ref_sections_size));

    // Table-specific tests.
    // Check known values in the test tables.
    // Reserialize the table

    ts::BinaryTable table2;

    switch (table1.tableId()) {
        case ts::TID_PAT: { // TNT R4
            ts::PAT pat(duck, table1);
            TSUNIT_ASSERT(pat.ts_id == 0x0004);
            TSUNIT_ASSERT(pat.nit_pid == 0x0010);
            TSUNIT_ASSERT(pat.pmts.size() == 7);
            TSUNIT_ASSERT(pat.pmts[0x0403] == 0x0136);
            pat.serialize(duck, table2);
            break;
        }
        case ts::TID_CAT: { // TNT R3 or R6
            ts::CAT cat(duck, table1);
            TSUNIT_ASSERT(cat.descs.count() == 1 || cat.descs.count() == 2);
            cat.serialize(duck, table2);
            break;
        }
        case ts::TID_PMT: { // Planete (TNT R3) or HEVC
            ts::PMT pmt(duck, table1);
            switch (pmt.service_id) {
                case 0x0304: { // Planete
                    TSUNIT_ASSERT(pmt.pcr_pid == 0x00A3);
                    TSUNIT_ASSERT(pmt.descs.count() == 1);
                    TSUNIT_ASSERT(pmt.descs[0].tag() == ts::DID_MPEG_CA);
                    TSUNIT_ASSERT(pmt.streams.size() == 2);
                    TSUNIT_ASSERT(pmt.streams[0x00A3].stream_type == 0x1B);
                    TSUNIT_ASSERT(pmt.streams[0x00A3].descs.count() == 3);
                    TSUNIT_ASSERT(pmt.streams[0x005C].stream_type == 0x04);
                    TSUNIT_ASSERT(pmt.streams[0x005C].descs.count() == 3);
                    break;
                }
                case 0x11FB: { // HEVC
                    TSUNIT_ASSERT(pmt.pcr_pid == 0x01C9);
                    TSUNIT_ASSERT(pmt.descs.count() == 0);
                    TSUNIT_ASSERT(pmt.streams.size() == 2);
                    TSUNIT_ASSERT(pmt.streams[0x01C9].stream_type == 0x24);
                    TSUNIT_ASSERT(pmt.streams[0x01C9].descs.count() == 1);
                    TSUNIT_ASSERT(pmt.streams[0x01C9].descs[0].tag() == ts::DID_MPEG_HEVC_VIDEO);
                    TSUNIT_ASSERT(pmt.streams[0x01CA].stream_type == 0x0F);
                    TSUNIT_ASSERT(pmt.streams[0x01CA].descs.count() == 2);
                    break;
                }
                default: {
                    TSUNIT_FAIL("unexpected service id");
                }
            }
            pmt.serialize(duck, table2);
            break;
        }
        case ts::TID_SDT_ACT: { // TNT R3
            ts::SDT sdt(duck, table1);
            TSUNIT_ASSERT(sdt.ts_id == 0x0003);
            TSUNIT_ASSERT(sdt.onetw_id == 0x20FA);
            TSUNIT_ASSERT(sdt.services.size() == 8);
            TSUNIT_ASSERT(sdt.services[0x0304].EITpf_present);
            TSUNIT_ASSERT(!sdt.services[0x0304].EITs_present);
            TSUNIT_ASSERT(sdt.services[0x0304].running_status == 4); // running
            TSUNIT_ASSERT(sdt.services[0x0304].CA_controlled);
            TSUNIT_ASSERT(sdt.services[0x0304].descs.count() == 1);
            TSUNIT_ASSERT(sdt.services[0x0304].descs[0].tag() == ts::DID_DVB_SERVICE);
            TSUNIT_ASSERT(sdt.services[0x0304].serviceType(duck) == 0x01);
            TSUNIT_ASSERT(sdt.services[0x0304].serviceName(duck) == u"PLANETE");
            TSUNIT_ASSERT(sdt.services[0x0304].providerName(duck) == u"CNH");
            sdt.serialize(duck, table2);
            break;
        }
        case ts::TID_NIT_ACT: { // TNT v23
            ts::NIT nit(duck, table1);
            TSUNIT_ASSERT(nit.network_id == 0x20FA);
            TSUNIT_ASSERT(nit.descs.count() == 8);
            TSUNIT_ASSERT(nit.descs[0].tag() == ts::DID_DVB_NETWORK_NAME);
            TSUNIT_ASSERT(nit.descs[7].tag() == ts::DID_DVB_LINKAGE);
            TSUNIT_ASSERT(nit.transports.size() == 7);
            ts::TransportStreamId id(0x0004, 0x20FA); // TNT R4
            TSUNIT_ASSERT(nit.transports[id].descs.count() == 4);
            TSUNIT_ASSERT(nit.transports[id].descs[0].tag() == ts::DID_DVB_PRIV_DATA_SPECIF);
            TSUNIT_ASSERT(nit.transports[id].descs[3].tag() == ts::DID_DVB_TERREST_DELIVERY);
            nit.serialize(duck, table2);
            break;
        }
        case ts::TID_BAT: { // Tv Numeric or Canal+ TNT
            ts::BAT bat(duck, table1);
            switch (bat.bouquet_id) {
                case 0x0086: { // Tv Numeric
                    TSUNIT_ASSERT(bat.descs.count() == 5);
                    TSUNIT_ASSERT(bat.descs[0].tag() == ts::DID_DVB_BOUQUET_NAME);
                    TSUNIT_ASSERT(bat.descs[4].tag() == ts::DID_LW_SUBSCRIPTION);
                    TSUNIT_ASSERT(bat.transports.size() == 3);
                    ts::TransportStreamId id(0x0006, 0x20FA); // TNT R6
                    TSUNIT_ASSERT(bat.transports[id].descs.count() == 1);
                    TSUNIT_ASSERT(bat.transports[id].descs[0].tag() == ts::DID_DVB_SERVICE_LIST);
                    break;
                }
                case 0xC003: { // Canal+ TNT
                    TSUNIT_ASSERT(bat.descs.count() == 4);
                    TSUNIT_ASSERT(bat.descs[0].tag() == ts::DID_DVB_BOUQUET_NAME);
                    TSUNIT_ASSERT(bat.descs[1].tag() == ts::DID_DVB_LINKAGE);
                    TSUNIT_ASSERT(bat.transports.size() == 6);
                    ts::TransportStreamId id(0x0003, 0x20FA); // TNT R3
                    TSUNIT_ASSERT(bat.transports[id].descs.count() == 5);
                    TSUNIT_ASSERT(bat.transports[id].descs[0].tag() == ts::DID_DVB_SERVICE_LIST);
                    break;
                }
                default: {
                    TSUNIT_FAIL("unexpected bouquet id");
                }
            }
            bat.serialize(duck, table2);
            break;
        }
        case ts::TID_TDT: { // TNT
            ts::TDT tdt(duck, table1);
            TSUNIT_ASSERT(tdt.utc_time == ts::Time(2007, 11, 23, 13, 25, 03));
            tdt.serialize(duck, table2);
            break;
        }
        case ts::TID_TOT: { // TNT
            ts::TOT tot(duck, table1);
            TSUNIT_ASSERT(tot.utc_time == ts::Time(2007, 11, 23, 13, 25, 14));
            TSUNIT_ASSERT(tot.regions.size() == 1);
            TSUNIT_ASSERT(tot.descs.count() == 0);
            TSUNIT_ASSERT(tot.regions[0].country == u"FRA");
            TSUNIT_ASSERT(tot.regions[0].region_id == 0);
            TSUNIT_ASSERT(tot.regions[0].time_offset == cn::minutes(60));
            TSUNIT_ASSERT(tot.regions[0].next_change == ts::Time(2008, 3, 30, 1, 0, 0));
            TSUNIT_ASSERT(tot.regions[0].next_time_offset == cn::minutes(120));
            tot.serialize(duck, table2);
            break;
        }
        default: {
            TSUNIT_FAIL("unexpected table id");
        }
    }

    // Now we have:
    //   BinaryTable table1  -> as demuxed from referenced packets
    //   BinaryTable table2  -> deserialized/check/serialized from table1
    //
    // It is not valid to compare the two binary tables. The
    // deserialization / serialization process may have changed the
    // order of some elements.

    // Repacketize table1 and check that the packets are identical to
    // the reference packets.

    ts::TSPacketVector packets;
    ts::OneShotPacketizer pzer(duck, table1.sourcePID(), true);

    pzer.setNextContinuityCounter(ref_pkt[0].getCC());
    pzer.addTable(table1);
    pzer.getPackets(packets);

    TSUNIT_ASSERT(checkPackets(name, "rebuilt packets", packets, ref_packets, ref_packets_size));

    // Packetize the serialized table

    pzer.reset();
    pzer.addTable(table2);
    pzer.getPackets(packets);

    // Reanalyze the packetized table and check it is identical to table2

    ts::StandaloneTableDemux demux2(duck, ts::AllPIDs());

    for (const auto& pkt : packets) {
        demux2.feedPacket(pkt);
    }
    TSUNIT_EQUAL(1, demux2.tableCount());

    const ts::BinaryTable& table3(*demux2.tableAt(0));
    if (table2 != table3) {
        debug() << "DemuxTest: " << name << ": rebuilt tables differ" << std::endl;
        debug() << "DemuxTest:   Re-serialized table: " << ts::TIDName(duck, table2.tableId())
                << ", " << table2.sectionCount() << " sections" << std::endl
                << "  Re-packetized table: " << ts::TIDName(duck, table3.tableId())
                << ", " << table3.sectionCount() << " sections" << std::endl;
    }
    TSUNIT_ASSERT(table2 == table3);
}

#define TEST_TABLE(title,name) testTable(title,               \
         psi_##name##_packets, sizeof(psi_##name##_packets),  \
         psi_##name##_sections, sizeof(psi_##name##_sections))

TSUNIT_DEFINE_TEST(PAT)
{
    TEST_TABLE("PAT: TNT R4", pat_r4);
}

TSUNIT_DEFINE_TEST(CATR3)
{
    TEST_TABLE("CAT: TNT R3", cat_r3);
}

TSUNIT_DEFINE_TEST(CATR6)
{
    TEST_TABLE("CAT: TNT R6", cat_r6);
}

TSUNIT_DEFINE_TEST(PMT)
{
    TEST_TABLE("PMT: Planete (TNT R3)", pmt_planete);
}

TSUNIT_DEFINE_TEST(SDT)
{
    TEST_TABLE("SDT: TNT R3", sdt_r3);
}

TSUNIT_DEFINE_TEST(NIT)
{
    TEST_TABLE("NIT: TNT v23", nit_tntv23);
}

TSUNIT_DEFINE_TEST(BATTvNumeric)
{
    TEST_TABLE("BAT: Tv Numeric", bat_tvnum);
}

TSUNIT_DEFINE_TEST(BATCanalPlus)
{
    TEST_TABLE("BAT: Canal+ TNT", bat_cplus);
}

TSUNIT_DEFINE_TEST(TDT)
{
    TEST_TABLE("TDT: TNT", tdt_tnt);
}

TSUNIT_DEFINE_TEST(TOT)
{
    TEST_TABLE("TOT: TNT", tot_tnt);
}

TSUNIT_DEFINE_TEST(HEVC)
{
    TEST_TABLE("PMT with HEVC descriptor", pmt_hevc);
}
