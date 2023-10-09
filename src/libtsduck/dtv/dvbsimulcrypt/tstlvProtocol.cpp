//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstlvProtocol.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tlv::Protocol::Protocol(VERSION v) :
    _has_version(true),
    _version(v)
{
}

ts::tlv::Protocol::~Protocol()
{
}


//----------------------------------------------------------------------------
// Declare a command tag in the protocol and its parameters.
//----------------------------------------------------------------------------

void ts::tlv::Protocol::add(TAG cmd_tag, TAG param_tag, size_t min_size, size_t max_size, size_t min_count, size_t max_count)
{
    _commands[cmd_tag].params[param_tag] = Parameter{nullptr, min_size, max_size, min_count, max_count};
}

void ts::tlv::Protocol::add(TAG cmd_tag, TAG param_tag, const Protocol* compound, size_t min_count, size_t max_count)
{
    _commands[cmd_tag].params[param_tag] = Parameter{compound, 0, 0, min_count, max_count};
}
