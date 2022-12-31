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
//!
//!  @file
//!  Context of a plugin event.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsTS.h"

namespace ts {

    class Plugin;

    //!
    //! Context of a plugin event.
    //! Each time a plugin signals an event for the application, a PluginEventContext
    //! instance is built and passed to all registered event handlers for that event.
    //! @ingroup plugin
    //!
    class TSDUCKDLL PluginEventContext
    {
        // Prevent copy to allow safe storage of references.
        TS_NOBUILD_NOCOPY(PluginEventContext);
    public:
        //!
        //! Constructor.
        //!
        //! @param [in] event_code A plugin-defined 32-bit code describing the event type.
        //! There is no predefined list of event codes. Plugin should define their own codes
        //! based on meaningful 4-char literals in order to avoid value clashes, for instance:
        //! @code
        //! static constexpr uint32_t FAIL_EVENT = 'FAIL';
        //! @endcode
        //! @param [in] plugin_name Plugin name as found in the plugin registry.
        //! @param [in] plugin_index Plugin index in the chain. For tsp, plugins are numbered
        //! from 0 (the input plugin) to N-1 (the output plugin). For tsswitch, the input plugins
        //! are numbered from 0 to N-2 and the output plugin is N-1.
        //! @param [in] plugin_count Total number N of plugins in the chain.
        //! @param [in] plugin Address of the plugin instance which signalled the event.
        //! If this is an application-defined plugin which exposes more services, the event
        //! handler may try a dynamic_cast on this pointer.
        //! @param [in] plugin_data Address of the plugin-specific data. It can be a null pointer.
        //! In the case of an application-defined plugin the application may try a dynamic_cast on
        //! this pointer to an expected type
        //! @param [in] bitrate Known bitrate in the context of the plugin at the time of the event.
        //! @param [in] plugin_packets Number of packets which passed through the plugin at the time
        //! of the event.
        //! @param [in] total_packets Total number of packets which passed through the plugin thread
        //! at the time of the event. It can be more than @a plugin_packets if some packets were not
        //! submitted to the plugin (deleted or excluded packets).
        //!
        PluginEventContext(uint32_t       event_code,
                           UString        plugin_name,
                           size_t         plugin_index,
                           size_t         plugin_count,
                           Plugin*        plugin,
                           Object*        plugin_data = nullptr,
                           const BitRate& bitrate = 0,
                           PacketCounter  plugin_packets = 0,
                           PacketCounter  total_packets = 0);

        //!
        //! Get the event code.
        //! @return A plugin-defined 32-bit code describing the event type.
        //!
        uint32_t eventCode() const { return _event_code; }

        //!
        //! Get the plugin name.
        //! @return Plugin name as found in the plugin registry.
        //!
        UString pluginName() const { return _plugin_name; }

        //!
        //! Get the plugin index in the processing chain.
        //! @return Plugin index in the chain. For tsp, plugins are numbered
        //! from 0 (the input plugin) to N-1 (the output plugin). For tsswitch, the input plugins
        //! are numbered from 0 to N-2 and the output plugin is N-1.
        //!
        size_t pluginIndex() const { return _plugin_index; }

        //!
        //! Get the total number of plugins in the processing chain.
        //! @return Total number of plugins in the chain.
        //!
        size_t pluginCount() const { return _plugin_count; }

        //!
        //! Get the plugin which signalled the event.
        //! @return Address of the plugin instance which signalled the event.
        //! If this is an application-defined plugin which exposes more services, the event
        //! handler may try a dynamic_cast on this pointer.
        //!
        Plugin* plugin() const { return _plugin; }

        //!
        //! Get the plugin-specific data for this event
        //! @return Address of the plugin-specific data. It can be a null pointer.
        //! In the case of an application-defined plugin the application may try a dynamic_cast on
        //! this pointer to an expected type
        //!
        Object* pluginData() const { return _plugin_data; }

        //!
        //! Get the plugin bitrate.
        //! @return Known bitrate in the context of the plugin at the time of the event.
        //!
        BitRate bitrate() const { return _bitrate; }

        //!
        //! Get the number of packets which passed through the plugin.
        //! @return Number of packets which passed through the plugin at the time
        //! of the event.
        //!
        PacketCounter pluginPackets() const { return _plugin_packets; }

        //!
        //! Get the total number of packets which passed through the plugin thread.
        //! @return Total number of packets which passed through the plugin thread
        //! at the time of the event. It can be more than @a plugin_packets if some packets were not
        //! submitted to the plugin (deleted or excluded packets).
        //!
        PacketCounter totalPackets() const { return _total_packets; }

    private:
        uint32_t      _event_code;
        UString       _plugin_name;
        size_t        _plugin_index;
        size_t        _plugin_count;
        Plugin*       _plugin;
        Object*       _plugin_data;
        BitRate       _bitrate;
        PacketCounter _plugin_packets;
        PacketCounter _total_packets;
    };
}
