//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDuckProtocol.h"
#include "tstlvMessageFactory.h"
#include "tsSection.h"


//----------------------------------------------------------------------------
// Protocol name.
//----------------------------------------------------------------------------

#define PROTOCOL_NAME u"TSDuck"

ts::UString ts::duck::Protocol::name() const
{
    return PROTOCOL_NAME;
}


//----------------------------------------------------------------------------
// Protocol Constructor: Define the syntax of the protocol
//----------------------------------------------------------------------------

ts::duck::Protocol::Protocol() : tlv::Protocol(ts::duck::CURRENT_VERSION)
{
    // Define the syntax of all commands:
    // add (cmd_tag, param_tag, min_size, max_size, min_count, max_count)

    add(Tags::MSG_LOG_SECTION, Tags::PRM_PID,       2, 2, 0, 1);
    add(Tags::MSG_LOG_SECTION, Tags::PRM_TIMESTAMP, 8, 8, 0, 1);
    add(Tags::MSG_LOG_SECTION, Tags::PRM_SECTION,   MIN_SHORT_SECTION_SIZE, MAX_PRIVATE_SECTION_SIZE, 1, 1);

    add(Tags::MSG_LOG_TABLE, Tags::PRM_PID,       2, 2, 0, 1);
    add(Tags::MSG_LOG_TABLE, Tags::PRM_TIMESTAMP, 8, 8, 0, 1);
    add(Tags::MSG_LOG_TABLE, Tags::PRM_SECTION,   MIN_SHORT_SECTION_SIZE, MAX_PRIVATE_SECTION_SIZE, 1, 256);

    add(Tags::MSG_ECM, Tags::PRM_CW_EVEN,         0, 0xFFFF, 0, 1);
    add(Tags::MSG_ECM, Tags::PRM_CW_ODD,          0, 0xFFFF, 0, 1);
    add(Tags::MSG_ECM, Tags::PRM_ACCESS_CRITERIA, 0, 0xFFFF, 0, 1);
}


//----------------------------------------------------------------------------
// Message factory for the protocol
//----------------------------------------------------------------------------

void ts::duck::Protocol::factory(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    switch (fact.commandTag()) {
        case Tags::MSG_LOG_SECTION:
            msg = new LogSection(fact);
            break;
        case Tags::MSG_LOG_TABLE:
            msg = new LogTable(fact);
            break;
        case Tags::MSG_ECM:
            msg = new ClearECM(fact);
            break;
        default:
            throw tlv::DeserializationInternalError(UString::Format(PROTOCOL_NAME u" message 0x%X unimplemented", {fact.commandTag()}));
    }
}


//----------------------------------------------------------------------------
// Create an error response message for a faulty incoming message.
//----------------------------------------------------------------------------

void ts::duck::Protocol::buildErrorResponse(const tlv::MessageFactory& fact, tlv::MessagePtr& msg) const
{
    // Create an error message
    SafePtr<Error> errmsg(new Error(version()));

    // Convert general TLV error code into protocol error_status
    switch (fact.errorStatus()) {
        case tlv::OK: // should not happen
        case tlv::InvalidMessage:
            errmsg->error_status = Errors::inv_message;
            break;
        case tlv::UnsupportedVersion:
            errmsg->error_status = Errors::inv_proto_version;
            break;
        case tlv::UnknownCommandTag:
            errmsg->error_status = Errors::inv_message_type;
            break;
        case tlv::UnknownParameterTag:
            errmsg->error_status = Errors::inv_param_type;
            break;
        case tlv::InvalidParameterLength:
            errmsg->error_status = Errors::inv_param_length;
            break;
        case tlv::InvalidParameterCount:
        case tlv::MissingParameter:
            errmsg->error_status = Errors::missing_param;
            break;
        default:
            errmsg->error_status = Errors::unknown_error;
            break;
    }

    // Transfer ownership of safe ptr
    msg = errmsg.release();
}


//----------------------------------------------------------------------------
// Some help functions for dump.
//----------------------------------------------------------------------------

namespace {
    // Dump a timestamp.
    ts::UString DumpTimestamp(size_t indent, const std::optional<ts::SimulCryptDate>& timestamp)
    {
        if (timestamp.has_value()) {
            return ts::UString::Format(u"%*sTimestamp = %s\n", {indent, u"", ts::UString(timestamp.value())});
        }
        else {
            return ts::UString();
        }
    }

    // Dump a section.
    ts::UString DumpSection(size_t indent, const ts::SectionPtr& section)
    {
        if (section.isNull()) {
            return ts::UString();
        }
        else {
            const uint32_t flags = ts::UString::HEXA | ts::UString::ASCII | ts::UString::OFFSET | ts::UString::BPL;
            const ts::UString sec(ts::UString::Dump(section->content(), section->size(), flags, indent + 2, 16));
            return ts::UString::Format(u"%*sSection = \n%s", {indent, u"", sec});
        }
    }

    // Dump a byte block wshen not empty.
    ts::UString DumpBin(size_t indent, const ts::UString& name, const ts::ByteBlock& data)
    {
        if (data.empty()) {
            return ts::UString();
        }
        else {
            return ts::UString::Format(u"%*s%s = %s\n", {indent, u"", name, ts::UString::Dump(data, ts::UString::COMPACT)});
        }
    }
}


//----------------------------------------------------------------------------
// Log a section
//----------------------------------------------------------------------------

