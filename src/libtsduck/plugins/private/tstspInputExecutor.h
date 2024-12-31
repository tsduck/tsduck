//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor: Execution context of an input plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginExecutor.h"
#include "tsInputPlugin.h"
#include "tsPCRAnalyzer.h"
#include "tsWatchDog.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp input plugin.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
        //!
        class InputExecutor: public PluginExecutor, private WatchDogHandlerInterface
        {
            TS_NOBUILD_NOCOPY(InputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            InputExecutor(const TSProcessorArgs& options,
                          const PluginEventHandlerRegistry& handlers,
                          const PluginOptions& pl_options,
                          const ThreadAttributes& attributes,
                          std::recursive_mutex& global_mutex,
                          Report* report);

            //!
            //! Virtual destructor.
            //!
            virtual ~InputExecutor() override;

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
            //! @param [out] metadata Address of the packet metadata buffer.
            //! @return True on success, false on error.
            //!
            bool initAllBuffers(PacketBuffer* buffer, PacketMetadataBuffer* metadata);

            // Overridden methods.
            virtual void setAbort() override;
            virtual size_t pluginIndex() const override;

        private:
            InputPlugin*   _input = nullptr;           // Plugin API
            bool           _in_sync_lost = false;      // Input synchronization lost (no 0x47 at start of packet)
            bool           _plugin_completed = false;  // Input plugin reported termination.
            size_t         _instuff_start_remain = 0;
            size_t         _instuff_stop_remain = 0;
            size_t         _instuff_nullpkt_remain = 0;
            size_t         _instuff_inpkt_remain = 0;
            PCRAnalyzer    _pcr_analyzer {};           // Compute input bitrate from PCR's.
            PCRAnalyzer    _dts_analyzer {};           // Compute input bitrate from video DTS's.
            bool           _use_dts_analyzer = false;  // Use DTS analyzer, not PCR analyzer.
            WatchDog       _watchdog {};               // Watchdog when plugin does not support receive timeout.
            bool           _use_watchdog = false;      // The watchdog shall be used.
            monotonic_time _start_time {monotonic_time::clock::now()}; // Creation time, initialized with current system time.

            // Inherited from Thread
            virtual void main() override;

            // Implementation of WatchDogHandlerInterface
            virtual void handleWatchDogTimeout(WatchDog& watchdog) override;

            // Receive null packets.
            size_t receiveNullPackets(size_t index, size_t max_packets);

            // Encapsulation of the plugin's receive() method, checking the validity of the input.
            size_t receiveAndValidate(size_t index, size_t max_packets);

            // Encapsulation of receiveAndValidate() method, adding tsp input stuffing options.
            size_t receiveAndStuff(size_t index, size_t max_packets);

            // Encapsulation of the plugin's getBitrate() method, taking into account the tsp input
            // stuffing options. Use PCR analysis if bitrate not otherwise available.
            void getBitrate(BitRate& bitrate, BitRateConfidence& confidence);

            // Encapsulation of passPackets().
            void passInputPackets(size_t pkt_count, bool input_end);
        };
    }
}
