//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsPluginEventContext.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PluginEventContext::PluginEventContext(uint32_t       event_code,
                                           UString        plugin_name,
                                           size_t         plugin_index,
                                           size_t         plugin_count,
                                           Plugin*        plugin,
                                           Object*        plugin_data,
                                           const BitRate& bitrate,
                                           PacketCounter  plugin_packets,
                                           PacketCounter  total_packets) :
    _event_code(event_code),
    _plugin_name(plugin_name),
    _plugin_index(plugin_index),
    _plugin_count(plugin_count),
    _plugin(plugin),
    _plugin_data(plugin_data),
    _bitrate(bitrate),
    _plugin_packets(plugin_packets),
    _total_packets(total_packets)
{
}
