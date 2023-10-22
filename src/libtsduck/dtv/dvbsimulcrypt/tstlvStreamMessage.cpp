//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvStreamMessage.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tlv::StreamMessage::StreamMessage(TAG tag, uint16_t ch_id, uint16_t st_id) :
    ChannelMessage(tag, ch_id),
    stream_id(st_id)
{
}

ts::tlv::StreamMessage::StreamMessage(VERSION protocol_version, TAG tag, uint16_t ch_id, uint16_t st_id) :
    ChannelMessage(protocol_version, tag, ch_id),
    stream_id(st_id)
{
}

ts::tlv::StreamMessage::StreamMessage(const tlv::MessageFactory& fact, TAG tag_ch_id, TAG tag_st_id) :
    ChannelMessage(fact, tag_ch_id),
    stream_id(fact.get<uint16_t>(tag_st_id))
{
}

ts::tlv::StreamMessage::~StreamMessage()
{
}
