//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tstspProcessorExecutor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::tsp::ProcessorExecutor::ProcessorExecutor(const TSProcessorArgs& options,
                                              const PluginEventHandlerRegistry& handlers,
                                              size_t plugin_index,
                                              const ThreadAttributes& attributes,
                                              Mutex& global_mutex,
                                              Report* report) :

    PluginExecutor(options, handlers, PluginType::PROCESSOR, options.plugins[plugin_index], attributes, global_mutex, report),
    _processor(dynamic_cast<ProcessorPlugin*>(PluginThread::plugin())),
    _plugin_index(1 + plugin_index) // include first input plugin in the count
{
    if (options.log_plugin_index) {
        // Make sure that plugins display their index.
        setLogName(UString::Format(u"%s[%d]", {pluginName(), _plugin_index}));
    }
}

ts::tsp::ProcessorExecutor::~ProcessorExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP: return the packet index in the chain.
//----------------------------------------------------------------------------

size_t ts::tsp::ProcessorExecutor::pluginIndex() const
{
    return _plugin_index;
}


//----------------------------------------------------------------------------
// Packet processor plugin thread
//----------------------------------------------------------------------------

void ts::tsp::ProcessorExecutor::main()
{
    debug(u"packet processing thread started");

    // Debug feature: if the environment variable TSP_FORCED_WINDOW_SIZE is
    // defined to some non-zero integer value, force all plugins to use the
    // packet window processing method. This can be used to check that using
    // this method does not break a plugin or tsp itself.
    size_t window_size = 0;
    if (!GetEnvironment(u"TSP_FORCED_WINDOW_SIZE").toInteger(window_size)) {
        window_size = 0; // invalid value, reset
    }

    // Get and apply the processing method.
    if (window_size == 0) {
        window_size = _processor->getPacketWindowSize();
    }
    if (window_size == 0) {
        processIndividualPackets();
    }
    else {
        processPacketWindows(window_size);
    }

    // Close the packet processor
    _processor->stop();
}


//----------------------------------------------------------------------------
// Process packets one by one.
//----------------------------------------------------------------------------

void ts::tsp::ProcessorExecutor::processIndividualPackets()
{
    TSPacketMetadata::LabelSet only_labels(_processor->getOnlyLabelOption());
    PacketCounter passed_packets = 0;
    PacketCounter dropped_packets = 0;
    PacketCounter nullified_packets = 0;
    BitRate output_bitrate = _tsp_bitrate;
    bool bitrate_never_modified = true;
    bool input_end = false;
    bool aborted = false;
    bool restarted = false;

    do {
        // Wait for packets to process
        size_t pkt_first = 0;
        size_t pkt_cnt = 0;
        bool timeout = false;
        waitWork(1, pkt_first, pkt_cnt, _tsp_bitrate, input_end, aborted, timeout);

        // If bitrate was never modified by the plugin, always copy the input bitrate as output bitrate.
        // Otherwise, keep previous output bitrate, as modified by the plugin.
        if (bitrate_never_modified) {
            output_bitrate = _tsp_bitrate;
        }

        // Process restart requests.
        if (!processPendingRestart(restarted)) {
            timeout = true; // restart error
        }
        else if (restarted) {
            // Plugin was restarted, need to recheck --only-label
            only_labels = _processor->getOnlyLabelOption();
        }

        // In case of abort on timeout, notify previous and next plugin, then exit.
        if (timeout) {
            passPackets(0, output_bitrate, true, true);
            break;
        }

        // If next processor has aborted, abort as well.
        // We call passPacket to inform our predecessor that we aborted.
        if (aborted && !input_end) {
            passPackets(0, output_bitrate, true, true);
            break;
        }

        // Exit thread if no more packet to process.
        // We call passPackets to inform our successor of end of input.
        if (pkt_cnt == 0 && input_end) {
            passPackets(0, output_bitrate, true, false);
            break;
        }

        // Now process the packets.
        size_t pkt_done = 0;
        size_t pkt_flush = 0;

        while (pkt_done < pkt_cnt && !aborted) {

            TSPacket* const pkt = _buffer->base() + pkt_first + pkt_done;
            TSPacketMetadata* const pkt_data = _metadata->base() + pkt_first + pkt_done;

            pkt_done++;
            pkt_flush++;

            if (pkt->b[0] == 0) {
                // The packet has already been dropped by a previous packet processor.
                addNonPluginPackets(1);
            }
            else {
                // Apply the processing routine to the packet
                const bool was_null = pkt->getPID() == PID_NULL;
                pkt_data->setFlush(false);
                pkt_data->setBitrateChanged(false);
                ProcessorPlugin::Status status = ProcessorPlugin::TSP_OK;
                if (!_suspended && (only_labels.none() || pkt_data->hasAnyLabel(only_labels))) {
                    // Either no --only-label option or the packet has a specified label => process it.
                    status = _processor->processPacket(*pkt, *pkt_data);
                    addPluginPackets(1);
                }
                else {
                    // The plugin is suspended or some --only-label was specified but the packet does
                    // not have any required label. Pass the packet without submitting it to the plugin.
                    addNonPluginPackets(1);
                }

                // Use the returned status
                switch (status) {
                    case ProcessorPlugin::TSP_OK:
                        // Normal case, pass packet
                        passed_packets++;
                        break;
                    case ProcessorPlugin::TSP_NULL:
                        // Replace the packet with a complete null packet
                        *pkt = NullPacket;
                        break;
                    case ProcessorPlugin::TSP_DROP:
                        // Drop this packet.
                        pkt->b[0] = 0;
                        dropped_packets++;
                        break;
                    case ProcessorPlugin::TSP_END:
                        // Signal end of input to successors and abort to predecessors
                        debug(u"plugin requests termination");
                        input_end = aborted = true;
                        pkt_done--;
                        pkt_flush--;
                        pkt_cnt = pkt_done;
                        break;
                    default:
                        // Invalid status, report error and accept packet.
                        error(u"invalid packet processing status %d", {status});
                        break;
                }

                // Detect if the packet was nullified by the plugin, either by returning TSP_NULL or by overwriting the packet.
                if (!was_null && pkt->getPID() == PID_NULL) {
                    pkt_data->setNullified(true);
                    nullified_packets++;
                }

                // If the packet processor has signaled a new bitrate, get it.
                if (pkt_data->getBitrateChanged()) {
                    const BitRate new_bitrate = _processor->getBitrate();
                    if (new_bitrate != 0) {
                        bitrate_never_modified = false;
                        output_bitrate = new_bitrate;
                    }
                }
            }

            // Do not wait to process pkt_cnt packets before notifying
            // the next processor. Perform periodic flush to avoid waiting
            // too long before two output operations.

            if (pkt_data->getFlush() || pkt_done == pkt_cnt || (_options.max_flush_pkt > 0 && pkt_flush % _options.max_flush_pkt == 0)) {
                aborted = !passPackets(pkt_flush, output_bitrate, pkt_done == pkt_cnt && input_end, aborted);
                pkt_flush = 0;
            }
        }

    } while (!input_end && !aborted);

    debug(u"packet processing thread %s after %'d packets, %'d passed, %'d dropped, %'d nullified",
          {input_end ? u"terminated" : u"aborted", pluginPackets(), passed_packets, dropped_packets, nullified_packets});
}


