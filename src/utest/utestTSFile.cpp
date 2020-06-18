//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  TSUnit test suite for TSFile.
//
//----------------------------------------------------------------------------

#include "tsTSFile.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsCerrReport.h"
#include "tsSysUtils.h"
#include "tsunit.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TSFileTest: public tsunit::Test
{
public:
    TSFileTest();

    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testTS();
    void testM2TS();
    void testDuck();

    TSUNIT_TEST_BEGIN(TSFileTest);
    TSUNIT_TEST(testTS);
    TSUNIT_TEST(testM2TS);
    TSUNIT_TEST(testDuck);
    TSUNIT_TEST_END();

private:
    ts::UString _tempFileName;
};

TSUNIT_REGISTER(TSFileTest);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Constructor.
TSFileTest::TSFileTest() :
    _tempFileName()
{
}

// Test suite initialization method.
void TSFileTest::beforeTest()
{
    if (_tempFileName.empty()) {
        _tempFileName = ts::TempFile(u".ts");
    }
    ts::DeleteFile(_tempFileName);
}

// Test suite cleanup method.
void TSFileTest::afterTest()
{
    ts::DeleteFile(_tempFileName);
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

void TSFileTest::testTS()
{
    ts::TSFile file;
    ts::TSPacketVector packets(100);

    TSUNIT_ASSERT(!ts::FileExists(_tempFileName));
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
    TSUNIT_ASSERT(ts::FileExists(_tempFileName));
    TSUNIT_EQUAL(18800, ts::GetFileSize(_tempFileName));
    TSUNIT_ASSERT(!file.isOpen());
}

void TSFileTest::testM2TS()
{
    ts::TSFile file;
    ts::TSPacket packet;
    ts::TSPacketMetadata mdata;

    debug() << "TSFileTest::testM2TS: TS file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(!ts::FileExists(_tempFileName));
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR, ts::TSPacketFormat::M2TS));

    packet = ts::NullPacket;
    for (size_t i = 0; i < 5; ++i) {
        packet.setPID(ts::PID(200 + i));
        mdata.setInputTimeStamp(i, ts::SYSTEM_CLOCK_FREQ / 2, ts::TimeSource::UNDEFINED);
        TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));
    }
    TSUNIT_EQUAL(5, file.writePacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_ASSERT(ts::FileExists(_tempFileName));
    TSUNIT_EQUAL(960, ts::GetFileSize(_tempFileName));

    TSUNIT_ASSERT(!file.isOpen());
    TSUNIT_ASSERT(file.openRead(_tempFileName, 2 * (4 + ts::PKT_SIZE), CERR));
    TSUNIT_ASSERT(file.isOpen());
    TSUNIT_EQUAL(ts::TSPacketFormat::AUTODETECT, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(202, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(4, mdata.getInputTimeStamp());
    TSUNIT_EQUAL(ts::TimeSource::M2TS, mdata.getInputTimeSource());
    TSUNIT_EQUAL(ts::TSPacketFormat::M2TS, file.packetFormat());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(203, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(6, mdata.getInputTimeStamp());
    TSUNIT_EQUAL(ts::TimeSource::M2TS, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(204, packet.getPID());
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(8, mdata.getInputTimeStamp());
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

    debug() << "TSFileTest::testM2TS: TS file: " << _tempFileName << std::endl;
    TSUNIT_ASSERT(!ts::FileExists(_tempFileName));
    TSUNIT_ASSERT(file.open(_tempFileName, ts::TSFile::WRITE, CERR, ts::TSPacketFormat::DUCK));

    packet = ts::NullPacket;
    packet.setPID(ts::PID(300));
    mdata.setLabel(1);
    mdata.setLabel(3);
    mdata.setInputTimeStamp(TS_UCONST64(0x212345678), ts::SYSTEM_CLOCK_FREQ, ts::TimeSource::KERNEL);
    TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));

    packet.setPID(ts::PID(400));
    mdata.reset();
    mdata.setLabel(2);
    mdata.setLabel(4);
    mdata.setInputTimeStamp(TS_UCONST64(0x223456789), ts::SYSTEM_CLOCK_FREQ, ts::TimeSource::PCR);
    TSUNIT_ASSERT(file.writePackets(&packet, &mdata, 1, CERR));

    TSUNIT_EQUAL(2, file.writePacketsCount());
    TSUNIT_ASSERT(file.close(CERR));
    TSUNIT_ASSERT(ts::FileExists(_tempFileName));
    TSUNIT_EQUAL(404, ts::GetFileSize(_tempFileName)); // 2 packets, header size = 14 bytes

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
    TSUNIT_EQUAL(TS_UCONST64(0x212345678), mdata.getInputTimeStamp());
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
    TSUNIT_EQUAL(TS_UCONST64(0x223456789), mdata.getInputTimeStamp());
    TSUNIT_EQUAL(ts::TimeSource::PCR, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(300, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(mdata.hasLabel(1));
    TSUNIT_ASSERT(!mdata.hasLabel(2));
    TSUNIT_ASSERT(mdata.hasLabel(3));
    TSUNIT_ASSERT(!mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(TS_UCONST64(0x212345678), mdata.getInputTimeStamp());
    TSUNIT_EQUAL(ts::TimeSource::KERNEL, mdata.getInputTimeSource());

    TSUNIT_EQUAL(1, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_EQUAL(400, packet.getPID());
    TSUNIT_ASSERT(!mdata.hasLabel(0));
    TSUNIT_ASSERT(!mdata.hasLabel(1));
    TSUNIT_ASSERT(mdata.hasLabel(2));
    TSUNIT_ASSERT(!mdata.hasLabel(3));
    TSUNIT_ASSERT(mdata.hasLabel(4));
    TSUNIT_ASSERT(mdata.hasInputTimeStamp());
    TSUNIT_EQUAL(TS_UCONST64(0x223456789), mdata.getInputTimeStamp());
    TSUNIT_EQUAL(ts::TimeSource::PCR, mdata.getInputTimeSource());

    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));

    TSUNIT_EQUAL(4, file.readPacketsCount());
    TSUNIT_EQUAL(0, file.readPackets(&packet, &mdata, 1, CERR));
    TSUNIT_ASSERT(file.close(CERR));
}
