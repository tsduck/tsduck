//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for tlv namespace.
//
//----------------------------------------------------------------------------

#include "tsECMGSCS.h"
#include "tsEMMGMUX.h"
#include "tstlvMessageFactory.h"
#include "tsunit.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class TagLengthValueTest: public tsunit::Test
{
    TSUNIT_DECLARE_TEST(ECMG);
    TSUNIT_DECLARE_TEST(EMMG);
    TSUNIT_DECLARE_TEST(ECMGError);
    TSUNIT_DECLARE_TEST(EMMGError);
};

TSUNIT_REGISTER(TagLengthValueTest);


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

TSUNIT_DEFINE_TEST(ECMG)
{
    ts::ecmgscs::Protocol protocol;
    ts::ecmgscs::ChannelStatus refMessage(protocol);
    refMessage.channel_id = 2;
    refMessage.section_TSpkt_flag = true;
    refMessage.has_AC_delay_start = true;
    refMessage.AC_delay_start = -200;
    refMessage.has_AC_delay_stop = true;
    refMessage.AC_delay_stop = -200;
    refMessage.delay_start = -300;
    refMessage.delay_stop = 100;
    refMessage.has_transition_delay_start = true;
    refMessage.transition_delay_start = -500;
    refMessage.has_transition_delay_stop = true;
    refMessage.transition_delay_stop = 100;
    refMessage.ECM_rep_period = 100;
    refMessage.max_streams = 2;
    refMessage.min_CP_duration = 10;
    refMessage.lead_CW = 1;
    refMessage.CW_per_msg = 2;
    refMessage.max_comp_time = 500;

    static uint8_t refData[] = {
        0x03,
        0x00, 0x03, 0x00, 0x51, // channel_status, 0x51 bytes
        0x00, 0x0E, 0x00, 0x02, 0x00, 0x02,
        0x00, 0x02, 0x00, 0x01, 0x01,
        0x00, 0x16, 0x00, 0x02, 0xFF, 0x38,
        0x00, 0x17, 0x00, 0x02, 0xFF, 0x38,
        0x00, 0x03, 0x00, 0x02, 0xFE, 0xD4,
        0x00, 0x04, 0x00, 0x02, 0x00, 0x64,
        0x00, 0x05, 0x00, 0x02, 0xFE, 0x0C,
        0x00, 0x06, 0x00, 0x02, 0x00, 0x64,
        0x00, 0x07, 0x00, 0x02, 0x00, 0x64,
        0x00, 0x08, 0x00, 0x02, 0x00, 0x02,
        0x00, 0x09, 0x00, 0x02, 0x00, 0x0A,
        0x00, 0x0A, 0x00, 0x01, 0x01,
        0x00, 0x0B, 0x00, 0x01, 0x02,
        0x00, 0x0C, 0x00, 0x02, 0x01, 0xF4
   };

    static const ts::UChar refString[] =
        u"  channel_status (ECMG<=>SCS)\n"
        u"  protocol_version = 0x03\n"
        u"  message_type = 0x0003\n"
        u"  ECM_channel_id = 0x0002\n"
        u"  section_TSpkt_flag = 1\n"
        u"  AC_delay_start = -200\n"
        u"  AC_delay_stop = -200\n"
        u"  delay_start = -300\n"
        u"  delay_stop = 100\n"
        u"  transition_delay_start = -500\n"
        u"  transition_delay_stop = 100\n"
        u"  ECM_rep_period = 100\n"
        u"  max_streams = 2\n"
        u"  min_CP_duration = 10\n"
        u"  lead_CW = 1\n"
        u"  CW_per_msg = 2\n"
        u"  max_comp_time = 500\n";

    ts::ByteBlockPtr data(new ts::ByteBlock);
    ts::tlv::Serializer zer(data);
    refMessage.serialize(zer);

    debug() << "TagLengthValueTest::testECMG: serialized:" << std::endl
                 << ts::UString::Dump(*data, ts::UString::HEXA, 2) << std::endl;

    TSUNIT_EQUAL(sizeof(refData), data->size());
    TSUNIT_EQUAL(0, ts::MemCompare(refData, data->data(), sizeof(data)));

    ts::tlv::MessageFactory fac(refData, sizeof(refData), protocol);
    ts::tlv::MessagePtr msg(fac.factory());
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(ts::ecmgscs::Tags::channel_status, msg->tag());
    ts::ecmgscs::ChannelStatus* ptr = dynamic_cast<ts::ecmgscs::ChannelStatus*>(msg.get());
    TSUNIT_ASSERT(ptr != nullptr);
    TSUNIT_EQUAL(refMessage.channel_id, ptr->channel_id);
    TSUNIT_EQUAL(refMessage.section_TSpkt_flag, ptr->section_TSpkt_flag);
    TSUNIT_EQUAL(refMessage.has_AC_delay_start, ptr->has_AC_delay_start);
    TSUNIT_EQUAL(refMessage.AC_delay_start, ptr->AC_delay_start);
    TSUNIT_EQUAL(refMessage.has_AC_delay_stop, ptr->has_AC_delay_stop);
    TSUNIT_EQUAL(refMessage.AC_delay_stop, ptr->AC_delay_stop);
    TSUNIT_EQUAL(refMessage.delay_start, ptr->delay_start);
    TSUNIT_EQUAL(refMessage.delay_stop, ptr->delay_stop);
    TSUNIT_EQUAL(refMessage.has_transition_delay_start, ptr->has_transition_delay_start);
    TSUNIT_EQUAL(refMessage.transition_delay_start, ptr->transition_delay_start);
    TSUNIT_EQUAL(refMessage.has_transition_delay_stop, ptr->has_transition_delay_stop);
    TSUNIT_EQUAL(refMessage.transition_delay_stop, ptr->transition_delay_stop);
    TSUNIT_EQUAL(refMessage.ECM_rep_period, ptr->ECM_rep_period);
    TSUNIT_EQUAL(refMessage.max_streams, ptr->max_streams);
    TSUNIT_EQUAL(refMessage.min_CP_duration, ptr->min_CP_duration);
    TSUNIT_EQUAL(refMessage.lead_CW, ptr->lead_CW);
    TSUNIT_EQUAL(refMessage.CW_per_msg, ptr->CW_per_msg);
    TSUNIT_EQUAL(refMessage.max_comp_time, ptr->max_comp_time);

    const ts::UString str(refMessage.dump(2));
    debug() << "TagLengthValueTest::testECMG: dump" << std::endl << str << std::endl;
    TSUNIT_EQUAL(refString, str);
}

