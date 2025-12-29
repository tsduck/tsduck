//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        //! @ingroup libtsduck plugin
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