ts::duck::LogSection::LogSection(const tlv::MessageFactory& fact) :
    tlv::Message(fact.protocolVersion(), fact.commandTag())
{
    if (1 == fact.count(Tags::PRM_PID)) {
        pid = fact.get<PID>(Tags::PRM_PID);
    }
    if (1 == fact.count(Tags::PRM_TIMESTAMP)) {
        timestamp = SimulCryptDate();
        timestamp.value().get(fact, Tags::PRM_TIMESTAMP);
    }
    assert(1 == fact.count(Tags::PRM_SECTION));
    ByteBlock bb;
    fact.get(Tags::PRM_SECTION, bb);
    section = new Section(bb);
}

void ts::duck::LogSection::serializeParameters(tlv::Serializer& fact) const
{
    if (pid.has_value()) {
        fact.put(Tags::PRM_PID, pid.value());
    }
    if (timestamp.has_value()) {
        timestamp.value().put(fact, Tags::PRM_TIMESTAMP);
    }
    if (!section.isNull()) {
        fact.put(Tags::PRM_SECTION, section->content(), section->size());
    }
}

ts::UString ts::duck::LogSection::dump(size_t indent) const
{
    return UString::Format(u"%*sLogSection (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpOptionalHexa(indent, u"PID", pid) +
        DumpTimestamp(indent, timestamp) +
        DumpSection(indent, section);
}


//----------------------------------------------------------------------------
// Log a table
//----------------------------------------------------------------------------

ts::duck::LogTable::LogTable(const tlv::MessageFactory& fact) :
    tlv::Message(fact.protocolVersion(), fact.commandTag())
{
    if (1 == fact.count(Tags::PRM_PID)) {
        pid = fact.get<PID>(Tags::PRM_PID);
    }
    if (1 == fact.count(Tags::PRM_TIMESTAMP)) {
        timestamp = SimulCryptDate();
        timestamp.value().get(fact, Tags::PRM_TIMESTAMP);
    }
    std::vector<tlv::MessageFactory::Parameter> params;
    fact.get(Tags::PRM_SECTION, params);
    for (size_t i = 0; i < params.size(); ++i) {
        sections.push_back(new Section(params[i].addr, params[i].length));
    }
}

void ts::duck::LogTable::serializeParameters(tlv::Serializer& fact) const
{
    if (pid.has_value()) {
        fact.put(Tags::PRM_PID, pid.value());
    }
    if (timestamp.has_value()) {
        timestamp.value().put(fact, Tags::PRM_TIMESTAMP);
    }
    for (size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].isNull()) {
            fact.put(Tags::PRM_SECTION, sections[i]->content(), sections[i]->size());
        }
    }
}

ts::UString ts::duck::LogTable::dump(size_t indent) const
{
    UString secDump;
    for (size_t i = 0; i < sections.size(); ++i) {
        if (!sections[i].isNull()) {
            secDump.append(DumpSection(indent, sections[i]));
        }
    }

    return UString::Format(u"%*sLogTable (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpOptionalHexa(indent, u"PID", pid) +
        DumpTimestamp(indent, timestamp) +
        secDump;
}


//----------------------------------------------------------------------------
// Fake / demo clear ECM
//----------------------------------------------------------------------------

ts::duck::ClearECM::ClearECM(const tlv::MessageFactory& fact) :
    tlv::Message(fact.protocolVersion(), fact.commandTag())
{
    if (fact.count(Tags::PRM_CW_EVEN) > 0) {
        fact.get(Tags::PRM_CW_EVEN, cw_even);
    }
    if (fact.count(Tags::PRM_CW_ODD) > 0) {
        fact.get(Tags::PRM_CW_ODD, cw_odd);
    }
    if (fact.count(Tags::PRM_ACCESS_CRITERIA) > 0) {
        fact.get(Tags::PRM_ACCESS_CRITERIA, access_criteria);
    }
}

void ts::duck::ClearECM::serializeParameters(tlv::Serializer& fact) const
{
    if (!cw_even.empty()) {
        fact.put(Tags::PRM_CW_EVEN, cw_even);
    }
    if (!cw_odd.empty()) {
        fact.put(Tags::PRM_CW_ODD, cw_odd);
    }
    if (!access_criteria.empty()) {
        fact.put(Tags::PRM_ACCESS_CRITERIA, access_criteria);
    }
}

ts::UString ts::duck::ClearECM::dump(size_t indent) const
{
    return UString::Format(u"%*sClearECM (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        DumpBin(indent, u"CW (even)", cw_even) +
        DumpBin(indent, u"CW (odd)", cw_odd) +
        DumpBin(indent, u"Access criteria", access_criteria);
}


//----------------------------------------------------------------------------
// Error message
//----------------------------------------------------------------------------

ts::duck::Error::Error(const tlv::MessageFactory& fact) :
    tlv::Message(fact.protocolVersion(), fact.commandTag()),
    error_status(fact.get<uint16_t>(Tags::PRM_ERROR_CODE))
{
}

void ts::duck::Error::serializeParameters(tlv::Serializer& fact) const
{
    fact.put(Tags::PRM_ERROR_CODE, error_status);
}

ts::UString ts::duck::Error::dump(size_t indent) const
{
    return UString::Format(u"%*sError (" PROTOCOL_NAME u")\n", {indent, u""}) +
        tlv::Message::dump(indent) +
        dumpHexa(indent, u"error_status", error_status);
}