TSUNIT_DEFINE_TEST(EMMG)
{
    ts::emmgmux::Protocol protocol;
    ts::emmgmux::StreamBWAllocation refMessage(protocol);
    refMessage.channel_id = 0x1234;
    refMessage.stream_id = 0x5678;
    refMessage.client_id = 0x98765432;
    refMessage.has_bandwidth = true;
    refMessage.bandwidth = 200;

    static uint8_t refData[] = {
        0x03,
        0x01, 0x18, 0x00, 0x1A,
        0x00, 0x03, 0x00, 0x02, 0x12, 0x34,
        0x00, 0x04, 0x00, 0x02, 0x56, 0x78,
        0x00, 0x01, 0x00, 0x04, 0x98, 0x76, 0x54, 0x32,
        0x00, 0x06, 0x00, 0x02, 0x00, 0xC8,
    };

    static const ts::UChar refString[] =
        u"  stream_BW_allocation (EMMG/PDG<=>MUX)\n"
        u"  protocol_version = 0x03\n"
        u"  message_type = 0x0118\n"
        u"  client_id = 0x98765432\n"
        u"  data_channel_id = 0x1234\n"
        u"  data_stream_id = 0x5678\n"
        u"  bandwidth = 200\n";

    ts::ByteBlockPtr data(new ts::ByteBlock);
    ts::tlv::Serializer zer(data);
    refMessage.serialize(zer);

    debug() << "TagLengthValueTest::testEMMG: serialized:" << std::endl
            << ts::UString::Dump(*data, ts::UString::HEXA, 2) << std::endl;

    TSUNIT_EQUAL(sizeof(refData), data->size());
    TSUNIT_EQUAL(0, ts::MemCompare(refData, data->data(), sizeof(data)));

    ts::tlv::MessageFactory fac(refData, sizeof(refData), protocol);
    ts::tlv::MessagePtr msg(fac.factory());
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(ts::emmgmux::Tags::stream_BW_allocation, msg->tag());
    ts::emmgmux::StreamBWAllocation* ptr = dynamic_cast<ts::emmgmux::StreamBWAllocation*>(msg.get());
    TSUNIT_ASSERT(ptr != nullptr);
    TSUNIT_EQUAL(refMessage.channel_id, ptr->channel_id);
    TSUNIT_EQUAL(refMessage.stream_id, ptr->stream_id);
    TSUNIT_EQUAL(refMessage.client_id, ptr->client_id);
    TSUNIT_EQUAL(refMessage.has_bandwidth, ptr->has_bandwidth);
    TSUNIT_EQUAL(refMessage.bandwidth, ptr->bandwidth);

    const ts::UString str(refMessage.dump(2));
    debug() << "TagLengthValueTest::testEMMG: dump" << std::endl << str << std::endl;
    TSUNIT_EQUAL(refString, str);
}

