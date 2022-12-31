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

package io.tsduck;

/**
 * Context of a plugin event.
 * Each time a plugin signals an event for the application, a PluginEventContext
 * instance is built and passed to all registered event handlers for that event.
 * @ingroup java
 */
public class PluginEventContext {

    private int     _eventCode = 0;
    private String  _pluginName = "";
    private int     _pluginIndex = 0;
    private int     _pluginCount = 0;
    private int     _bitrate = 0;
    private long    _pluginPackets = 0;
    private long    _totalPackets = 0;
    private boolean _readOnlyData = true;
    private int     _maxDataSize = 0;
    private byte[]  _outputData = null;

    /**
     * Constructor.
     *
     * @param ecode A plugin-defined 32-bit code describing the event type.
     * There is no predefined list of event codes. Plugin should define their own codes
     * based on meaningful 4-char literals in order to avoid value clashes, for instance:
     * @code
     * static constexpr uint32_t FAIL_EVENT = 'FAIL';
     * @endcode
     * @param pname Plugin name as found in the plugin registry.
     * @param pindex Plugin index in the chain. For tsp, plugins are numbered from 0 (the input plugin) to N-1 (the output plugin).
     * For tsswitch, the input plugins are numbered from 0 to N-2 and the output plugin is N-1.
     * @param pcount Total number N of plugins in the chain.
     * @param brate Known bitrate in the context of the plugin at the time of the event.
     * @param ppackets Number of packet0s which passed through the plugin at the time of the event.
     * @param tpackets Total number of packets which passed through the plugin thread at the time of the event.
     * It can be more than @a ppackets if some packets were not submitted to the plugin (deleted or excluded packets).
     * @param rdonly True if the event data are read-only.
     * @param maxdsize Maximum returned data size (if not read only).
     */
    public PluginEventContext(int ecode, String pname, int pindex, int pcount, int brate, long ppackets, long tpackets, boolean rdonly, int maxdsize) {
        _eventCode = ecode;
        _pluginName = pname;
        _pluginIndex = pindex;
        _pluginCount = pcount;
        _bitrate = brate;
        _pluginPackets = ppackets;
        _totalPackets = tpackets;
        _readOnlyData = rdonly;
        _maxDataSize = maxdsize;
    }

    /**
     * Get the event code.
     * @return A plugin-defined 32-bit code describing the event type.
     */
    public int eventCode() {
        return _eventCode;
    }

    /**
     * Get the plugin name.
     * @return Plugin name as found in the plugin registry.
     */
    public String pluginName() {
        return _pluginName;
    }

    /**
     * Get the plugin index in the processing chain.
     * @return Plugin index in the chain. For tsp, plugins are numbered from 0 (the input plugin) to N-1 (the output plugin).
     * For tsswitch, the input plugins are numbered from 0 to N-2 and the output plugin is N-1.
     */
    public int pluginIndex() {
        return _pluginIndex;
    }

    /**
     * Get the total number of plugins in the processing chain.
     * @return Total number of plugins in the chain.
     */
    public int pluginCount() {
        return _pluginCount;
    }

    /**
     * Get the plugin bitrate.
     * @return Known bitrate in the context of the plugin at the time of the event.
     */
    public int bitrate() {
        return _bitrate;
    }

    /**
     * Get the number of packets which passed through the plugin.
     * @return Number of packets which passed through the plugin at the time of the event.
     */
    public long pluginPackets() {
        return _pluginPackets;
    }

    /**
     * Get the total number of packets which passed through the plugin thread.
     * @return Total number of packets which passed through the plugin thread at the time of the event.
     * It can be more than @a pluginPackets() if some packets were not submitted to the plugin (deleted or excluded packets).
     */
    public long totalPackets() {
        return _totalPackets;
    }

    /**
     * Indicate if the event data are read-only or if they can be updated.
     * @return True if the event data are read-only, false if they can be updated.
     */
    public boolean readOnlyData() {
        return _readOnlyData;
    }

    /**
     * Get the maximum returned data size in bytes (if they can be modified).
     * @return Maximum returned data size in bytes.
     */
    public int maxDataSize() {
        return _readOnlyData ? 0 : _maxDataSize;
    }

    /**
     * Set the event returned data.
     * @param data Event returned data. Ignored is returned data is read-only or larger than its max size.
     */
    public void setOutputData(byte[] data) {
        _outputData = _readOnlyData || data == null || data.length > _maxDataSize ? null : data;
    }

    /**
     * Get the event returned data.
     * @return Event returned data.
     */
    public byte[] outputData() {
        return _outputData;
    }
}
