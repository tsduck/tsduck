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
//
//  Transport stream processor shared library:
//  Delay packet transmission by a fixed amount of packets.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTimeShiftBuffer.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TimeShiftPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(TimeShiftPlugin);
    public:
        // Implementation of plugin API
        TimeShiftPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool            _drop_initial;   // Drop initial packets instead of null.
        MilliSecond     _time_shift_ms;  // Time-shift in milliseconds.
        TimeShiftBuffer _buffer;         // The timeshift buffer logic.

        // Try to initialize the buffer using the time as size.
        // Return false on fatal error only.
        bool initBufferByTime();
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"timeshift", ts::TimeShiftPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TimeShiftPlugin::TimeShiftPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Delay transmission by a fixed amount of packets", u"[options]"),
    _drop_initial(false),
    _time_shift_ms(0),
    _buffer()
{
    option(u"directory", 0, DIRECTORY);
    help(u"directory",
         u"Specify a directory where the temporary buffer file is created. "
         u"By default, the system-specific area for temporary files is used. "
         u"The temporary file is hidden and automatically deleted on termination. "
         u"Specifying another location can be useful to redirect very large buffers to another disk. "
         u"If the reserved memory area is large enough to hold the buffer, no file is created.");

    option(u"drop-initial", 'd');
    help(u"drop-initial",
         u"Drop output packets during the initial phase, while the time-shift buffer is filling. "
         u"By default, initial packets are replaced by null packets.");

    option(u"memory-packets", 'm', UNSIGNED);
    help(u"memory-packets",
         u"Specify the number of packets which are cached in memory. "
         u"Having a larger memory cache improves the performances. "
         u"By default, the size of the memory cache is " +
         UString::Decimal(TimeShiftBuffer::DEFAULT_MEMORY_PACKETS) + u" packets.");

    option(u"packets", 'p', UNSIGNED);
    help(u"packets",
         u"Specify the size of the time-shift buffer in packets. "
         u"There is no default, the size of the buffer shall be specified either using --packets or --time.");

    option(u"time", 't', UNSIGNED);
    help(u"time", u"milliseconds",
         u"Specify the size of the time-shift buffer in milliseconds. "
         u"The initial bitrate is used to convert this duration in number "
         u"of packets and this value is used as fixed-size for the buffer. "
         u"This is convenient only for constant bitrate (CBR) streams. "
         u"There is no default, the size of the buffer shall be specified either using --packets or --time.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::TimeShiftPlugin::getOptions()
{
    _drop_initial = present(u"drop-initial");
    _time_shift_ms = intValue<MilliSecond>(u"time", 0);
    const size_t packets = intValue<size_t>(u"packets", 0);
    _buffer.setBackupDirectory(value(u"directory"));
    _buffer.setMemoryPackets(intValue<size_t>(u"memory-packets", TimeShiftBuffer::DEFAULT_MEMORY_PACKETS));

    if ((packets > 0 && _time_shift_ms > 0) || (packets == 0 && _time_shift_ms == 0)) {
        tsp->error(u"specify exactly one of --packets and --time for time-shift buffer sizing");
        return false;
    }

    if (packets > 0) {
        _buffer.setTotalPackets(packets);
    }

    return true;
}


//----------------------------------------------------------------------------
// Try to initialize the buffer using the time as size.
//----------------------------------------------------------------------------

bool ts::TimeShiftPlugin::initBufferByTime()
{
    // Try to open only when the buffer is not yet open and --time was specified.
    if (!_buffer.isOpen() && _time_shift_ms > 0) {
        const BitRate bitrate = tsp->bitrate();
        if (bitrate > 0) {
            const PacketCounter packets = PacketDistance(bitrate, _time_shift_ms);
            if (packets < TimeShiftBuffer::MIN_TOTAL_PACKETS) {
                tsp->error(u"bitrate %'d b/s is too small to perform time-shift", {bitrate});
                return false;
            }
            else {
                _buffer.setTotalPackets(size_t(packets));
                return _buffer.open(*tsp);
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TimeShiftPlugin::start()
{
    // Initialize the buffer only when its size is specified in packets or the bitrate is already known.
    return _time_shift_ms == 0 ? _buffer.open(*tsp) : initBufferByTime();
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::TimeShiftPlugin::stop()
{
    _buffer.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TimeShiftPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // If buffer is not yet open, we are waiting for a valid bitrate to size it.
    if (!_buffer.isOpen()) {
        // Try to open it.
        if (!initBufferByTime()) {
            return TSP_END; // fatal error
        }
        // Issue a warning the first time only.
        if (_buffer.isOpen()) {
            tsp->verbose(u"time-shift buffer size is %'d packets", {_buffer.size()});
        }
        else if (tsp->pluginPackets() == 0) {
            tsp->warning(u"unknown initial bitrate, discarding packets until a valid bitrate can set the buffer size");
        }
    }

    if (!_buffer.isOpen()) {
        // Still waiting to set a buffer size, discarding packets.
        return _drop_initial ? TSP_DROP : TSP_NULL;
    }
    else {
        // Check if we are in the initial filling phase.
        const bool init_phase = !_buffer.full();
        if (!_buffer.shift(pkt, pkt_data, *tsp)) {
            return TSP_END; // fatal error
        }
        return init_phase && _drop_initial ? TSP_DROP : TSP_OK;
    }
}
