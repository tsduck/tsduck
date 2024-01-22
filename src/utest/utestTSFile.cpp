//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for TSFile.
//
//----------------------------------------------------------------------------

#include "tsTSFile.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsCerrReport.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSFileTest: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testTS();
    void testM2TS();
    void testDuck();
    void testStuffingRead();
    void testStuffingWrite();

    TSUNIT_TEST_BEGIN(TSFileTest);
    TSUNIT_TEST(testTS);
    TSUNIT_TEST(testM2TS);
    TSUNIT_TEST(testDuck);
    TSUNIT_TEST(testStuffingRead);
    TSUNIT_TEST(testStuffingWrite);
    TSUNIT_TEST_END();

private:
    fs::path _tempFileName {};
};

TSUNIT_REGISTER(TSFileTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void TSFileTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".ts");
    }
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}

// Test suite cleanup method.
void TSFileTest::afterTest()
{
    fs::remove(_tempFileName, &ts::ErrCodeReport());
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSFileTest::testTS()
{
    ts::TSFile file;
    ts::TSPacketVector packets(100);

    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(!file.isOpen());
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::READ | ts::TSFile::WRITE, CERR));
    TSUNIT_ASSERT(file.isOpen());

    for (size_t i = 0; i < packets.size(); ++i) {
        packets[i] = ts::NullPacket;
        packets[i].setPID(ts::PID(100 + i));
    }
    TSUNIT_ASSERT(file.writePackets(packets.data(), nullptr, packets.size(), CERR));
    TSUNIT_ASSERT(file.rewind(CERR));
    TSUNIT_EQUAL(ts::TSPacketFormat::TS, file.packetFormat());
    TSUNIT_EQUAL(u"TS", file.packetFormatString());

    ts::TSPacketVector inpackets(packets.size() / 5);
    ts::PID pid = 100;
    for (size_t i1 = 0; i1 < 5; ++i1) {
        TSUNIT_EQUAL(inpackets.size(), file.readPackets(&inpackets[0], nullptr, inpackets.size(), CERR));
        for (size_t i2 = 0; i2 < inpackets.size(); ++i2) {
            TSUNIT_EQUAL(pid++, inpackets[i2].getPID());
        }
    }
    TSUNIT_EQUAL(0, file.readPackets(&inpackets[0], nullptr, inpackets.size(), CERR));
    TSUNIT_EQUAL(100, file.writePacketsCount());
    TSUNIT_EQUAL(100, file.readPacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(18800, fs::file_size(_tempFileName, &ts::ErrCodeReport(CERR)));
    TSUNIT_ASSERT(!file.isOpen());
}

