//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsPluginEventData.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PluginEventData::PluginEventData(const uint8_t* data, size_t size) :
    _read_only(true),
    _error(false),
    _data(const_cast<uint8_t*>(data)),
    _max_size(data == nullptr ? 0 : size),
    _cur_size(_max_size)
{
}

ts::PluginEventData::PluginEventData(uint8_t* data, size_t size, size_t max_size) :
    _read_only(data == nullptr),
    _error(false),
    _data(data),
    _max_size(data == nullptr ? 0 : max_size),
    _cur_size(std::min(size, _max_size))
{
}

ts::PluginEventData::~PluginEventData()
{
}


//----------------------------------------------------------------------------
// Append new application data inside the plugin event data area.
//----------------------------------------------------------------------------

bool ts::PluginEventData::append(const void* data_addr, size_t data_size)
{
    if (_read_only || data_addr == nullptr || data_size > _max_size - _cur_size) {
        return false;
    }
    else {
        ::memcpy(_data + _cur_size, data_addr, data_size);
        _cur_size += data_size;
        return true;
    }
}


//----------------------------------------------------------------------------
// Update the current size of the plugin modifiable event data.
//----------------------------------------------------------------------------

bool ts::PluginEventData::updateSize(size_t size)
{
    if (_read_only || size > _max_size) {
        return false;
    }
    else {
        _cur_size = std::min(size, _max_size);
        return true;
    }
}
