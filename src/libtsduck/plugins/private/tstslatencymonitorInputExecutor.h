//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Latency monitor (tslatencymonitor) input plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginThread.h"
#include "tsLatencyMonitorArgs.h"
#include "tsInputPlugin.h"

namespace ts {

    class LatencyMonitor;

    namespace tslatencymonitor {
        //!
        //! Execution context of a tslatencymonitor input plugin.
        //! @ingroup plugin
        //!
        class InputExecutor : public PluginThread
        {
            TS_NOBUILD_NOCOPY(InputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] index Input plugin index.
            //! @param [in,out] monitor Monitor instance
            //! @param [in,out] log Log report.
            //!
            InputExecutor(const LatencyMonitorArgs& opt,
                          size_t index,
                          LatencyMonitor& monitor,
                          Report& log);

            //!
            //! Virtual destructor.
            //!
            ~InputExecutor() override;

            // Implementation of TSP. We do not use "joint termination" in tslatencymonitor.
            void useJointTermination(bool) override;
            void jointTerminate() override;
            bool useJointTermination() const override;
            bool thisJointTerminated() const override;
            size_t pluginCount() const override;
            void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;
            size_t pluginIndex() const override;

            //!
            //! Terminate the input executor thread.
            //!
            void terminateInput();

        private:
            LatencyMonitor&        _monitor;     // Monitor core instance
            InputPlugin*           _input;       // Plugin API.
            const size_t           _pluginIndex; // Index of this input plugin.
            const size_t           _pluginCount; // Count of total plugin
            TSPacketVector         _buffer;      // Packet buffer.
            TSPacketMetadataVector _metadata;    // Packet metadata.

            static constexpr size_t BUFFERED_PACKETS = 512; // Input size buffer in packets.

            // Implementation of Thread.
            void main() override;
        };
    }
}