void TSFileTest::testM2TS()
{
    ts::TSFile file;
    ts::TSPacket packet;
    ts::TSPacketMetadata mdata;

    debug() << "TSFileTest::testM2TS: TS file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR, ts::TSPacketFormat::M2TS));

    packet = ts::NullPacket;
    for (size_t i = 0; i < 5; ++i) {
        packet.setPID(ts::PID(200 + i));
        mdata.setInputTimeStamp(ts::pcr_units(2 * i), ts::TimeSource::UNDEFINED);
        TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));
    }
    TSUNIT_EQUAL(5, file.writePacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(960, fs::file_size(_tempFileName, &ts::ErrCodeReport(CERR)));

    TSUNIT_ASSERT(!file.isOpen());
    TSUNIT_ASSERT(file.openRead(_tempFileName, 2 * (4 + ts::PKT_SIZE), CERR));
    TSUNIT_ASSERT(file.isOpen());
    TSUNIT_EQUAL(ts::TSPacketFormat::AUTODETECT, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(202, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(4, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::M2TS, mdata.getInputTimeSource());
    TSUNIT_EQUAL(ts::TSPacketFormat::M2TS, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(203, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(6, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::M2TS, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(204, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(8, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::M2TS, mdata.getInputTimeSource());

    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));

    TSUNIT_EQUAL(3, file.readPacketsCount());
    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_ASSERT(file.close(CERR));
}

void TSFileTest::testDuck()
{
    ts::TSFile file;
    ts::TSPacket packet;
    ts::TSPacketMetadata mdata;

    debug() << "TSFileTest::testDuck: TS file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR, ts::TSPacketFormat::DUCK));

    packet = ts::NullPacket;
    packet.setPID(ts::PID(300));
    mdata.setLabel(1);
    mdata.setLabel(3);
    mdata.setInputTimeStamp(ts::pcr_units(0x212345678), ts::TimeSource::KERNEL);
    TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));

    packet.setPID(ts::PID(400));
    mdata.reset();
    mdata.setLabel(2);
    mdata.setLabel(4);
    mdata.setInputTimeStamp(ts::pcr_units(0x223456789), ts::TimeSource::PCR);
    TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));

    TSUNIT_EQUAL(2, file.writePacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(404, fs::file_size(_tempFileName, &ts::ErrCodeReport(CERR)));  // 2 packets, header size = 14 bytes

    TSUNIT_ASSERT(!file.isOpen());
    TSUNIT_ASSERT(file.openRead(_tempFileName, 2, 0, CERR));
    TSUNIT_ASSERT(file.isOpen());
    TSUNIT_EQUAL(ts::TSPacketFormat::AUTODETECT, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(300, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(mdata.hasLabel(1));
    TSUNIT_ASSERT(!mdata.hasLabel(2));
    TSUNIT_ASSERT(mdata.hasLabel(3));
    TSUNIT_ASSERT(!mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(0x212345678, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::KERNEL, mdata.getInputTimeSource());
    TSUNIT_EQUAL(ts::TSPacketFormat::DUCK, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(400, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(!mdata.hasLabel(1));
    TSUNIT_ASSERT(mdata.hasLabel(2));
    TSUNIT_ASSERT(!mdata.hasLabel(3));
    TSUNIT_ASSERT(mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(0x223456789, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::PCR, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(300, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(mdata.hasLabel(1));
    TSUNIT_ASSERT(!mdata.hasLabel(2));
    TSUNIT_ASSERT(mdata.hasLabel(3));
    TSUNIT_ASSERT(!mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(0x212345678, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::KERNEL, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(400, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(!mdata.hasLabel(1));
    TSUNIT_ASSERT(mdata.hasLabel(2));
    TSUNIT_ASSERT(!mdata.hasLabel(3));
    TSUNIT_ASSERT(mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(0x223456789, mdata.getInputTimeStamp().count());
    TSUNIT_EQUAL(ts::TimeSource::PCR, mdata.getInputTimeSource());

    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));

    TSUNIT_EQUAL(4, file.readPacketsCount());
    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_ASSERT(file.close(CERR));
}

void TSFileTest::testStuffingRead()
{
    ts::TSFile file;
    ts::TSPacket pkt;
    ts::TSPacketVector packets(20);

    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(!file.isOpen());

    // Create a file with one packet.
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR));
    TSUNIT_ASSERT(file.isOpen());
    pkt.init(100, 0, 0xAB);
    TSUNIT_ASSERT(file.writePackets(&pkt, nullptr, 1, CERR));
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_EQUAL(1, file.writePacketsCount());
    TSUNIT_EQUAL(0, file.readPacketsCount());
    TSUNIT_ASSERT(!file.isOpen());

    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(188, fs::file_size(_tempFileName, &ts::ErrCodeReport(CERR)));

    // Read it with artificial stuffing.
    file.setStuffing(2, 3);
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::READ, CERR));
    TSUNIT_ASSERT(file.isOpen());
    TSUNIT_EQUAL(6, file.readPackets(&packets[0], nullptr, packets.size(), CERR));
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_EQUAL(0, file.writePacketsCount());
    TSUNIT_EQUAL(6, file.readPacketsCount());
    TSUNIT_ASSERT(!file.isOpen());

    TSUNIT_EQUAL(ts::PID_NULL, packets[0].getPID());
    TSUNIT_EQUAL(184, packets[0].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[0].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[1].getPID());
    TSUNIT_EQUAL(184, packets[1].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[1].getPayload()[0]);

    TSUNIT_EQUAL(100, packets[2].getPID());
    TSUNIT_EQUAL(184, packets[2].getPayloadSize());
    TSUNIT_EQUAL(0xAB, packets[2].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[3].getPID());
    TSUNIT_EQUAL(184, packets[3].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[3].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[4].getPID());
    TSUNIT_EQUAL(184, packets[4].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[4].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[5].getPID());
    TSUNIT_EQUAL(184, packets[5].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[5].getPayload()[0]);
}

void TSFileTest::testStuffingWrite()
{
    ts::TSFile file;
    ts::TSPacket pkt;
    ts::TSPacketVector packets(20);

    TSUNIT_ASSERT(!fs::exists(_tempFileName));
    TSUNIT_ASSERT(!file.isOpen());

    // Create a file with one packet plus stuffing.
    file.setStuffing(3, 2);
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR));
    TSUNIT_ASSERT(file.isOpen());
    TSUNIT_EQUAL(3, file.writePacketsCount());
    TSUNIT_EQUAL(0, file.readPacketsCount());
    pkt.init(200, 0, 0xCD);
    TSUNIT_ASSERT(file.writePackets(&pkt, nullptr, 1, CERR));
    TSUNIT_EQUAL(4, file.writePacketsCount());
    TSUNIT_EQUAL(0, file.readPacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_EQUAL(6, file.writePacketsCount());
    TSUNIT_EQUAL(0, file.readPacketsCount());
    TSUNIT_ASSERT(!file.isOpen());

    TSUNIT_ASSERT(fs::exists(_tempFileName));
    TSUNIT_EQUAL(6 * 188, fs::file_size(_tempFileName, &ts::ErrCodeReport(CERR)));

    // Read it without artificial stuffing.
    ts::TSFile file2;
    TSUNIT_ASSERT(file2.open(_tempFileName, ts::TSFile::READ, CERR));
    TSUNIT_ASSERT(file2.isOpen());
    TSUNIT_EQUAL(6, file2.readPackets(&packets[0], nullptr, packets.size(), CERR));
    TSUNIT_ASSERT(file2.close(CERR));
    TSUNIT_EQUAL(0, file2.writePacketsCount());
    TSUNIT_EQUAL(6, file2.readPacketsCount());
    TSUNIT_ASSERT(!file2.isOpen());

    TSUNIT_EQUAL(ts::PID_NULL, packets[0].getPID());
    TSUNIT_EQUAL(184, packets[0].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[0].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[1].getPID());
    TSUNIT_EQUAL(184, packets[1].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[1].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[2].getPID());
    TSUNIT_EQUAL(184, packets[2].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[2].getPayload()[0]);

    TSUNIT_EQUAL(200, packets[3].getPID());
    TSUNIT_EQUAL(184, packets[3].getPayloadSize());
    TSUNIT_EQUAL(0xCD, packets[3].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[4].getPID());
    TSUNIT_EQUAL(184, packets[4].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[4].getPayload()[0]);

    TSUNIT_EQUAL(ts::PID_NULL, packets[5].getPID());
    TSUNIT_EQUAL(184, packets[5].getPayloadSize());
    TSUNIT_EQUAL(0xFF, packets[5].getPayload()[0]);
}
