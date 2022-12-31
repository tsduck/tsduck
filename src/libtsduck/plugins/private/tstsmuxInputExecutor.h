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
//!  Multiplexer (tsmux) input plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstsmuxPluginExecutor.h"
#include "tsMuxerArgs.h"
#include "tsInputPlugin.h"

namespace ts {
    namespace tsmux {
        //!
        //! Execution context of a tsmux input plugin.
        //! @ingroup plugin
        //!
        class InputExecutor : public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(InputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] index Input plugin index.
            //! @param [in,out] log Log report.
            //!
            InputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, size_t index, Report& log);

            //!
            //! Virtual destructor.
            //!
            virtual ~InputExecutor() override;

            //!
            //! Copy packets from the input buffer.
            //! @param [out] pkt Address of packet buffer.
            //! @param [out] mdata Address of packet metadata buffer.
            //! @param [in] max_count Buffer size in number of packets.
            //! @param [out] ret_count Returned number of actual packets.
            //! @param [in] blocking If true, block until at least one packet is available.
            //! If false, immediately return with @a ret_count being zero if no packet is available.
            //! @return True on success, false if the output is terminated on error.
            //!
            bool getPackets(TSPacket* pkt, TSPacketMetadata* mdata, size_t max_count, size_t& ret_count, bool blocking);

            // Implementation of TSP.
            virtual size_t pluginIndex() const override;

            // Inherited from PluginExecutor, also abort input in progress when possible.
            virtual void terminate() override;

        private:
            InputPlugin* _input;         // Plugin API.
            const size_t _pluginIndex;   // Index of this input plugin.

            // Implementation of Thread.
            virtual void main() override;
        };
    }
}
