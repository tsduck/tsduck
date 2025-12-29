//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for class ts::MPEPacket
//
//----------------------------------------------------------------------------

#include "tsMPEPacket.h"
#include "tsunit.h"
#include "tables/psi_mpe_sections.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class MPEPacketTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(Section);
    TSUNIT_DECLARE_TEST(Build);
};

TSUNIT_REGISTER(MPEPacketTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(Section)
{
    const ts::PID pid = 1234;
    ts::Section sec(psi_mpe_sections, sizeof(psi_mpe_sections), pid, ts::CRC32::CHECK);

    TSUNIT_ASSERT(sec.isValid());
    TSUNIT_EQUAL(ts::TID_DSMCC_PD, sec.tableId()); // DSM-CC Private Data
    TSUNIT_EQUAL(pid, sec.sourcePID());
    TSUNIT_ASSERT(sec.isLongSection());

    ts::MPEPacket mpe(sec);
    TSUNIT_ASSERT(mpe.isValid());
    TSUNIT_EQUAL(pid, mpe.sourcePID());
    TSUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(0x01, 0x00, 0x5E, 0x14, 0x14, 0x02));
    TSUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPAddress(224, 20, 20, 2));
    TSUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPAddress(192, 168, 135, 190));
    TSUNIT_EQUAL(6000, mpe.sourceUDPPort());
    TSUNIT_EQUAL(6000, mpe.destinationUDPPort());
    TSUNIT_EQUAL(1468, mpe.udpMessageSize());
}

TSUNIT_DEFINE_TEST(Build)
{
    ts::MPEPacket mpe;
    TSUNIT_ASSERT(!mpe.isValid());
    TSUNIT_EQUAL(ts::PID_NULL, mpe.sourcePID());

    static const uint8_t ref[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    mpe.setSourcePID(765);
    mpe.setDestinationMACAddress(ts::MACAddress(6, 7, 8, 9, 10, 11));
    mpe.setSourceIPAddress(ts::IPAddress(54, 59, 197, 201));
    mpe.setDestinationIPAddress(ts::IPAddress(123, 34, 45, 78));
    mpe.setSourceUDPPort(7920);
    mpe.setDestinationUDPPort(4654);
    mpe.setUDPMessage(ref, sizeof(ref));

    TSUNIT_ASSERT(mpe.isValid());
    TSUNIT_EQUAL(765, mpe.sourcePID());
    TSUNIT_ASSERT(mpe.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    TSUNIT_ASSERT(mpe.sourceIPAddress() == ts::IPAddress(54, 59, 197, 201));
    TSUNIT_ASSERT(mpe.destinationIPAddress() == ts::IPAddress(123, 34, 45, 78));
    TSUNIT_EQUAL(7920, mpe.sourceUDPPort());
    TSUNIT_EQUAL(4654, mpe.destinationUDPPort());
    TSUNIT_EQUAL(sizeof(ref), mpe.udpMessageSize());
    TSUNIT_ASSERT(mpe.udpMessage() != nullptr);
    TSUNIT_EQUAL(0, ts::MemCompare(mpe.udpMessage(), ref, mpe.udpMessageSize()));

    ts::Section sect;
    mpe.createSection(sect);
    TSUNIT_ASSERT(sect.isValid());

    ts::MPEPacket mpe2(sect);
    TSUNIT_ASSERT(mpe2.isValid());
    TSUNIT_EQUAL(765, mpe2.sourcePID());
    TSUNIT_ASSERT(mpe2.destinationMACAddress() == ts::MACAddress(6, 7, 8, 9, 10, 11));
    TSUNIT_ASSERT(mpe2.sourceIPAddress() == ts::IPAddress(54, 59, 197, 201));
    TSUNIT_ASSERT(mpe2.destinationIPAddress() == ts::IPAddress(123, 34, 45, 78));
    TSUNIT_EQUAL(7920, mpe2.sourceUDPPort());
    TSUNIT_EQUAL(4654, mpe2.destinationUDPPort());
    TSUNIT_EQUAL(sizeof(ref), mpe2.udpMessageSize());
    TSUNIT_ASSERT(mpe2.udpMessage() != nullptr);
    TSUNIT_EQUAL(0, ts::MemCompare(mpe2.udpMessage(), ref, mpe2.udpMessageSize()));
}
