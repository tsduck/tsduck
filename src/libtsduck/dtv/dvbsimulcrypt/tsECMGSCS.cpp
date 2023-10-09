//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsECMGSCS.h"
#include "tstlvMessageFactory.h"
#include "tsNamesFile.h"


//----------------------------------------------------------------------------
// Protocol name.
//----------------------------------------------------------------------------

#define PROTOCOL_NAME u"ECMG<=>SCS"

ts::UString ts::ecmgscs::Protocol::name() const
{
    return PROTOCOL_NAME;
}


//----------------------------------------------------------------------------
// Protocol Constructor: Define the syntax of the protocol
//----------------------------------------------------------------------------

ts::ecmgscs::Protocol::Protocol() : tlv::Protocol(ts::ecmgscs::CURRENT_VERSION)
{
    // Define the syntax of all commands:
    // add(cmd_tag, param_tag, min_size, max_size, min_count, max_count)

    add(Tags::channel_setup,  Tags::Super_CAS_id,           4, 4, 1, 1);
    add(Tags::channel_setup,  Tags::ECM_channel_id,         2, 2, 1, 1);

    add(Tags::channel_test,   Tags::ECM_channel_id,         2, 2, 1, 1);

    add(Tags::channel_status, Tags::ECM_channel_id,         2, 2, 1, 1);
    add(Tags::channel_status, Tags::section_TSpkt_flag,     1, 1, 1, 1);
    add(Tags::channel_status, Tags::AC_delay_start,         2, 2, 0, 1);
    add(Tags::channel_status, Tags::AC_delay_stop,          2, 2, 0, 1);
    add(Tags::channel_status, Tags::delay_start,            2, 2, 1, 1);
    add(Tags::channel_status, Tags::delay_stop,             2, 2, 1, 1);
    add(Tags::channel_status, Tags::transition_delay_start, 2, 2, 0, 1);
    add(Tags::channel_status, Tags::transition_delay_stop,  2, 2, 0, 1);
    add(Tags::channel_status, Tags::ECM_rep_period,         2, 2, 1, 1);
    add(Tags::channel_status, Tags::max_streams,            2, 2, 1, 1);
    add(Tags::channel_status, Tags::min_CP_duration,        2, 2, 1, 1);
    add(Tags::channel_status, Tags::lead_CW,                1, 1, 1, 1);
    add(Tags::channel_status, Tags::CW_per_msg,             1, 1, 1, 1);
    add(Tags::channel_status, Tags::max_comp_time,          2, 2, 1, 1);

    add(Tags::channel_close,  Tags::ECM_channel_id,         2, 2, 1, 1);

    add(Tags::channel_error,  Tags::ECM_channel_id,         2, 2, 1, 1);
    add(Tags::channel_error,  Tags::error_status,           2, 2, 1, 0xFFFF);
    add(Tags::channel_error,  Tags::error_information,      2, 2, 0, 0xFFFF);

    add(Tags::stream_setup,   Tags::ECM_channel_id,         2, 2, 1, 1);
    add(Tags::stream_setup,   Tags::ECM_stream_id,          2, 2, 1, 1);
    add(Tags::stream_setup,   Tags::ECM_id,                 2, 2, 1, 1);
    add(Tags::stream_setup,   Tags::nominal_CP_duration,    2, 2, 1, 1);

    add(Tags::stream_test,    Tags::ECM_channel_id,         2, 2, 1, 1);
    add(Tags::stream_test,    Tags::ECM_stream_id,          2, 2, 1, 1);

    add(Tags::stream_status,  Tags::ECM_channel_id,                2, 2, 1, 1);
    add(Tags::stream_status,  Tags::ECM_stream_id,                 2, 2, 1, 1);
    add(Tags::stream_status,  Tags::ECM_id,                        2, 2, 1, 1);
    add(Tags::stream_status,  Tags::access_criteria_transfer_mode, 1, 1, 1, 1);

    add(Tags::stream_close_request,  Tags::ECM_channel_id,  2, 2, 1, 1);
    add(Tags::stream_close_request,  Tags::ECM_stream_id,   2, 2, 1, 1);

    add(Tags::stream_close_response, Tags::ECM_channel_id,  2, 2, 1, 1);
    add(Tags::stream_close_response, Tags::ECM_stream_id,   2, 2, 1, 1);

    add(Tags::stream_error,   Tags::ECM_channel_id,         2, 2, 1, 1);
    add(Tags::stream_error,   Tags::ECM_stream_id,          2, 2, 1, 1);
    add(Tags::stream_error,   Tags::error_status,           2, 2, 1, 0xFFFF);
    add(Tags::stream_error,   Tags::error_information,      2, 2, 0, 0xFFFF);

    add(Tags::CW_provision,   Tags::ECM_channel_id,    2, 2,      1, 1);
    add(Tags::CW_provision,   Tags::ECM_stream_id,     2, 2,      1, 1);
    add(Tags::CW_provision,   Tags::CP_number,         2, 2,      1, 1);
    add(Tags::CW_provision,   Tags::CW_encryption,     0, 0xFFFF, 0, 1);
    add(Tags::CW_provision,   Tags::CP_CW_combination, 2, 0xFFFF, 0, 0xFFFF);
    add(Tags::CW_provision,   Tags::CP_duration,       2, 2,      0, 1);
    add(Tags::CW_provision,   Tags::access_criteria,   0, 0xFFFF, 0, 1);

    add(Tags::ECM_response,   Tags::ECM_channel_id,    2, 2,      1, 1);
    add(Tags::ECM_response,   Tags::ECM_stream_id,     2, 2,      1, 1);
    add(Tags::ECM_response,   Tags::CP_number,         2, 2,      1, 1);
    add(Tags::ECM_response,   Tags::ECM_datagram,      0, 0xFFFF, 1, 1);
}


