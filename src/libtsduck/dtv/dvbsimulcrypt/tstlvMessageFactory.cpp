//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvMessageFactory.h"
#include "tstlvAnalyzer.h"


//----------------------------------------------------------------------------
// Constructors: Analyze a TLV message in memory.
//----------------------------------------------------------------------------

ts::tlv::MessageFactory::MessageFactory(const void* addr, size_t size, const Protocol& protocol) :
    _msg_base(reinterpret_cast<const uint8_t*>(addr)),
    _msg_length(size),
    _protocol(protocol)
{
    analyzeMessage();
}

ts::tlv::MessageFactory::MessageFactory(const ByteBlock &bb, const Protocol& protocol) :
    _msg_base(bb.data()),
    _msg_length(bb.size()),
    _protocol(protocol),
    _error_status(OK),
    _error_info(0),
    _error_info_is_offset(false),
    _protocol_version(0),
    _command_tag(0),
    _params()
{
    analyzeMessage();
}


//----------------------------------------------------------------------------
// Message factory
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::factory(MessagePtr& msg) const
{
    if (_error_status == OK) {
        _protocol.factory(*this, msg);
    }
    else {
        msg.clear();
    }
}

ts::tlv::MessagePtr ts::tlv::MessageFactory::factory() const
{
    MessagePtr msg;
    factory(msg);
    return msg;
}

void ts::tlv::MessageFactory::buildErrorResponse(MessagePtr& msg) const
{
    if (_error_status == OK) {
        msg.clear(); // no error
    }
    else {
        _protocol.buildErrorResponse(*this, msg);
    }
}

ts::tlv::MessagePtr ts::tlv::MessageFactory::errorResponse() const
{
    MessagePtr msg;
    buildErrorResponse(msg);
    return msg;
}