//----------------------------------------------------------------------------
// Process packets using packet windows.
//----------------------------------------------------------------------------

void ts::tsp::ProcessorExecutor::processPacketWindows(size_t window_size)
{
    debug(u"packet processing window size: %'d packets", {window_size});

    TSPacketMetadata::LabelSet only_labels(_processor->getOnlyLabelOption());
    PacketCounter passed_packets = 0;
    PacketCounter dropped_packets = 0;
    PacketCounter nullified_packets = 0;
    BitRate output_bitrate = _tsp_bitrate;
    bool bitrate_never_modified = true;
    bool input_end = false;
    bool aborted = false;
    bool timeout = false;
    bool restarted = false;

    // Loop on packet processing.
    do {
        // Wait for a part of the buffer which is large enough for the packet window.
        // Initially, we request the window size. But maybe not all packets can be used
        // in the returned area: maybe there are dropped packets or excluded packets
        // when --only-label is used. Compute haw many packets are missing and restart
        // the request with that many more packets. But again, some of the the additional
        // packets may be excluded. So, restart again and again until we get 'window_size'
        // usable packets.
        TSPacketWindow::PacketRangeVector packet_ranges;
        size_t request_packets = window_size;  // number of packets to request in the buffer.
        size_t first_packet_index = 0;         // index of first allocated packet in the global buffer.
        size_t allocated_packets = 0;          // number of allocated packet from the global buffer.
        size_t packets_in_window = 0;          // accumulated packets in packet window.

        // Loop on building a large enough packet window.
        while (!aborted && !input_end && !timeout) {

            // Restart building a packet window.
            packet_ranges.clear();
            packets_in_window = 0;

            // Wait for packets to process.
            waitWork(request_packets, first_packet_index, allocated_packets, _tsp_bitrate, input_end, aborted, timeout);

            // If bitrate was never modified by the plugin, always copy the input bitrate as output bitrate.
            // Otherwise, keep previous output bitrate, as modified by the plugin.
            if (bitrate_never_modified) {
                output_bitrate = _tsp_bitrate;
            }

            // Process restart requests.
            if (!processPendingRestart(restarted)) {
                timeout = true; // restart error
            }
            else if (restarted) {
                // Plugin was restarted, need to recheck --only-label and window size.
                // Don't let window size be zero, we are in packet window mode.
                only_labels = _processor->getOnlyLabelOption();
                window_size = std::max<size_t>(1, _processor->getPacketWindowSize());
            }

            // If the plugin is suspended, simply pass the packets to the next plugin.
            if (_suspended) {
                // Drop all packets which are owned by this plugin.
                addNonPluginPackets(allocated_packets);
                passPackets(allocated_packets, output_bitrate, input_end, aborted);
                // Continue building a packet window (the plugin maybe resumed in the meantime).
                continue;
            }

            // Inspect the packets we got from the buffer (pkt_first / pkt_count) and insert usable packets in the packet window.
            // Take care that waitWork() may have returned a slice of the buffer which wraps up.
            TSPacket* pkt = _buffer->base() + first_packet_index;
            TSPacketMetadata* pkt_data = _metadata->base() + first_packet_index;
            size_t range_first = first_packet_index;
            size_t range_cnt = 0;
            for (size_t i = 0; i < allocated_packets; ++i) {
                if (pkt->b[0] == 0 || (only_labels.any() && !pkt_data->hasAnyLabel(only_labels))) {
                    // Packet was previously dropped or its label is not in --only-label.
                    // Close previous range.
                    if (range_cnt > 0) {
                        packet_ranges.push_back({_buffer->base() + range_first, _metadata->base() + range_first, range_cnt});
                        packets_in_window += range_cnt;
                    }
                    // Next range will start at next packet.
                    range_first = (first_packet_index + i + 1) % _buffer->count();
                    range_cnt = 0;
                }
                else {
                    // Packet shall be included in current range.
                    range_cnt++;
                    pkt_data->setBitrateChanged(false);
                }
                // Point to next packet.
                if (i + 1 < allocated_packets && first_packet_index + i + 1 < _buffer->count()) {
                    // Not the end of the returned area, not the end of the buffer, simply move to next packet.
                    pkt++;
                    pkt_data++;
                }
                else {
                    // Wrap up at beginning of buffer.
                    pkt = _buffer->base();
                    pkt_data = _metadata->base();
                    // If current range is not yet included, do it now.
                    if (range_cnt > 0) {
                        packet_ranges.push_back({_buffer->base() + range_first, _metadata->base() + range_first, range_cnt});
                        packets_in_window += range_cnt;
                    }
                    // Next range will restart at beginning of buffer.
                    range_first = 0;
                    range_cnt = 0;
                }
            }

            // Stop when we have enough packets in the window.
            if (packets_in_window >= window_size || allocated_packets < request_packets) {
                // Either we have enough packets or waitWork() returned less than the requested minimum (meaning more is impossible).
                break;
            }

            // Add the number of missing packets.
            request_packets += window_size - packets_in_window;
        }

        // A packet window is ready to be processed. Build the packet window.
        TSPacketWindow win(packet_ranges);
        assert(win.size() == packets_in_window);

        // Let the plugin process the packet window.
        const size_t processed_packets = _processor->processPacketWindow(win);

        // If not all packets from the window were processed, the plugin want to terminate the stream processing.
        if (processed_packets < win.size()) {
            input_end = aborted = true;
            // We shall not pass packets after the last processed one to next plugin.
            // The number of processed packets is an index after the last "logical" packet in the window.
            // This is not an index from 'first_packet_index'. We compute in 'allocated_packets' the
            // number of allocated packets up to the last processed one (inclusive).
            if (processed_packets == 0) {
                allocated_packets = 0;
            }
            else {
                // Physical index in buffer of last processed packet:
                const size_t index = win.packetIndexInBuffer(processed_packets - 1, _buffer->base(), _buffer->count());
                assert(index < _buffer->count());
                if (index >= first_packet_index) {
                    // Contiguous range.
                    allocated_packets = (index - first_packet_index) + 1;
                }
                else {
                    // Two parts, wrap-up at end of buffer.
                    allocated_packets = (_buffer->count() - first_packet_index) + index + 1;
                }
            }
        }

        // Count packets which were processed in the plugin.
        passed_packets += processed_packets - win.dropCount();
        dropped_packets += win.dropCount();
        nullified_packets += win.nullifyCount();
        addPluginPackets(processed_packets);
        addNonPluginPackets(allocated_packets - processed_packets);

        // Check if the plugin reported a new bitrate.
        for (size_t i = 0; i < std::min(processed_packets, win.size()); ++i) {
            TSPacketMetadata* mdata = win.metadata(i);
            if (mdata != nullptr && mdata->getBitrateChanged()) {
                const BitRate new_bitrate = _processor->getBitrate();
                if (new_bitrate != 0) {
                    bitrate_never_modified = false;
                    output_bitrate = new_bitrate;
                }
                break;
            }
        }

        // In case of timeout on waiting for packets, abort this plugin.
        if (timeout) {
            aborted = true;
        }

        // Pass all allocated packets to the next plugin.
        // Can be less than actually allocated in case of termination.
        passPackets(allocated_packets, output_bitrate, input_end, aborted);

    } while (!input_end && !aborted);

    debug(u"packet processing thread %s after %'d packets, %'d passed, %'d dropped, %'d nullified",
          {input_end ? u"terminated" : u"aborted", pluginPackets(), passed_packets, dropped_packets, nullified_packets});
}