//----------------------------------------------------------------------------
// Message factory for the protocol
//----------------------------------------------------------------------------

void ts::ecmgscs::Protocol::factory(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    switch (fact.commandTag()) {
        case Tags::channel_setup:
            msg = new ChannelSetup(fact);
            break;
        case Tags::channel_test:
            msg = new ChannelTest(fact);
            break;
        case Tags::channel_status:
            msg = new ChannelStatus(fact);
            break;
        case Tags::channel_close:
            msg = new ChannelClose(fact);
            break;
        case Tags::channel_error:
            msg = new ChannelError(fact);
            break;
        case Tags::stream_setup:
            msg = new StreamSetup(fact);
            break;
        case Tags::stream_test:
            msg = new StreamTest(fact);
            break;
        case Tags::stream_status:
            msg = new StreamStatus(fact);
            break;
        case Tags::stream_close_request:
            msg = new StreamCloseRequest(fact);
            break;
        case Tags::stream_close_response:
            msg = new StreamCloseResponse(fact);
            break;
        case Tags::stream_error:
            msg = new StreamError(fact);
            break;
        case Tags::CW_provision:
            msg = new CWProvision(fact);
            break;
        case Tags::ECM_response:
            msg = new ECMResponse(fact);
            break;
        default:
            throw tlv::DeserializationInternalError(UString::Format(PROTOCOL_NAME u" message 0x%X unimplemented", {fact.commandTag()}));
    }
}


//----------------------------------------------------------------------------
// Return a message for a given protocol error status.
//----------------------------------------------------------------------------

ts::UString ts::ecmgscs::Errors::Name(uint16_t status)
{
    return NameFromDTV(u"EcmgScsErrors", status, NamesFlags::HEXA_FIRST);
}


//----------------------------------------------------------------------------
// Create an error response message for a faulty incoming message.
//----------------------------------------------------------------------------

void ts::ecmgscs::Protocol::buildErrorResponse(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    // Create a channel_error message
    SafePtr<ChannelError> errmsg(new ChannelError(version()));

    // Try to get an ECM_channel_id from the incoming message.
    try {
        errmsg->channel_id = fact.get<uint16_t>(Tags::ECM_channel_id);
    }
    catch (const tlv::DeserializationInternalError&) {
        errmsg->channel_id = 0;
    }

    // Convert general TLV error code into ECMG <=> SCS error_status
    uint16_t status;
    switch (fact.errorStatus()) {
        case tlv::OK: // should not happen
        case tlv::InvalidMessage:
            status = Errors::inv_message;
            break;
        case tlv::UnsupportedVersion:
            status = Errors::inv_proto_version;
            break;
        case tlv::UnknownCommandTag:
            status = Errors::inv_message_type;
            break;
        case tlv::UnknownParameterTag:
            status = Errors::inv_param_type;
            break;
        case tlv::InvalidParameterLength:
            status = Errors::inv_param_length;
            break;
        case tlv::InvalidParameterCount:
        case tlv::MissingParameter:
            status = Errors::missing_param;
            break;
        default:
            status = Errors::unknown_error;
            break;
    }

    // Copy error_status and error_information into response
    errmsg->error_status.push_back(status);
    errmsg->error_information.push_back(fact.errorInformation());

    // Transfer ownership of safe ptr
    msg = errmsg.release();
}


