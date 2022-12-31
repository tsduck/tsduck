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
//  Shift one or more PID's forward in the transport stream.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTimeShiftBuffer.h"

#define DEF_EVAL_MS       1000  // Default initial evaluation duration in milliseconds.
#define MAX_EVAL_PACKETS 30000  // Max number of packets after which the bitrate must be known.


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PIDShiftPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PIDShiftPlugin);
    public:
        // Implementation of plugin API
        PIDShiftPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool            _ignore_errors;  // Ignore evaluation errors.
        size_t          _shift_packets;  // Shift buffer size in packets.
        MilliSecond     _shift_ms;       // Shift buffer size in milliseconds.
        MilliSecond     _eval_ms;        // Initial evaluation phase duration in milliseconds.
        PIDSet          _pids;           // List of PID's to shift forward.

        // Working data:
        bool            _pass_all;       // Pass all packets after an error.
        PacketCounter   _init_packets;   // Count packets in PID's to shift during initial evaluation phase.
        TimeShiftBuffer _buffer;         // The timeshift buffer logic.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pidshift", ts::PIDShiftPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PIDShiftPlugin::PIDShiftPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Shift one or more PID's forward in the transport stream", u"[options]"),
    _ignore_errors(false),
    _shift_packets(0),
    _shift_ms(0),
    _eval_ms(0),
    _pids(),
    _pass_all(false),
    _init_packets(0),
    _buffer()
{
    option(u"pid", 'p', PIDVAL, 1, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specify a PID or range of PID's to shift forward. "
         u"Several -p or --pid options may be specified. At least one of them is required.");

    option(u"backward", 'b');
    help(u"backward",
         u"Revert the list of PID's, meaning shift forward all PID's except those in -p or --pid options. "
         u"In practice, this can be seen as shifting backward the selected PID's from the rest of the transport stream.");

    option(u"packets", 0, POSITIVE);
    help(u"packets", u"count",
         u"Specify the size of the shift buffer in packets. "
         u"There is no default, the size of the buffer shall be specified either using --packets or --time. "
         u"Using --packets is less intuitive than --time but allows starting the shift from the beginning.");

    option(u"time", 't', POSITIVE);
    help(u"time", u"milliseconds",
         u"Specify the size of the shift buffer in milliseconds. "
         u"During an initial evaluation phase (see option --initial-evaluation), "
         u"the global bitrate of all PID's to shift forward is evaluated. "
         u"This global bitrate is then used to convert the specified --time duration in a number of packets "
         u"and this value is used as fixed-size for the shift buffer. "
         u"Actual shifting the PID's starts at the end of this evaluation phase. "
         u"There is no default, the size of the buffer shall be specified either using --packets or --time.");

    option(u"initial-evaluation", 'i', POSITIVE);
    help(u"initial-evaluation", u"milliseconds",
         u"With --time, specify the duration of the initial evaluation phase in milliseconds. "
         u"This is a transport stream playout duration, not a wall-clock duration. "
         u"The default is " + UString::Decimal(DEF_EVAL_MS) + u" milliseconds.");

    option(u"ignore-errors");
    help(u"ignore-errors",
         u"Ignore shift buffer size evaluation errors or shift buffer write errors, pass packets without shifting.");

    option(u"directory", 0, DIRECTORY);
    help(u"directory",
         u"Specify a directory where the temporary buffer file is created (if one is needed). "
         u"By default, the system-specific area for temporary files is used. "
         u"The temporary file is hidden and automatically deleted on termination. "
         u"Specifying another location can be useful to redirect very large buffers to another disk. "
         u"If the reserved memory area is large enough to hold the buffer, no file is created.");

    option(u"memory-packets", 'm', POSITIVE);
    help(u"memory-packets",
         u"Specify the number of packets which are cached in memory. "
         u"Having a larger memory cache improves the performances. "
         u"By default, the size of the memory cache is " +
         UString::Decimal(TimeShiftBuffer::DEFAULT_MEMORY_PACKETS) + u" packets.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PIDShiftPlugin::getOptions()
{
    _ignore_errors = present(u"ignore-errors");
    getIntValue(_shift_packets, u"packets", 0);
    getIntValue(_shift_ms, u"time", 0);
    getIntValue(_eval_ms, u"initial-evaluation", DEF_EVAL_MS);
    getIntValues(_pids, u"pid");

    _buffer.setBackupDirectory(value(u"directory"));
    _buffer.setMemoryPackets(intValue<size_t>(u"memory-packets", TimeShiftBuffer::DEFAULT_MEMORY_PACKETS));

    // With --backward, the PID's to shift forward are all others.
    if (present(u"backward")) {
        _pids.flip();
    }

    if ((_shift_packets > 0 && _shift_ms > 0) || (_shift_packets == 0 && _shift_ms == 0)) {
        tsp->error(u"specify exactly one of --packets and --time for shift buffer sizing");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PIDShiftPlugin::start()
{
    if (_shift_packets > 0) {
        // Initialize the buffer only when its size is specified in packets.
        _buffer.setTotalPackets(_shift_packets);
        return _buffer.open(*tsp);
    }
    else {
        // Need an evaluation phase.
        _pass_all = false;
        _init_packets = 0;
        return true;
    }
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PIDShiftPlugin::stop()
{
    _buffer.close(*tsp);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PIDShiftPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // After an ignored error, let all packets pass, don't shift.
    if (_pass_all) {
        return TSP_OK;
    }

    // If the buffer is not yet open, we are in the initial evaluation phase.
    if (!_buffer.isOpen()) {

        // Count packets in the PID's to shift.
        if (_pids.test(pid)) {
            _init_packets++;
        }

        // Evaluate the duration from the beginning of the TS (zero if bitrate is unknown).
        const BitRate ts_bitrate = tsp->bitrate();
        const PacketCounter ts_packets = tsp->pluginPackets() + 1;
        const MilliSecond ms = PacketInterval(ts_bitrate, ts_packets);

        if (ms >= _eval_ms) {
            // The evaluation phase is completed.
            // Global bitrate of the selected PID's = ts_bitrate * _init_packet / ts_packets
            // Compute the amount of packets to shift in the selected PID's:
            const PacketCounter count = ((ts_bitrate * _init_packets * _shift_ms) / (ts_packets * MilliSecPerSec * PKT_SIZE_BITS)).toInt();

            tsp->debug(u"TS bitrate: %'d b/s, TS packets: %'d, selected: %'d, duration: %'d ms, shift: %'d packets", {ts_bitrate, ts_packets, _init_packets, ms, count});

            // We can do that only if we have seen some packets from them.
            if (count < TimeShiftBuffer::MIN_TOTAL_PACKETS) {
                tsp->error(u"not enough packets from selected PID's during evaluation phase, cannot compute the shift buffer size");
                _pass_all = true;
                return _ignore_errors ? TSP_OK : TSP_END;
            }

            tsp->verbose(u"setting shift buffer size to %'d packets", {count});
            _buffer.setTotalPackets(size_t(count));

            // Open the shift buffer.
            if (!_buffer.open(*tsp)) {
                _pass_all = true;
                return _ignore_errors ? TSP_OK : TSP_END;
            }
        }
        else if (ts_packets > MAX_EVAL_PACKETS && ts_bitrate == 0) {
            tsp->error(u"bitrate still unknown after %'d packets, cannot compute the shift buffer size", {ts_packets});
            _pass_all = true;
            return _ignore_errors ? TSP_OK : TSP_END;
        }
        else {
            // Still in evaluation phase, pass all packets.
            return TSP_OK;
        }
    }

    // No longer in evaluation phase, shift packets.
    if (_pids.test(pid) && !_buffer.shift(pkt, pkt_data, *tsp)) {
        _pass_all = true;
        return _ignore_errors ? TSP_OK : TSP_END;
    }
    return TSP_OK;
}
