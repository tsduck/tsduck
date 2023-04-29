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

#include "tstspInputExecutor.h"
#include "tsTime.h"

// Minimum number of PID's and PCR/DTS to analyze before getting a valid bitrate.
#define MIN_ANALYZE_PID   1
#define MIN_ANALYZE_PCR  32
#define MIN_ANALYZE_DTS  32


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tsp::InputExecutor::InputExecutor(const TSProcessorArgs& options,
                                      const PluginEventHandlerRegistry& handlers,
                                      const PluginOptions& pl_options,
                                      const ThreadAttributes& attributes,
                                      Mutex& global_mutex,
                                      Report* report) :

    PluginExecutor(options, handlers, PluginType::INPUT, pl_options, attributes, global_mutex, report),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _in_sync_lost(false),
    _plugin_completed(false),
    _instuff_start_remain(options.instuff_start),
    _instuff_stop_remain(options.instuff_stop),
    _instuff_nullpkt_remain(0),
    _instuff_inpkt_remain(0),
    _pcr_analyzer(MIN_ANALYZE_PID, MIN_ANALYZE_PCR),
    _dts_analyzer(),
    _use_dts_analyzer(false),
    _watchdog(this, options.receive_timeout, 0, *this),
    _use_watchdog(false),
    _start_time(true) // initialized with current system time
{
    if (options.log_plugin_index) {
        // Make sure that plugins display their index. Input plugin is always at index 0.
        setLogName(UString::Format(u"%s[0]", {pluginName()}));
    }

    // Configure PTS/DTS analyze
    _dts_analyzer.resetAndUseDTS(MIN_ANALYZE_PID, MIN_ANALYZE_DTS);

    // Propose receive timeout to input plugin.
    if (options.receive_timeout > 0 && !_input->setReceiveTimeout(options.receive_timeout)) {
        debug(u"%s input plugin does not support receive timeout, using watchdog and abort", {pluginName()});
        _use_watchdog = true;
    }
}

ts::tsp::InputExecutor::~InputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Initializes the buffer for all plugin executors.
//----------------------------------------------------------------------------