//----------------------------------------------------------------------------
// channel_setup
//----------------------------------------------------------------------------

ts::ecmgscs::ChannelSetup::ChannelSetup(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::ECM_channel_id),
    Super_CAS_id(fact.get<uint32_t>(Tags::Super_CAS_id))
{
}

void ts::ecmgscs::ChannelSetup::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::Super_CAS_id, Super_CAS_id);
}

ts::UString ts::ecmgscs::ChannelSetup::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_setup (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"Super_CAS_id", Super_CAS_id);
}


//----------------------------------------------------------------------------
// channel_test
//----------------------------------------------------------------------------

ts::ecmgscs::ChannelTest::ChannelTest(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::ECM_channel_id)
{
}

void ts::ecmgscs::ChannelTest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
}

ts::UString ts::ecmgscs::ChannelTest::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_test (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id);
}


//----------------------------------------------------------------------------
// channel_status
//----------------------------------------------------------------------------

ts::ecmgscs::ChannelStatus::ChannelStatus(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::ECM_channel_id),
    section_TSpkt_flag(fact.get<bool>(Tags::section_TSpkt_flag)),
    has_AC_delay_start(1 == fact.count(Tags::AC_delay_start)),
    AC_delay_start(!has_AC_delay_start ? 0 : fact.get<int16_t>(Tags::AC_delay_start)),
    has_AC_delay_stop(1 == fact.count(Tags::AC_delay_stop)),
    AC_delay_stop(!has_AC_delay_stop ? 0 : fact.get<int16_t>(Tags::AC_delay_stop)),
    delay_start(fact.get<int16_t>(Tags::delay_start)),
    delay_stop(fact.get<int16_t>(Tags::delay_stop)),
    has_transition_delay_start(1 == fact.count(Tags::transition_delay_start)),
    transition_delay_start(!has_transition_delay_start ? 0 : fact.get<int16_t>(Tags::transition_delay_start)),
    has_transition_delay_stop(1 == fact.count(Tags::transition_delay_stop)),
    transition_delay_stop(!has_transition_delay_stop ? 0 : fact.get<int16_t>(Tags::transition_delay_stop)),
    ECM_rep_period(fact.get<uint16_t>(Tags::ECM_rep_period)),
    max_streams(fact.get<uint16_t>(Tags::max_streams)),
    min_CP_duration(fact.get<uint16_t>(Tags::min_CP_duration)),
    lead_CW(fact.get<uint8_t>(Tags::lead_CW)),
    CW_per_msg(fact.get<uint8_t>(Tags::CW_per_msg)),
    max_comp_time(fact.get<uint16_t>(Tags::max_comp_time))
{
}

void ts::ecmgscs::ChannelStatus::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::section_TSpkt_flag, section_TSpkt_flag);
    if (has_AC_delay_start) {
        fact.put(Tags::AC_delay_start, AC_delay_start);
    }
    if (has_AC_delay_stop) {
        fact.put(Tags::AC_delay_stop, AC_delay_stop);
    }
    fact.put(Tags::delay_start, delay_start);
    fact.put(Tags::delay_stop, delay_stop);
    if (has_transition_delay_start) {
        fact.put(Tags::transition_delay_start, transition_delay_start);
    }
    if (has_transition_delay_stop) {
        fact.put(Tags::transition_delay_stop, transition_delay_stop);
    }
    fact.put(Tags::ECM_rep_period, ECM_rep_period);
    fact.put(Tags::max_streams, max_streams);
    fact.put(Tags::min_CP_duration, min_CP_duration);
    fact.put(Tags::lead_CW, lead_CW);
    fact.put(Tags::CW_per_msg, CW_per_msg);
    fact.put(Tags::max_comp_time, max_comp_time);
}

ts::UString ts::ecmgscs::ChannelStatus::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_status (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpDecimal(indent, u"section_TSpkt_flag", section_TSpkt_flag ? 1 : 0) +
        dumpOptionalDecimal(indent, u"AC_delay_start", has_AC_delay_start, AC_delay_start) +
        dumpOptionalDecimal(indent, u"AC_delay_stop", has_AC_delay_stop, AC_delay_stop) +
        dumpDecimal(indent, u"delay_start", delay_start) +
        dumpDecimal(indent, u"delay_stop", delay_stop) +
        dumpOptionalDecimal(indent, u"transition_delay_start", has_transition_delay_start, transition_delay_start) +
        dumpOptionalDecimal(indent, u"transition_delay_stop", has_transition_delay_stop, transition_delay_stop) +
        dumpDecimal(indent, u"ECM_rep_period", ECM_rep_period) +
        dumpDecimal(indent, u"max_streams", max_streams) +
        dumpDecimal(indent, u"min_CP_duration", min_CP_duration) +
        dumpDecimal(indent, u"lead_CW", lead_CW) +
        dumpDecimal(indent, u"CW_per_msg", CW_per_msg) +
        dumpDecimal(indent, u"max_comp_time", max_comp_time);
}


