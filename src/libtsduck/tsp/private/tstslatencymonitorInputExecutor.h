//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Latency monitor (tslatencymonitor) input plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginThread.h"
#include "tsLatencyMonitorArgs.h"
#include "tsInputPlugin.h"

namespace ts {

    class LatencyMonitor;

    namespace tslatencymonitor {
        //!
        //! Execution context of a tslatencymonitor input plugin.
        //! @ingroup libtsduck plugin
        //!
        class InputExecutor: public tsp::PluginThread
        {
            TS_NOBUILD_NOCOPY(InputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] tsp_core Shared pointer to the transport stream processor internal core API.
            //! @param [in] opt Command line options.
            //! @param [in] index Input plugin index.
            //! @param [in,out] monitor Monitor instance
            //!
            InputExecutor(const std::shared_ptr<tsp::Core>& tsp_core, const LatencyMonitorArgs& opt, size_t index, LatencyMonitor& monitor);

            //!
            //! Virtual destructor.
            //!
            ~InputExecutor() override;

            // Implementation of TSP.
            size_t pluginCount() const override;
            void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;
            size_t pluginIndex() const override;

            //!
            //! Terminate the input executor thread.
            //!
            void terminateInput();

        private:
            LatencyMonitor&        _monitor;      // Monitor core instance
            InputPlugin*           _input;        // Plugin API.
            const size_t           _plugin_index; // Index of this input plugin.
            const size_t           _plugin_count; // Count of total plugin
            TSPacketVector         _buffer;       // Packet buffer.
            TSPacketMetadataVector _metadata;     // Packet metadata.

            static constexpr size_t BUFFERED_PACKETS = 512; // Input size buffer in packets.

            // Implementation of Thread.
            void main() override;
        };
    }
}
