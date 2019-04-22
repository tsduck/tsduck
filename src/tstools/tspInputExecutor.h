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
//!
//!  @file
//!  Transport stream processor: Execution context of an input plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tspPluginExecutor.h"
#include "tsPCRAnalyzer.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp input plugin.
        //! @ingroup plugin
        //!
        class InputExecutor: public PluginExecutor
        {
        public:
            //!
            //! Constructor.
            //! @param [in,out] options Command line options for tsp.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //!
            InputExecutor(Options* options,
                          const PluginOptions* pl_options,
                          const ThreadAttributes& attributes,
                          Mutex& global_mutex);

            //!
            //! Initializes the packet buffer for all plugin executors, starting at this input executor.
            //!
            //! The buffer is pre-loaded with initial data.
            //! The initial bitrate is evaluated.
            //! The buffer is propagated to all executors.
            //!
            //! Must be executed in synchronous environment, before starting all executor threads.
            //!
            //! @param [out] buffer Packet buffer address.
            //! @return True on success, false on error.
            //!
            bool initAllBuffers(PacketBuffer* buffer);

        private:
            InputPlugin* _input;                  // Plugin API
            bool         _in_sync_lost;           // Input synchronization lost (no 0x47 at start of packet)
            size_t       _instuff_start_remain;
            size_t       _instuff_stop_remain;
            size_t       _instuff_nullpkt_remain;
            size_t       _instuff_inpkt_remain;
            PCRAnalyzer  _pcr_analyzer;           // Compute input bitrate from PCR's.
            PCRAnalyzer  _dts_analyzer;           // Compute input bitrate from video DTS's.
            bool         _use_dts_analyzer;       // Use DTS analyzer, not PCR analyzer.

            // Inherited from Thread
            virtual void main() override;

            // Receive null packets.
            size_t receiveNullPackets(TSPacket* buffer, size_t max_packets);

            // Encapsulation of the plugin's receive() method, checking the validity of the input.
            size_t receiveAndValidate(TSPacket* buffer, size_t max_packets);

            // Encapsulation of receiveAndValidate() method, adding tsp input stuffing options.
            size_t receiveAndStuff(TSPacket* buffer, size_t max_packets);

            // Encapsulation of the plugin's getBitrate() method, taking into account the tsp input
            // stuffing options. Use PCR analysis if bitrate not otherwise available.
            BitRate getBitrate();

            // Inaccessible operations
            InputExecutor() = delete;
            InputExecutor(const InputExecutor&) = delete;
            InputExecutor& operator=(const InputExecutor&) = delete;
        };
    }
}
