//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPluginEventData.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PluginEventData::PluginEventData(const uint8_t* data, size_t size) :
    _data(const_cast<uint8_t*>(data)),
    _max_size(data == nullptr ? 0 : size),
    _cur_size(_max_size)
{
}

ts::PluginEventData::PluginEventData(uint8_t* data, size_t size, size_t max_size) :
    _read_only(data == nullptr),
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
        MemCopy(_data + _cur_size, data_addr, data_size);
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
