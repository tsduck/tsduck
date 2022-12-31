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
//  TSUnit test suite for packetizer classes
//
//----------------------------------------------------------------------------

#include "tsPacketizer.h"
#include "tsCyclingPacketizer.h"
#include "tsStandaloneTableDemux.h"
#include "tsDuckContext.h"
#include "tsTSPacket.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsNames.h"
#include "tsunit.h"

#include "tables/psi_pat_r4_packets.h"
#include "tables/psi_pmt_planete_packets.h"
#include "tables/psi_sdt_r3_packets.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class PacketizerTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testPacketizer();

    TSUNIT_TEST_BEGIN(PacketizerTest);
    TSUNIT_TEST(testPacketizer);
    TSUNIT_TEST_END();

private:
    // Demux one table from a list of packets
    static void DemuxTable(ts::BinaryTablePtr& binTable, const char* name, const uint8_t* packets, size_t packets_size);
};

TSUNIT_REGISTER(PacketizerTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void PacketizerTest::beforeTest()
{
}

// Test suite cleanup method.
void PacketizerTest::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

// Demux one table from a list of packets
void PacketizerTest::DemuxTable(ts::BinaryTablePtr& binTable, const char* name, const uint8_t* packets, size_t packets_size)
{
    binTable.clear();

    debug() << "PacketizerTest: DemuxTable: Rebuilding " << name << std::endl;
    TSUNIT_EQUAL(0, packets_size % ts::PKT_SIZE);

    ts::DuckContext duck;
    ts::StandaloneTableDemux demux(duck, ts::AllPIDs);
    const ts::TSPacket* pkt = reinterpret_cast<const ts::TSPacket*>(packets);
    for (size_t pi = 0; pi < packets_size / ts::PKT_SIZE; ++pi) {
        demux.feedPacket (pkt[pi]);
    }
    TSUNIT_EQUAL(1, demux.tableCount());

    binTable = demux.tableAt(0);
    TSUNIT_ASSERT(!binTable.isNull());
    TSUNIT_ASSERT(binTable->isValid());
}

void PacketizerTest::testPacketizer()
{
    // Build a PAT, PMT and SDT. All these tables use one packet.

    ts::DuckContext duck;
    ts::BinaryTablePtr binpat;
    ts::BinaryTablePtr binpmt;
    ts::BinaryTablePtr binsdt;

    DemuxTable(binpat, "PAT", psi_pat_r4_packets, sizeof(psi_pat_r4_packets));
    DemuxTable(binpmt, "PMT", psi_pmt_planete_packets, sizeof(psi_pmt_planete_packets));
    DemuxTable(binsdt, "SDT", psi_sdt_r3_packets, sizeof(psi_sdt_r3_packets));

    ts::PAT pat(duck, *binpat);
    ts::PMT pmt(duck, *binpmt);
    ts::SDT sdt(duck, *binsdt);

    TSUNIT_ASSERT(pat.isValid());
    TSUNIT_ASSERT(pmt.isValid());
    TSUNIT_ASSERT(sdt.isValid());

    // Packetize these sections using specific repetition rates.

    const ts::BitRate bitrate = ts::PKT_SIZE_BITS * 10; // 10 packets per second

    ts::CyclingPacketizer pzer(duck, ts::PID_PAT, ts::CyclingPacketizer::StuffingPolicy::ALWAYS, bitrate);
    pzer.addTable(duck, pat);        // unscheduled
    pzer.addTable(duck, pmt, 1000);  // 1000 ms => 1 table / second
    pzer.addTable(duck, sdt, 250);   // 250 ms => 4 tables / second

    debug() << "PacketizerTest: Packetizer state before packetization: " << std::endl << pzer;

    // Generate 40 packets (4 seconds)

    ts::SectionCounter pat_count = 0;
    ts::SectionCounter pmt_count = 0;
    ts::SectionCounter sdt_count = 0;

    for (int pi = 1; pi <= 40; ++pi) {
        ts::TSPacket pkt;
        pzer.getNextPacket(pkt);
        TSUNIT_EQUAL(ts::SYNC_BYTE, pkt.b[0]);
        TSUNIT_EQUAL(0, pkt.b[4]); // pointer field
        ts::TID tid = pkt.b[5];
        debug() << "PacketizerTest:   " << pi << ": " << ts::names::TID(duck, tid) << std::endl;
        switch (tid) {
            case ts::TID_PAT:
                pat_count++;
                break;
            case ts::TID_PMT:
                pmt_count++;
                break;
            case ts::TID_SDT_ACT:
                sdt_count++;
                break;
            default:
                TSUNIT_FAIL("unexpected TID");
        }
    }

    debug() << "PacketizerTest: Table count: " << pat_count << " PAT, " << pmt_count << " PMT, " << sdt_count << " SDT" << std::endl
            << "PacketizerTest: Packetizer state after packetization: " << std::endl << pzer;

    TSUNIT_ASSERT(pmt_count == 4);
    TSUNIT_ASSERT(sdt_count >= 12 && sdt_count <= 18);
}
