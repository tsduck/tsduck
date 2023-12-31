//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