bool ts::tsp::InputExecutor::initAllBuffers(PacketBuffer* buffer, PacketMetadataBuffer* metadata)
{
    // Pre-declare buffer for input plugin.
    initBuffer(buffer, metadata, 0, buffer->count(), false, false, 0, BitRateConfidence::LOW);

    // Pre-load half of the buffer (the default) with packets from the input device.
    const size_t init_packets = _options.init_input_pkt == 0 ? buffer->count() / 2 : std::min(_options.init_input_pkt, buffer->count());
    const size_t pkt_read = receiveAndStuff(0, init_packets);

    if (pkt_read == 0) {
        debug(u"no initial packet read");
        return false; // receive error
    }

    debug(u"initial buffer load: %'d packets, %'d bytes", {pkt_read, pkt_read * PKT_SIZE});

    // Try to evaluate the initial input bitrate.
    BitRate init_bitrate = 0;
    BitRateConfidence init_confidence = BitRateConfidence::LOW;
    getBitrate(init_bitrate, init_confidence);

    if (init_bitrate == 0) {
        verbose(u"unknown initial input bitrate");
    }
    else {
        verbose(u"initial input bitrate is %'d b/s", {init_bitrate});
    }

    // Indicate that the loaded packets are now available to the next packet processor.
    PluginExecutor* next = ringNext<PluginExecutor>();
    next->initBuffer(buffer, metadata, 0, pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate, init_confidence);

    // The rest of the buffer belongs to this input processor for reading additional packets.
    initBuffer(buffer, metadata, pkt_read % buffer->count(), buffer->count() - pkt_read, pkt_read == 0, pkt_read == 0, init_bitrate, init_confidence);

    // All other processors have an implicit empty buffer (_pkt_first and _pkt_cnt are zero).
    // Propagate initial input bitrate to all processors
    while ((next = next->ringNext<PluginExecutor>()) != this) {
        next->initBuffer(buffer, metadata, 0, 0, pkt_read == 0, pkt_read == 0, init_bitrate, init_confidence);
    }

    return true;
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's getBitrate() method or PCR analysis.
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::getBitrate(BitRate& bitrate, BitRateConfidence& confidence)
{
    if (_options.fixed_bitrate > 0) {
        // Get bitrate from --bitrate command line option. It takes precedence over all.
        bitrate = _options.fixed_bitrate;
        confidence = BitRateConfidence::OVERRIDE;
    }
    else {
        // Get bitrate from plugin.
        bitrate = _input->getBitrate();
        confidence = _input->getBitrateConfidence();
    }

    if (bitrate != 0) {
        // Got a bitrate value from command line or plugin.
        if (_options.instuff_inpkt != 0) {
            // Need to adjust with artificial input stuffing.
            bitrate = (bitrate * (_options.instuff_nullpkt + _options.instuff_inpkt)) / _options.instuff_inpkt;
        }
    }
    else if (!_use_dts_analyzer && _pcr_analyzer.bitrateIsValid()) {
        // Got a bitrate from the PCR's, continuously re-evaluated.
        bitrate = _pcr_analyzer.bitrate188();
        confidence = BitRateConfidence::PCR_CONTINUOUS;
    }
    else {
        // Still no bitrate available from PCR, try DTS from video PID's.
        // If DTS are used at least once, continue to use them all the time.
        _use_dts_analyzer = _use_dts_analyzer || _dts_analyzer.bitrateIsValid();
        // Return the bitrate from DTS.
        bitrate = _use_dts_analyzer ? _dts_analyzer.bitrate188() : 0;
        confidence = BitRateConfidence::PCR_CONTINUOUS;
    }
}


//----------------------------------------------------------------------------
// This method sets the current processor in an abort state.
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::setAbort()
{
    // Call the superclass to place the executor in an abort state.
    PluginExecutor::setAbort();

    // Abort current input operation if still blocked.
    if (_input != nullptr) {
        _input->abortInput();
    }
}


//----------------------------------------------------------------------------
// Implementation of TSP: return the packet index in the chain.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::pluginIndex() const
{
    // An input plugin is always first.
    return 0;
}


//----------------------------------------------------------------------------
// Implementation of WatchDogHandlerInterface
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::handleWatchDogTimeout(WatchDog& watchdog)
{
    debug(u"receive timeout, aborting");
    if (_input != nullptr && !_input->abortInput()) {
        warning(u"failed to abort input on receive timeout, maybe not supported by this plugin");
    }
}


//----------------------------------------------------------------------------
// Receive null packets.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveNullPackets(size_t index, size_t max_packets)
{
    TSPacket* const pkt = _buffer->base() + index;
    TSPacketMetadata* const data = _metadata->base() + index;

    // Fill the buffer with null packets.
    for (size_t n = 0; n < max_packets; ++n) {
        pkt[n] = NullPacket;
        _pcr_analyzer.feedPacket(pkt[n]);
        _dts_analyzer.feedPacket(pkt[n]);
        data[n].reset();
        data[n].setInputStuffing(true);
    }

    // Count those packets as not coming from the real input plugin.
    addNonPluginPackets(max_packets);
    return max_packets;
}


//----------------------------------------------------------------------------
// Encapsulation of the plugin's receive() method,
// checking the validity of the input.
//----------------------------------------------------------------------------

size_t ts::tsp::InputExecutor::receiveAndValidate(size_t index, size_t max_packets)
{
    // If synchronization lost, report an error
    if (_in_sync_lost) {
        return 0;
    }

    TSPacket* const pkt = _buffer->base() + index;
    TSPacketMetadata* const data = _metadata->base() + index;

    // Reset metadata for new incoming packets.
    for (size_t n = 0; n < max_packets; ++n) {
        data[n].reset();
    }

    // Invoke the plugin receive method
    if (_use_watchdog) {
        _watchdog.restart();
    }
    size_t count = _input->receive(pkt, data, max_packets);
    _plugin_completed = _plugin_completed || count == 0;
    if (_use_watchdog) {
        _watchdog.suspend();
    }

    // Fill input time stamps with monotonic clock if none was provided by the input plugin.
    // Only check the first returned packet. Assume that the input plugin generates time stamps for all or none.
    if (count > 0 && !data[0].hasInputTimeStamp()) {
        const NanoSecond current = Monotonic(true) - _start_time;
        for (size_t n = 0; n < count; ++n) {
            data[n].setInputTimeStamp(current, NanoSecPerSec, TimeSource::TSP);
        }
    }

    // Validate sync byte (0x47) at beginning of each packet
    for (size_t n = 0; n < count; ++n) {
        if (pkt[n].hasValidSync()) {
            // Count good packets from plugin
            addPluginPackets(1);

            // Include packet in bitrate analysis.
            _pcr_analyzer.feedPacket(pkt[n]);
            _dts_analyzer.feedPacket(pkt[n]);
        }
        else {
            // Report error
            error(u"synchronization lost after %'d packets, got 0x%X instead of 0x%X", {pluginPackets(), pkt[n].b[0], SYNC_BYTE});
            // In debug mode, partial dump of input
            // (one packet before lost of sync and 3 packets starting at lost of sync).
            if (maxSeverity() >= 1) {
                if (n > 0) {
                    debug(u"content of packet before loss of synchronization:\n%s",
                          {UString::Dump(pkt[n-1].b, PKT_SIZE, UString::HEXA | UString::OFFSET | UString::ASCII | UString::BPL, 4, 16)});
                }
                const size_t dump_count = std::min<size_t>(3, count - n);
                debug(u"data at loss of synchronization:\n%s",
                      {UString::Dump(pkt[n].b, dump_count * PKT_SIZE, UString::HEXA | UString::OFFSET | UString::ASCII | UString::BPL, 4, 16)});
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

size_t ts::tsp::InputExecutor::receiveAndStuff(size_t index, size_t max_packets)
{
    size_t pkt_done = 0;              // Number of received packets in buffer
    size_t pkt_remain = max_packets;  // Remaining number of packets to read

    // If initial stuffing not yet completed, add initial stuffing.
    while (_instuff_start_remain > 0 && pkt_remain > 0) {
        _buffer->base()[index] = NullPacket;
        _metadata->base()[index].reset();
        _metadata->base()[index].setInputStuffing(true);
        _instuff_start_remain--;
        index++;
        pkt_remain--;
        pkt_done++;
        addNonPluginPackets(1);
    }

    // Now read real packets.
    if (_options.instuff_inpkt == 0) {
        // There is no --add-input-stuffing option, simply call the plugin
        if (pkt_remain > 0) {
            pkt_done += receiveAndValidate(index, pkt_remain);
        }
    }
    else {
        // Otherwise, we have to alternate input packets and null packets.
        while (pkt_remain > 0) {

            // Stuff null packets.
            size_t count = receiveNullPackets(index, std::min(_instuff_nullpkt_remain, pkt_remain));
            _instuff_nullpkt_remain -= count;
            index += count;
            pkt_remain -= count;
            pkt_done += count;

            // Exit on buffer full.
            if (pkt_remain == 0) {
                break;
            }

            // Restart sequence of input packets to read after reading intermediate null packets.
            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_inpkt_remain = _options.instuff_inpkt;
            }

            // Read input packets from the plugin
            const size_t max_input = std::min(pkt_remain, _instuff_inpkt_remain);
            count = receiveAndValidate(index, max_input);
            index += count;
            pkt_remain -= count;
            pkt_done += count;
            _instuff_inpkt_remain -= count;

            // Restart sequence of null packets to stuff after reading chunk of input packets.
            if (_instuff_nullpkt_remain == 0 && _instuff_inpkt_remain == 0) {
                _instuff_nullpkt_remain = _options.instuff_nullpkt;
            }

            // If input plugin returned less than expected, exit now
            if (count < max_input) {
                break;
            }
        }
    }
    return pkt_done;
}


//----------------------------------------------------------------------------
// Encapsulation of passPackets().
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::passInputPackets(size_t pkt_count, bool input_end)
{
    // At end of input with --final-wait, wait before reporting the end of input.
    if (input_end && _options.final_wait >= 0) {
        // If there are some packets, report them without end-of-input before waiting.
        if (pkt_count > 0) {
            passPackets(pkt_count, _tsp_bitrate, _tsp_bitrate_confidence, false, false);
            pkt_count = 0;
        }
        // Wait the specified number of milliseconds or forever if zero.
        debug(u"final wait after end of input: %'d ms", {_options.final_wait});
        if (_options.final_wait > 0) {
            SleepThread(_options.final_wait);
        }
        else {
            // Wait forever. Repeatedly use long waits (one day) to avoid system limitations.
            for (;;) {
                SleepThread(MilliSecPerDay);
            }
        }
        debug(u"end of final wait");
    }

    // Do not progate abort to previous processor since the "previous" one is the output one.
    passPackets(pkt_count, _tsp_bitrate, _tsp_bitrate_confidence, input_end, false);
}



//----------------------------------------------------------------------------
// Input plugin thread
//----------------------------------------------------------------------------

void ts::tsp::InputExecutor::main()
{
    debug(u"input thread started");

    Time current_time(Time::CurrentUTC());
    Time bitrate_due_time(current_time + _options.bitrate_adj);
    PacketCounter bitrate_due_packet = _options.init_bitrate_adj;
    bool input_end = false;
    bool aborted = false;
    bool restarted = false;
    _plugin_completed = false;

    do {
        size_t pkt_first = 0;
        size_t pkt_max = 0;
        BitRate bitrate = 0;
        BitRateConfidence br_confidence = BitRateConfidence::LOW;
        bool timeout = false;

        // Wait for space in the input buffer.
        // Ignore input_end and bitrate from previous, we are the input processor.
        waitWork(1, pkt_first, pkt_max, bitrate, br_confidence, input_end, aborted, timeout);

        // Process restart requests.
        if (!processPendingRestart(restarted)) {
            timeout = true; // restart error
        }

        // If the next thread has given up, give up too since our packets are now useless.
        // Do not even try to add trailing stuffing (--add-stop-stuffing).
        if (aborted) {
            break;
        }

        // In case of abort on timeout, notify previous and next plugin, then exit.
        if (timeout) {
            passInputPackets(0, true);
            aborted = true;
            break;
        }

        // Do not read more packets than request by --max-input-packets
        if (_options.max_input_pkt > 0 && pkt_max > _options.max_input_pkt) {
            pkt_max = _options.max_input_pkt;
        }

        // Now read at most the specified number of packets (pkt_max).
        size_t pkt_read = 0;

        // Read from the plugin if not already terminated.
        if (!_plugin_completed) {
            pkt_read = receiveAndStuff(pkt_first, pkt_max);
        }

        // Read additional trailing stuffing after completion of the input plugin.
        if (_plugin_completed && _instuff_stop_remain > 0 && pkt_read < pkt_max) {
            const size_t count = receiveNullPackets(pkt_first + pkt_read, std::min(_instuff_stop_remain, pkt_max - pkt_read));
            pkt_read += count;
            _instuff_stop_remain -= count;
        }

        // Overall input is completed when input plugin and trailing stuffing are completed.
        input_end = _plugin_completed && _instuff_stop_remain == 0;

        // Process periodic bitrate adjustment.
        // In initial phase, as long as the bitrate is unknown, retry every init_bitrate_adj packets.
        // Once the bitrate is known, retry every bitrate_adj milliseconds.
        if (_options.fixed_bitrate == 0 && ((_tsp_bitrate == 0 && pluginPackets() >= bitrate_due_packet) || (current_time = Time::CurrentUTC()) > bitrate_due_time)) {

            // When bitrate is unknown, retry in a fixed amount of packets.
            if (_tsp_bitrate == 0) {
                do {
                    bitrate_due_packet += _options.init_bitrate_adj;
                } while (bitrate_due_packet <= pluginPackets());
            }

            // Compute time for next bitrate adjustment. Note that we do not use a monotonic time
            // (we use current time and not due time as base for next calculation).
            if (current_time >= bitrate_due_time) {
                bitrate_due_time = current_time + _options.bitrate_adj;
            }

            // Call shared library to get input bitrate
            getBitrate(bitrate, br_confidence);

            if (bitrate > 0) {
                // Keep this bitrate
                _tsp_bitrate = bitrate;
                _tsp_bitrate_confidence = br_confidence;
                debug(u"input: got bitrate %'d b/s", {bitrate});
            }
        }

        // Pass received packets to next processor
        passInputPackets(pkt_read, input_end);

    } while (!input_end);

    // Close the input processor.
    debug(u"stopping the input plugin");
    _input->stop();

    debug(u"input thread %s after %'d packets", {aborted ? u"aborted" : u"terminated", totalPacketsInThread()});
}