TSUNIT_DEFINE_TEST(ECMGError)
{
    ts::ecmgscs::Protocol protocol;
    ts::ecmgscs::StreamError refMessage(protocol);
    refMessage.channel_id = 2;
    refMessage.stream_id = 3;
    refMessage.error_status = {ts::ecmgscs::Errors::inv_ECM_id, ts::ecmgscs::Errors::out_of_compute};
    refMessage.error_information = {0x1234};

    static uint8_t refData[] = {
        0x03,
        0x01, 0x06, 0x00, 0x1E,
        0x00, 0x0E, 0x00, 0x02, 0x00, 0x02,
        0x00, 0x0F, 0x00, 0x02, 0x00, 0x03,
        0x70, 0x00, 0x00, 0x02, 0x00, 0x12,
        0x70, 0x00, 0x00, 0x02, 0x00, 0x0D,
        0x70, 0x01, 0x00, 0x02, 0x12, 0x34,
   };

    static const ts::UChar refString[] =
        u"  stream_error (ECMG<=>SCS)\n"
        u"  protocol_version = 0x03\n"
        u"  message_type = 0x0106\n"
        u"  ECM_channel_id = 0x0002\n"
        u"  ECM_stream_id = 0x0003\n"
        u"  error_status = 0x0012 (unknown ECM_id value)\n"
        u"  error_status = 0x000D (ECMG out of computational resources)\n"
        u"  error_information = 0x1234\n";

    ts::ByteBlockPtr data(new ts::ByteBlock);
    ts::tlv::Serializer zer(data);
    refMessage.serialize(zer);

    debug() << "TagLengthValueTest::testECMGError: serialized:" << std::endl
                 << ts::UString::Dump(*data, ts::UString::HEXA, 2) << std::endl;

    TSUNIT_EQUAL(sizeof(refData), data->size());
    TSUNIT_EQUAL(0, ts::MemCompare(refData, data->data(), sizeof(data)));

    ts::tlv::MessageFactory fac(refData, sizeof(refData), protocol);
    ts::tlv::MessagePtr msg(fac.factory());
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(ts::ecmgscs::Tags::stream_error, msg->tag());
    ts::ecmgscs::StreamError* ptr = dynamic_cast<ts::ecmgscs::StreamError*>(msg.get());
    TSUNIT_ASSERT(ptr != nullptr);
    TSUNIT_EQUAL(refMessage.channel_id, ptr->channel_id);
    TSUNIT_EQUAL(refMessage.stream_id, ptr->stream_id);
    TSUNIT_ASSERT(refMessage.error_status == ptr->error_status);
    TSUNIT_ASSERT(refMessage.error_information == ptr->error_information);

    const ts::UString str(refMessage.dump(2));
    debug() << "TagLengthValueTest::testECMGError: dump" << std::endl << str << std::endl;
    TSUNIT_EQUAL(refString, str);
}

TSUNIT_DEFINE_TEST(EMMGError)
{
    ts::emmgmux::Protocol protocol;
    ts::emmgmux::StreamError refMessage(protocol);
    refMessage.channel_id = 2;
    refMessage.stream_id = 3;
    refMessage.client_id = 4;
    refMessage.error_status = {ts::emmgmux::Errors::exceeded_bw, ts::emmgmux::Errors::client_id_in_use};
    refMessage.error_information = {0x1234};

    static uint8_t refData[] = {
        0x03,
        0x01, 0x16, 0x00, 0x26,
        0x00, 0x03, 0x00, 0x02, 0x00, 0x02,
        0x00, 0x04, 0x00, 0x02, 0x00, 0x03,
        0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
        0x70, 0x00, 0x00, 0x02, 0x00, 0x0F,
        0x70, 0x00, 0x00, 0x02, 0x00, 0x14,
        0x70, 0x01, 0x00, 0x02, 0x12, 0x34,
    };

    static const ts::UChar refString[] =
        u"  stream_error (EMMG/PDG<=>MUX)\n"
        u"  protocol_version = 0x03\n"
        u"  message_type = 0x0116\n"
        u"  client_id = 0x00000004\n"
        u"  data_channel_id = 0x0002\n"
        u"  data_stream_id = 0x0003\n"
        u"  error_status = 0x000F (exceeded bandwidth)\n"
        u"  error_status = 0x0014 (client_id value already in use)\n"
        u"  error_information = 0x1234\n";

    ts::ByteBlockPtr data(new ts::ByteBlock);
    ts::tlv::Serializer zer(data);
    refMessage.serialize(zer);

    debug() << "TagLengthValueTest::testEMMGError: serialized:" << std::endl
            << ts::UString::Dump(*data, ts::UString::HEXA, 2) << std::endl;

    TSUNIT_EQUAL(sizeof(refData), data->size());
    TSUNIT_EQUAL(0, ts::MemCompare(refData, data->data(), sizeof(data)));

    ts::tlv::MessageFactory fac(refData, sizeof(refData), protocol);
    ts::tlv::MessagePtr msg(fac.factory());
    TSUNIT_ASSERT(msg != nullptr);
    TSUNIT_EQUAL(ts::tlv::TAG(ts::emmgmux::Tags::stream_error), msg->tag());
    ts::emmgmux::StreamError* ptr = dynamic_cast<ts::emmgmux::StreamError*>(msg.get());
    TSUNIT_ASSERT(ptr != nullptr);
    TSUNIT_EQUAL(refMessage.channel_id, ptr->channel_id);
    TSUNIT_EQUAL(refMessage.stream_id, ptr->stream_id);
    TSUNIT_ASSERT(refMessage.error_status == ptr->error_status);
    TSUNIT_ASSERT(refMessage.error_information == ptr->error_information);

    const ts::UString str(refMessage.dump(2));
    debug() << "TagLengthValueTest::testEMMGError: dump" << std::endl << str << std::endl;
    TSUNIT_EQUAL(refString, str);
}
