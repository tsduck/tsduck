//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tstlvProtocol.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tlv::Protocol::Protocol() :
    _has_version(false),
    _version(0),
    _commands()
{
}

ts::tlv::Protocol::Protocol(VERSION v) :
    _has_version(true),
    _version(v),
    _commands()
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
