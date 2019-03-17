//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Transport stream processor: Execution context of an input plugin
//
//----------------------------------------------------------------------------

#include "tspInputExecutor.h"
#include "tsPCRAnalyzer.h"
#include "tsTime.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::InputExecutor::InputExecutor(Options* options,
                                      const PluginOptions* pl_options,
                                      const ThreadAttributes& attributes,
                                      Mutex& global_mutex) :

    PluginExecutor(options, pl_options, attributes, global_mutex),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _in_sync_lost(false),
    _instuff_start_remain(options->instuff_start),
    _instuff_stop_remain(options->instuff_stop),
    _instuff_nullpkt_remain(0),
    _instuff_inpkt_remain(0)
{
}


//----------------------------------------------------------------------------
// Initializes the buffer for all plugin executors, starting at
// this input executor. The buffer is pre-loaded with initial data.
// The initial bitrate is evaluated. The buffer is propagated
// to all executors. Must be executed in synchronous environment,
// before starting all executor threads.
// Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::tsp::InputExecutor::initAllBuffers(PacketBuffer* buffer)
{
    // Pre-load half of the buffer with packets from the input device.
    const size_t pkt_read = receiveAndStuff(buffer->base(), buffer->count() / 2);

    if (pkt_read == 0) {
        return false; // receive error
    }

    debug(u"initial buffer load: %'d packets, %'d bytes", {pkt_read, pkt_read * PKT_SIZE});

    // Try to evaluate the initial input bitrate.
    // First, ask the plugin to evaluate its bitrate.
    BitRate init_bitrate = getBitrate();
    if (init_bitrate == 0) {
        // The input device cannot evaluate a bitrate.
        // Try to determine the original bitrate from PCR analysis.
        // Say we need at least 32 PCR's per PID, on at least 1 PID.
        PCRAnalyzer zer(1, 32); // 1 PID, 32 PCR's
        for (size_t p = 0; p < pkt_read && !zer.feedPacket(buffer->base()[p]); p++) {}
        if (zer.bitrateIsValid()) {
            init_bitrate = zer.bitrate188();
        }
    }
    if (init_bitrate == 0) {
        // Still no bitrate available from PCR, try DTS from video PID's.
        // Since DTS are less accurate than PCR, analyze all packets in
        // buffer, do not stop when bitrate is supposedly known.
        PCRAnalyzer zer;
        zer.resetAndUseDTS(1, 32); // 1 PID, 32 DTS
        for (size_t p = 0; p < pkt_read; p++) {
            zer.feedPacket(buffer->base()[p]);
        }
        if (zer.bitrateIsValid()) {
            init_bitrate = zer.bitrate188();
        }
    }
    if (init_bitrate == 0) {
        verbose(u"unknown input bitrate");
    }
    else {
        verbose(u"input bitrate is %'d b/s", {init_bitrate});
    }

    // Indicate that the loaded packets are now available to the next packet processor.
    PluginExecutor* next = ringNext<PluginExecutor>();
    next->initBuffer(buffer, 0, pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate);

    // The rest of the buffer belongs to this input processor for reading
    // additional packets. All other processors have an implicit empty buffer
    // (_pkt_first and _pkt_cnt are zero).
    initBuffer(buffer, pkt_read % buffer->count(), buffer->count() - pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate);

    // Propagate initial input bitrate to all processors
    while ((next = next->ringNext<PluginExecutor>()) != this) {
        next->initBuffer(buffer, 0, 0, pkt_read == 0, pkt_read == 0, init_bitrate);
    }

    return true;
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's getBitrate() method,
// taking into account the tsp input stuffing options.
//----------------------------------------------------------------------------

ts::BitRate ts::tsp::InputExecutor::getBitrate()
{
    // Get bitrate from plugin
    BitRate bitrate = _options->bitrate > 0 ? _options->bitrate : _input->getBitrate();

    // Adjust to input stuffing
    if (bitrate == 0 || _options->instuff_inpkt == 0) {
        return bitrate;
    }
    else {
        return BitRate((uint64_t(bitrate) * uint64_t(_options->instuff_nullpkt + _options->instuff_inpkt)) / uint64_t(_options->instuff_inpkt));
    }
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's receive() method,
// checking the validity of the input.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveAndValidate(TSPacket* buffer, size_t max_packets)
{
    // If synchronization lost, report an error
    if (_in_sync_lost) {
        return 0;
    }

    // Invoke the plugin receive method
    size_t count = _input->receive(buffer, max_packets);

    // Validate sync byte (0x47) at beginning of each packet
    for (size_t n = 0; n < count; ++n) {
        if (buffer[n].hasValidSync()) {
            // Count good packets from plugin
            addPluginPackets(1);
        }
        else {
            // Report error
            error(u"synchronization lost after %'d packets, got 0x%X instead of 0x%X", {pluginPackets(), buffer[n].b[0], SYNC_BYTE});
            // In debug mode, partial dump of input
            // (one packet before lost of sync and 3 packets starting at lost of sync).
            if (maxSeverity() >= 1) {
                if (n > 0) {
                    debug(u"content of packet before lost of synchronization:\n" +
                          UString::Dump(buffer[n-1].b, PKT_SIZE, UString::HEXA | UString::OFFSET | UString::BPL, 4, 16));
                }
                size_t dump_count = std::min<size_t>(3, count - n);
                debug(u"data at lost of synchronization:\n" +
                      UString::Dump(buffer[n].b, dump_count * PKT_SIZE, UString::HEXA | UString::OFFSET | UString::BPL, 4, 16));
            }
            // Ignore subsequent packets
            count = n;
            _in_sync_lost = true;
        }
    }

    return count;
}


//----------------------------------------------------------------------------
// Encapsulation of receiveAndValidate() method,
// taking into account the tsp input stuffing options.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveAndStuff(TSPacket* buffer, size_t max_packets)
{
    size_t pkt_done = 0;              // Number of received packets in buffer
    size_t pkt_remain = max_packets;  // Remaining number of packets to read
    size_t pkt_from_input = 0;        // Number of packets actually read from plugin

    // If initial stuffing not yet completed, add initial stuffing.
    while (_instuff_start_remain > 0 && pkt_remain > 0) {
        *buffer++ = NullPacket;
        _instuff_start_remain--;
        pkt_remain--;
        pkt_done++;
        addNonPluginPackets(1);
    }

    // Now read real packets.
    if (_options->instuff_inpkt == 0) {
        // There is no --add-input-stuffing option, simply call the plugin
        pkt_from_input = receiveAndValidate(buffer, pkt_remain);
        pkt_done += pkt_from_input;
        addPluginPackets(pkt_from_input);
    }
    else {
        // Otherwise, we have to alternate input packets and null packets.

        while (pkt_remain > 0) {

            // Stuff null packets.
            while (_instuff_nullpkt_remain > 0 && pkt_remain > 0) {
                *buffer++ = NullPacket;
                _instuff_nullpkt_remain--;
                pkt_remain--;
                pkt_done++;
                addNonPluginPackets(1);
            }

            if (pkt_remain == 0) {
                break;
            }

            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_inpkt_remain = _options->instuff_inpkt;
            }

            // Read input packets from the plugin
            max_packets = pkt_remain < _instuff_inpkt_remain ? pkt_remain : _instuff_inpkt_remain;

            size_t pkt_in = receiveAndValidate(buffer, max_packets);

            assert(pkt_in <= pkt_remain);
            assert(pkt_in <= _instuff_inpkt_remain);
            assert(pkt_in <= max_packets);

            buffer += pkt_in;
            pkt_remain -= pkt_in;
            pkt_done += pkt_in;
            pkt_from_input += pkt_in;
            _instuff_inpkt_remain -= pkt_in;

            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_nullpkt_remain = _options->instuff_nullpkt;
            }

            // If input plugin returned less than expected, exit now
            if (pkt_from_input == 0) {
                return 0; // end of input, no need to return null packets
            }
            else if (pkt_in < max_packets) {
                break;
            }
        }
    }

    return pkt_done;
}


