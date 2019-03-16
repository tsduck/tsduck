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
//  Transport stream processor: Execution context of a packet processor plugin
//
//----------------------------------------------------------------------------

#include "tspProcessorExecutor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::tsp::ProcessorExecutor::ProcessorExecutor(Options* options,
                                              const PluginOptions* pl_options,
                                              const ThreadAttributes& attributes,
                                              Mutex& global_mutex) :

    PluginExecutor(options, pl_options, attributes, global_mutex),
    _processor(dynamic_cast<ProcessorPlugin*>(PluginThread::plugin()))
{
}


//----------------------------------------------------------------------------
// Packet processor plugin thread
//----------------------------------------------------------------------------

void ts::tsp::ProcessorExecutor::main()
{
    debug(u"packet processing thread started");

    PacketCounter passed_packets = 0;
    PacketCounter dropped_packets = 0;
    PacketCounter nullified_packets = 0;
    BitRate output_bitrate = _tsp_bitrate;
    bool bitrate_never_modified = true;
    bool input_end = false;
    bool aborted = false;

    do {
        // Wait for packets to process

        size_t pkt_first = 0;
        size_t pkt_cnt = 0;
        waitWork(pkt_first, pkt_cnt, _tsp_bitrate, input_end, aborted);

        // If bit rate was never modified by the plugin, always copy the
        // input bitrate as output bitrate. Otherwise, keep previous
        // output bitrate, as modified by the plugin.

        if (bitrate_never_modified) {
            output_bitrate = _tsp_bitrate;
        }

        // If next processor has aborted, abort as well.
        // We call passPacket to inform our predecessor that we aborted.

        if (aborted) {
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

            bool flush_request = false;
            TSPacket* pkt = _buffer->base() + pkt_first + pkt_done;

            pkt_done++;
            pkt_flush++;

            if (pkt->b[0] == 0) {
                // The packet has already been dropped by a previous packet processor.
                addNonPluginPackets(1);
            }
            else {
                // Apply the processing routine to the packet
                bool bitrate_changed = false;
                ProcessorPlugin::Status status = _processor->processPacket(*pkt, flush_request, bitrate_changed);
                addPluginPackets(1);

                // Use the returned status
                switch (status) {
                    case ProcessorPlugin::TSP_OK:
                        // Normal case, pass packet
                        passed_packets++;
                        break;
                    case ProcessorPlugin::TSP_NULL:
                        // Replace the packet with a complete null packet
                        *pkt = NullPacket;
                        nullified_packets++;
                        break;
                    case ProcessorPlugin::TSP_DROP:
                        // Drop this packet.
                        pkt->b[0] = 0;
                        dropped_packets++;
                        break;
                    case ProcessorPlugin::TSP_END:
                        // Signal end of input to successors and abort
                        // to predecessors
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

                // If the packet processor has signaled a new bitrate, get it.
                if (bitrate_changed) {
                    BitRate new_bitrate = _processor->getBitrate();
                    if (new_bitrate != 0) {
                        bitrate_never_modified = false;
                        output_bitrate = new_bitrate;
                    }
                }
            }

            // Do not wait to process pkt_cnt packets before notifying
            // the next processor. Perform periodic flush to avoid waiting
            // too long before two output operations.

            if (flush_request || pkt_done == pkt_cnt || (_options->max_flush_pkt > 0 && pkt_flush % _options->max_flush_pkt == 0)) {
                aborted = !passPackets(pkt_flush, output_bitrate, pkt_done == pkt_cnt && input_end, aborted);
                pkt_flush = 0;
            }
        }

    } while (!input_end && !aborted);

    // Close the packet processor
    _processor->stop();

    debug(u"packet processing thread %s after %'d packets, %'d passed, %'d dropped, %'d nullified",
          {aborted ? u"aborted" : u"terminated", pluginPackets(), passed_packets, dropped_packets, nullified_packets});
}