//----------------------------------------------------------------------------
// Analyze the TLV message in memory.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::analyzeMessage()
{
    // Size of header, before the global TLV.
    size_t header_size = 0;

    // Get and check the protocol version.
    if (_protocol.hasVersion()) {
        header_size = sizeof(VERSION);
        if (_msg_length < sizeof(VERSION)) {
            _error_status = InvalidMessage;
            _error_info_is_offset = true;
            _error_info = 0; // offset in message
            return;
        }
        _protocol_version = GetUInt8(_msg_base);
        if (_protocol_version != _protocol.version()) {
            _error_status = UnsupportedVersion;
            _error_info_is_offset = true;
            _error_info = 0; // offset in message
            return;
        }
    }

    // Analyze the message envelope.
    tlv::Analyzer cmd_anl(_msg_base + header_size, _msg_length - header_size);
    if (cmd_anl.endOfMessage() || !cmd_anl.valid()) {
        _error_status = InvalidMessage;
        _error_info_is_offset = true;
        _error_info = uint16_t (header_size); // offset in message
        return;
    }

    _command_tag = cmd_anl.tag();
    LENGTH params_length = cmd_anl.length();
    const void* params_list = cmd_anl.valueAddr();

    // Get an interator pointing to the definition of the command in the protocol description.
    auto cmd_it = _protocol._commands.find(_command_tag);

    // Check that the command exists
    if (cmd_it == _protocol._commands.end()) {
        _error_status = UnknownCommandTag;
        _error_info_is_offset = true;
        _error_info = uint16_t(header_size); // offset in message
        return;
    }

    // Analyze the parameters
    tlv::Analyzer parm_anl(params_list, params_length);

    while (!parm_anl.endOfMessage()) {

        // Get current parameter
        TAG         parm_tag     = parm_anl.tag();
        const void* tlv_addr     = parm_anl.fieldAddr();
        size_t      tlv_size     = parm_anl.fieldSize();
        const void* value_addr   = parm_anl.valueAddr();
        LENGTH      value_length = parm_anl.length();

        // Locate the description of this parameter tag in the protocol definition.
        auto parm_it = cmd_it->second.params.find(parm_tag);

        if (parm_it == cmd_it->second.params.end()) {
            // Parameter tag not found in protocol definition
            _error_status = UnknownParameterTag;
            _error_info_is_offset = true;
            _error_info = uint16_t(uint8_ptr(tlv_addr) - _msg_base); // offset
            return;
        }

        // Store the parameter into the message factory
        if (parm_it->second.compound != nullptr) {
            // The parameter is a compound TLV, analyze it.
            // Store the parameter value in the multimap for this command.
            // Analyze the compound parameter.
            auto it = _params.insert(ParameterMultimap::value_type(parm_tag, ExtParameter(tlv_addr, tlv_size, value_addr, value_length, new MessageFactory(tlv_addr, tlv_size, *parm_it->second.compound))));

            // Check if the analysis is successful
            if ((_error_status = it->second.compound->_error_status) != OK) {
                _error_info = it->second.compound->_error_info;
                _error_info_is_offset = it->second.compound->_error_info_is_offset;
                if (_error_info_is_offset) {
                    _error_info += uint16_t(uint8_ptr(tlv_addr) - _msg_base); // offset
                }
                return;
            }
        }
        else if (value_length < parm_it->second.min_size || value_length > parm_it->second.max_size) {
            // The parameter is not a compound TLV and its length is not in protocol-defined range
            _error_status = InvalidParameterLength;
            _error_info_is_offset = true;
            _error_info = uint16_t(uint8_ptr(tlv_addr) - _msg_base); // offset
            return;
        }
        else {
            // The parameter is not a compound TLV and its length is fine.
            // Store the parameter value in the multimap for this command
            _params.insert(ParameterMultimap::value_type (parm_tag, ExtParameter(tlv_addr, tlv_size, value_addr, value_length)));
        }

        // Advance to next parameter
        parm_anl.next();
    }

    // Did we reach the end of parameter list without error ?
    if (!parm_anl.valid()) {
        _error_status = InvalidMessage;
        _error_info_is_offset = true;
        _error_info = uint16_t(uint8_ptr(parm_anl.fieldAddr()) - _msg_base); // offset
        return;
    }

    // At this point, we know that the command is defined in the protocol
    // and that all actual parameters are defined for this command in the
    // protocol. Now, we need to check that all protocol-defined parameters
    // are present, with a number of occurences in the allowed range.
    for (const auto& parm_it : cmd_it->second.params) {

        // Protocol-defined parameter tag:
        TAG tag = parm_it.first;
        // Protocol-defined parameter properties:
        const Protocol::Parameter& desc(parm_it.second);
        // Number of actual occurences in current command:
        size_t count = _params.count(tag);

        if (count < desc.min_count || count > desc.max_count) {
            if (count == 0 && desc.min_count > 0) {
                _error_status = MissingParameter;
            }
            else {
                _error_status = InvalidParameterCount;
            }
            _error_info_is_offset = false;
            _error_info = tag;
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Get location of the first occurence of a parameter:
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::get(TAG tag, Parameter& param) const
{
    const auto it = _params.find(tag);
    if (it == _params.end()) {
        throw DeserializationInternalError(UString::Format(u"No parameter 0x%X in message", {tag}));
    }
    else {
        param = it->second;
    }
}


//----------------------------------------------------------------------------
// Get all occurences of a parameter.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::get(TAG tag, std::vector<Parameter>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.reserve(_params.count(tag));

    // Fill vector with parameter values
    const auto last = _params.upper_bound(tag);
    for (auto it = _params.lower_bound(tag); it != last; ++it) {
        param.push_back(it->second);
    }
}


//----------------------------------------------------------------------------
// Get all occurences of a parameter.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::get(TAG tag, std::vector<bool>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.reserve(_params.count(tag));
    // Fill vector with parameter values
    const auto last = _params.upper_bound(tag);
    for (auto it = _params.lower_bound(tag); it != last; ++it) {
        checkParamSize<uint8_t> (tag, it);
        param.push_back(GetUInt8(it->second.addr) != 0);
    }
}


//----------------------------------------------------------------------------
// Get all occurences of a parameter.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::get(TAG tag, std::vector<std::string>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.resize(_params.count(tag));
    // Fill vector with parameter values
    auto it = _params.lower_bound(tag);
    const auto last = _params.upper_bound(tag);
    for (int i = 0; it != last; ++it, ++i) {
        param[i].assign(static_cast<const char*>(it->second.addr), it->second.length);
    }
}


//----------------------------------------------------------------------------
// Get first occurence of a parameter as a compound TLV parameter.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::getCompound(TAG tag, MessagePtr& param) const
{
    const auto it = _params.find(tag);
    if (it == _params.end()) {
        throw DeserializationInternalError(UString::Format(u"No parameter 0x%X in message", {tag}));
    }
    else if (it->second.compound.isNull()) {
        throw DeserializationInternalError(UString::Format(u"Parameter 0x%X is not a compound TLV", {tag}));
    }
    else {
        it->second.compound->factory(param);
    }
}


//----------------------------------------------------------------------------
// Get all occurences of a parameter as compound TLV parameters.
//----------------------------------------------------------------------------

void ts::tlv::MessageFactory::getCompound(TAG tag, std::vector<MessagePtr>& param) const
{
    // Reinitialize result vector
    param.clear();
    param.resize(_params.count(tag));
    // Fill vector with parameter values
    auto it = _params.lower_bound(tag);
    const auto last = _params.upper_bound(tag);
    for (int i = 0; it != last; ++it, ++i) {
        if (it->second.compound.isNull()) {
            throw DeserializationInternalError(UString::Format(u"Occurence %d of parameter 0x%X not a compound TLV", {i, tag}));
        }
        else {
            it->second.compound->factory(param[i]);
        }
    }
}