//----------------------------------------------------------------------------
// Input plugin thread
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::main()
{
    debug(u"input thread started");

    Time current_time(Time::CurrentUTC());
    Time bitrate_due_time(current_time + _options->bitrate_adj);
    bool plugin_completed = false;
    bool input_end = false;
    bool aborted = false;

    do {
        size_t pkt_first = 0;
        size_t pkt_max = 0;
        BitRate bitrate = 0;
        bool timeout = false;

        // Wait for space in the input buffer.
        // Ignore input_end and bitrate from previous, we are the input processor.
        waitWork(pkt_first, pkt_max, bitrate, input_end, aborted, timeout);

        // If the next thread has given up, give up too since our packets are now useless.
        // Do not even try to add trailing stuffing (--add-stop-stuffing).
        if (aborted) {
            break;
        }

        // In case of abort on timeout, notify previous and next plugin, then exit.
        if (timeout) {
            // Do not progate abort to previous processor since the "previous" one is the output one.
            passPackets(0, _tsp_bitrate, true, false);
            aborted = true;
            break;
        }

        // Do not read more packets than request by --max-input-packets
        if (_options->max_input_pkt > 0 && pkt_max > _options->max_input_pkt) {
            pkt_max = _options->max_input_pkt;
        }

        // Now read at most the specified number of packets (pkt_max).
        size_t pkt_read = 0;

        // Read from the plugin if not already terminated.
        if (!plugin_completed) {
            pkt_read = receiveAndStuff(_buffer->base() + pkt_first, pkt_max);
            plugin_completed = pkt_read == 0;
        }

        // Read additional trailing stuffing after completion of the input plugin.
        while (plugin_completed && _instuff_stop_remain > 0 && pkt_read < pkt_max) {
            *(_buffer->base() + pkt_first + pkt_read) = NullPacket;
            pkt_read++;
            _instuff_stop_remain--;
        }

        // Overall input is completed when input plugin and trailing stuffing are completed.
        input_end = plugin_completed && _instuff_stop_remain == 0;

        // Process periodic bitrate adjustment: get current input bitrate.
        if (_options->bitrate == 0 && (current_time = Time::CurrentUTC()) > bitrate_due_time) {
            // Compute time for next bitrate adjustment. Note that we do not
            // use a monotonic time (we use current time and not due time as
            // base for next calculation).
            bitrate_due_time = current_time + _options->bitrate_adj;
            // Call shared library to get input bitrate
            if ((bitrate = getBitrate()) > 0) {
                // Keep this bitrate
                _tsp_bitrate = bitrate;
                if (debug()) {
                    debug(u"input: got bitrate %'d b/s, next try in %'d ms", {bitrate, _options->bitrate_adj});
                }
            }
        }

        // Pass received packets to next processor
        passPackets(pkt_read, _tsp_bitrate, input_end, false);

    } while (!input_end);

    // Close the input processor
    _input->stop();

    debug(u"input thread %s after %'d packets", {aborted ? u"aborted" : u"terminated", totalPacketsInThread()});
}