//----------------------------------------------------------------------------
// channel_close
//----------------------------------------------------------------------------

ts::ecmgscs::ChannelClose::ChannelClose(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::ECM_channel_id)
{
}

void ts::ecmgscs::ChannelClose::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
}

ts::UString ts::ecmgscs::ChannelClose::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_close (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id);
}


//----------------------------------------------------------------------------
// channel_error
//----------------------------------------------------------------------------

ts::ecmgscs::ChannelError::ChannelError(const tlv::MessageFactory& fact) :
    ChannelMessage(fact, Tags::ECM_channel_id),
    error_status(),
    error_information()
{
    fact.get(Tags::error_status, error_status);
    fact.get(Tags::error_information, error_information);
}

void ts::ecmgscs::ChannelError::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::error_status, error_status);
    fact.put(Tags::error_information, error_information);
}

ts::UString ts::ecmgscs::ChannelError::dump(size_t indent) const
{
    return UString::Format(u"%*schannel_error (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpVector(indent, u"error_status", error_status, Errors::Name) +
        dumpVector(indent, u"error_information", error_information);
}


//----------------------------------------------------------------------------
// stream_setup
//----------------------------------------------------------------------------

ts::ecmgscs::StreamSetup::StreamSetup(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id),
    ECM_id(fact.get<uint16_t>(Tags::ECM_id)),
    nominal_CP_duration(fact.get<uint16_t>(Tags::nominal_CP_duration))
{
}

void ts::ecmgscs::StreamSetup::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id, stream_id);
    fact.put(Tags::ECM_id, ECM_id);
    fact.put(Tags::nominal_CP_duration, nominal_CP_duration);
}

ts::UString ts::ecmgscs::StreamSetup::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_setup (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id) +
        dumpHexa(indent, u"ECM_id", ECM_id) +
        dumpDecimal(indent, u"nominal_CP_duration", nominal_CP_duration);
}


//----------------------------------------------------------------------------
// stream_test
//----------------------------------------------------------------------------

ts::ecmgscs::StreamTest::StreamTest(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id)
{
}

void ts::ecmgscs::StreamTest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id, stream_id);
}

ts::UString ts::ecmgscs::StreamTest::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_test(" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_status
//----------------------------------------------------------------------------

ts::ecmgscs::StreamStatus::StreamStatus(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id),
    ECM_id(fact.get<uint16_t>(Tags::ECM_id)),
    access_criteria_transfer_mode(fact.get<bool>(Tags::access_criteria_transfer_mode))
{
}

void ts::ecmgscs::StreamStatus::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id, stream_id);
    fact.put(Tags::ECM_id, ECM_id);
    fact.put(Tags::access_criteria_transfer_mode, access_criteria_transfer_mode);
}

ts::UString ts::ecmgscs::StreamStatus::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_status (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id) +
        dumpHexa(indent, u"ECM_id", ECM_id) +
        dumpDecimal(indent, u"access_criteria_transfer_mode", access_criteria_transfer_mode ? 1 : 0);
}


//----------------------------------------------------------------------------
// stream_close_request
//----------------------------------------------------------------------------

ts::ecmgscs::StreamCloseRequest::StreamCloseRequest(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id)
{
}

void ts::ecmgscs::StreamCloseRequest::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id,  stream_id);
}

ts::UString ts::ecmgscs::StreamCloseRequest::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_close_request (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_close_response
//----------------------------------------------------------------------------

ts::ecmgscs::StreamCloseResponse::StreamCloseResponse(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id)
{
}

void ts::ecmgscs::StreamCloseResponse::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id,  stream_id);
}

ts::UString ts::ecmgscs::StreamCloseResponse::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_close_response (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id);
}


//----------------------------------------------------------------------------
// stream_error
//----------------------------------------------------------------------------

ts::ecmgscs::StreamError::StreamError(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id),
    error_status(),
    error_information()
{
    fact.get(Tags::error_status, error_status);
    fact.get(Tags::error_information, error_information);
}

