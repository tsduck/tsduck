//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Transport stream processor: Execution context of an input plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tspPluginExecutor.h"

namespace ts {
    namespace tsp {

        class InputExecutor: public PluginExecutor
        {
        public:
            // Constructor
            InputExecutor(Options* options,
                          const Options::PluginOptions* pl_options,
                          const ThreadAttributes& attributes,
                          Mutex& global_mutex);

            // Initializes the buffer for all plugin executors, starting at
            // this input executor. The buffer is pre-loaded with initial data.
            // The initial bitrate is evaluated. The buffer is propagated
            // to all executors. Must be executed in synchronous environment,
            // before starting all executor threads.
            // Return true on success, false on error.

            bool initAllBuffers (PacketBuffer*);

        private:
            InputPlugin*      _input;             // Plugin API
            const size_t      _instuff_nullpkt;   // Add input stuffing: add nullpkt null...
            const size_t      _instuff_inpkt;     // ... packets every inpkt input packets
            const BitRate     _input_bitrate;     // User-specified fixed input bitrate
            const MilliSecond _bitrate_adj;       // Bitrate adjust interval
            const size_t      _max_input_pkt;     // Max packets per input operation
            PacketCounter     _total_in_packets;  // Total packets from plugin (exclude added stuffing)
            bool              _in_sync_lost;      // Input synchronization lost (no 0x47 at start of packet)
            size_t            _instuff_nullpkt_remain;
            size_t            _instuff_inpkt_remain;

            // Inherited from Thread
            virtual void main();

            // Encapsulation of the plugin's receive() method,
            // checking the validity of the input.
            size_t receiveAndValidate (TSPacket* buffer, size_t max_packets);

            // Encapsulation of receiveAndValidate() method,
            // taking into account the tsp input stuffing options.
            size_t receiveAndStuff (TSPacket* buffer, size_t max_packets);

            // Encapsulation of the plugin's getBitrate() method,
            // taking into account the tsp input stuffing options.
            BitRate getBitrate();
        };
    }
}
