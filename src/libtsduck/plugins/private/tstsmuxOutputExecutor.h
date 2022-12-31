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
//!  Multiplexer (tsmux) output plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstsmuxPluginExecutor.h"
#include "tsMuxerArgs.h"
#include "tsOutputPlugin.h"

namespace ts {
    namespace tsmux {
        //!
        //! Execution context of a tsmux output plugin.
        //! @ingroup plugin
        //!
        class OutputExecutor : public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(OutputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in,out] log Log report.
            //!
            OutputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log);

            //!
            //! Virtual destructor.
            //!
            virtual ~OutputExecutor() override;

            //!
            //! Copy packets in the output buffer.
            //! @param [in] pkt Address of first packet to send.
            //! @param [in] mdata Address of first packet metadata to send.
            //! @param [in] count Number of packets to send.
            //! @return True on success, false if the output is terminated on error.
            //!
            bool send(const TSPacket* pkt, const TSPacketMetadata* mdata, size_t count);

            // Implementation of TSP.
            virtual size_t pluginIndex() const override;

        private:
            OutputPlugin* _output;  // Plugin API.

            // Implementation of Thread.
            virtual void main() override;
        };
    }
}
