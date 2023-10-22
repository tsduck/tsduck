//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvChannelMessage.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tlv::ChannelMessage::ChannelMessage(TAG tag, uint16_t ch_id) :
    Message(tag),
    channel_id(ch_id)
{
}

ts::tlv::ChannelMessage::ChannelMessage(VERSION protocol_version, TAG tag, uint16_t ch_id) :
    Message(protocol_version, tag),
    channel_id(ch_id)
{
}

ts::tlv::ChannelMessage::ChannelMessage(const tlv::MessageFactory& fact, TAG tag_ch_id) :
    Message(fact.protocolVersion(), fact.commandTag()),
    channel_id(fact.get<uint16_t>(tag_ch_id))
{
}

ts::tlv::ChannelMessage::~ChannelMessage()
{
}