void ts::ecmgscs::StreamError::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id, stream_id);
    fact.put(Tags::error_status, error_status);
    fact.put(Tags::error_information, error_information);
}

ts::UString ts::ecmgscs::StreamError::dump(size_t indent) const
{
    return UString::Format(u"%*sstream_error (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id) +
        dumpVector(indent, u"error_status", error_status, Errors::Name) +
        dumpVector(indent, u"error_information", error_information);
}


//----------------------------------------------------------------------------
// CW_provision
//----------------------------------------------------------------------------

ts::ecmgscs::CWProvision::CWProvision(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id),
    CP_number(fact.get<uint16_t>(Tags::CP_number)),
    has_CW_encryption(1 == fact.count(Tags::CW_encryption)),
    CW_encryption(),
    CP_CW_combination(),
    has_CP_duration(1 == fact.count(Tags::CP_duration)),
    CP_duration(!has_CP_duration ? 0 : fact.get<uint16_t>(Tags::CP_duration)),
    has_access_criteria(1 == fact.count(Tags::access_criteria)),
    access_criteria()
{
    if (has_CW_encryption) {
        fact.get(Tags::CW_encryption, CW_encryption);
    }
    if (has_access_criteria) {
        fact.get(Tags::access_criteria, access_criteria);
    }
    std::vector<tlv::MessageFactory::Parameter> plist;
    fact.get(Tags::CP_CW_combination, plist);
    CP_CW_combination.resize(plist.size());
    for (size_t i = 0; i < plist.size(); ++i) {
        assert(plist[i].length >= 2); // already enforced in protocol def.
        CP_CW_combination[i].CP = GetUInt16(plist[i].addr);
        CP_CW_combination[i].CW.copy(static_cast<const uint8_t*>(plist[i].addr) + 2, plist[i].length - 2);
    }
}

void ts::ecmgscs::CWProvision::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id,  stream_id);
    fact.put(Tags::CP_number,      CP_number);
    if (has_CW_encryption) {
        fact.put(Tags::CW_encryption, CW_encryption);
    }
    if (has_CP_duration) {
        fact.put(Tags::CP_duration, CP_duration);
    }
    if (has_access_criteria) {
        fact.put(Tags::access_criteria, access_criteria);
    }
    for (const auto& it : CP_CW_combination) {
        tlv::Serializer f(fact);
        f.openTLV(Tags::CP_CW_combination);
        f.put(it.CP);
        f.put(it.CW);
        f.closeTLV();
    }
}

ts::UString ts::ecmgscs::CWProvision::dump(size_t indent) const
{
    UString dump =
        UString::Format(u"%*sCW_provision (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id) +
        dumpDecimal(indent, u"CP_number", CP_number) +
        dumpOptional(indent, u"CW_encryption", has_CW_encryption, CW_encryption, UString::HEXA) +
        dumpOptionalDecimal(indent, u"CP_duration", has_CP_duration, CP_duration) +
        dumpOptional(indent, u"access_criteria", has_access_criteria, access_criteria, UString::HEXA);

    for (const auto& it : CP_CW_combination) {
        dump += dumpDecimal(indent, u"CP", it.CP);
        dump += dumpOptional(indent, u"CW", true, it.CW, UString::SINGLE_LINE);
    }

    return dump;
}


//----------------------------------------------------------------------------
// ECM_response
//----------------------------------------------------------------------------

ts::ecmgscs::ECMResponse::ECMResponse(const tlv::MessageFactory& fact) :
    StreamMessage(fact, Tags::ECM_channel_id, Tags::ECM_stream_id),
    CP_number(fact.get<uint16_t>(Tags::CP_number)),
    ECM_datagram()
{
    fact.get(Tags::ECM_datagram, ECM_datagram);
}

void ts::ecmgscs::ECMResponse::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::ECM_channel_id, channel_id);
    fact.put(Tags::ECM_stream_id, stream_id);
    fact.put(Tags::CP_number, CP_number);
    fact.put(Tags::ECM_datagram, ECM_datagram);
}

ts::UString ts::ecmgscs::ECMResponse::dump(size_t indent) const
{
    return UString::Format(u"%*sECM_response (" PROTOCOL_NAME u")\n", {indent, u"   "}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"ECM_channel_id", channel_id) +
        dumpHexa(indent, u"ECM_stream_id", stream_id) +
        dumpDecimal(indent, u"CP_number", CP_number) +
        dumpOptional(indent, u"ECM_datagram", true, ECM_datagram, UString::HEXA);
}
