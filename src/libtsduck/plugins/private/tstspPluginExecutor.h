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
//!  Transport stream processor: Execution context of a plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspJointTermination.h"
#include "tsRingNode.h"
#include "tsTSProcessorArgs.h"
#include "tsPluginEventHandlerRegistry.h"
#include "tsPlugin.h"
#include "tsUserInterrupt.h"
#include "tsCondition.h"
#include "tsMutex.h"
#include "tsThread.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp plugin.
        //! @ingroup plugin
        //!
        class PluginExecutor: public JointTermination, public RingNode
        {
            TS_NOBUILD_NOCOPY(PluginExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] type Plugin type.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            PluginExecutor(const TSProcessorArgs& options,
                           const PluginEventHandlerRegistry& handlers,
                           PluginType type,
                           const PluginOptions& pl_options,
                           const ThreadAttributes& attributes,
                           Mutex& global_mutex,
                           Report* report);

            //!
            //! Virtual destructor.
            //!
            virtual ~PluginExecutor() override;

            //!
            //! Set the initial state of the buffer for this plugin.
            //! Must be executed in synchronous environment, before starting all executor threads.
            //! @param [in] buffer Address of the packet buffer.
            //! @param [in] metadata Address of the packet metadata buffer.
            //! @param [in] pkt_first Starting index of packets area for this plugin.
            //! @param [in] pkt_cnt Size of packets area for this plugin.
            //! @param [in] input_end If true, there is no more packet after current ones.
            //! @param [in] aborted If true, there was a packet processor error, aborted.
            //! @param [in] bitrate Input bitrate (set by previous packet processor).
            //! @param [in] br_confidence Confidence level in @a bitrate.
            //!
            void initBuffer(PacketBuffer*         buffer,
                            PacketMetadataBuffer* metadata,
                            size_t                pkt_first,
                            size_t                pkt_cnt,
                            bool                  input_end,
                            bool                  aborted,
                            const BitRate&        bitrate,
                            BitRateConfidence     br_confidence);

            //!
            //! Inform if all plugins should use defaults for real-time.
            //! @param [in] on True if all plugins should use defaults for real-time.
            //!
            void setRealTimeForAll(bool on) { _use_realtime = on; }

            //!
            //! This method sets the current packet processor in an abort state.
            //!
            virtual void setAbort();

            //!
            //! Check if the plugin a real time one.
            //! @return True if the plugin usually requires real-time responsiveness.
            //!
            bool isRealTime() const;

            //!
            //! Set the plugin in suspended more or resume it.
            //! When suspended, a plugin no longer processes packets.
            //! The packets are passed directly from the previous to the next plugin.
            //! A plugin executor is typically suspended of resumed from another thread.
            //! @param [in] suspended When true, set the plugin in suspended mode.
            //! When false, resume the plugin if it is currently  suspended.
            //!
            void setSuspended(bool suspended) { _suspended = suspended; }

            //!
            //! Get the plugin suspension mode.
            //! @return True when the plugin is suspended, false otherwise.
            //!
            bool getSuspended() const { return _suspended; }

            //!
            //! Restart the plugin with new parameters.
            //! This method is called from another thread, not the plugin thread.
            //! @param [in] params New command line parameters.
            //! @param [in,out] report Where to report errors.
            //!
            void restart(const UStringVector& params, Report& report);

            //!
            //! Restart the plugin with same parameters.
            //! This method is called from another thread, not the plugin thread.
            //! @param [in,out] report Where to report errors.
            //!
            void restart(Report& report);

            // Implementation of TSP virtual methods.
            virtual size_t pluginCount() const override;
            virtual void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;

        protected:
            PacketBuffer*         _buffer;    //!< Description of shared packet buffer.
            PacketMetadataBuffer* _metadata;  //!< Description of shared packet metadata buffer.
            volatile bool         _suspended; //!< The plugin is suspended / resumed.

            //!
            //! Pass processed packets to the next packet processor.
            //!
            //! This method is invoked by a subclass to indicate that some packets have been
            //! processed by this plugin executor and shall be passed to the next plugin executor.
            //!
            //! If the caller thread is the output processor, the semantic of the operation is
            //! "these buffers are no longer used and can be reused by the input thread".
            //!
            //! @param [in] count Number of packets to pass to next processor.
            //! @param [in] bitrate Bitrate, as computed by this processor or passed from the previous processor.
            //! To be passed to next processor.
            //! @param [in] br_confidence Confidence level in @a bitrate. To be passed to next processor.
            //! @param [in] input_end If true, this processor will no longer produce packets.
            //! @param [in] aborted if true, this processor has encountered an error and will cease to accept packets.
            //! @return True when the processor shall continue, false when it shall stop.
            //!
            bool passPackets(size_t count, const BitRate& bitrate, BitRateConfidence br_confidence, bool input_end, bool aborted);

            //!
            //! Wait for something to do.
            //!
            //! This method is invoked by a subclass when it has nothing to do or needs more packets.
            //! This method makes the calling processor thread waiting for packets to process or some error condition.
            //!
            //! If @a min_pkt_cnt is 1, it always return a contiguous array of packets. If the circular buffer
            //! wrap-over occurs in the middle of the caller's area, it only returns the first part, up the
            //! buffer's highest address. The next call to waitWork() will return the second part.
            //!
            //! In other words, when there is no real need for a minimum number of packets, always specify
            //! @a min_pkt_cnt as 1 and you will always get a contiiguous buffer. On the other hand, if you
            //! do need a specific minimum number of packets, you must be prepared to handle the wrap-up in
            //! the middle of the returned area.
            //!
            //! @param [in] min_pkt_cnt Minimum number of packets to return. Wait until at least this number
            //! of packets can be returned in @a pkt_cnt (unless it is too large or some error occurs).
            //! @param [out] pkt_first Index of first packet to process in the buffer.
            //! @param [out] pkt_cnt Number of packets to process in the buffer.
            //! @param [out] bitrate Current bitrate, as computed from previous processors.
            //! @param [out] br_confidence Confidence level in @a bitrate, as evaluated by previous processors.
            //! @param [out] input_end The previous processor indicates that no more packets will be produced.
            //! @param [out] aborted The *next* processor indicates that it aborts and will no longer accept packets.
            //! @param [out] timeout No packet could be returned within the timeout specified by the plugin and
            //! the plugin requested an abort.
            //!
            void waitWork(size_t min_pkt_cnt, size_t& pkt_first, size_t& pkt_cnt,
                          BitRate& bitrate, BitRateConfidence& br_confidence,
                          bool& input_end, bool& aborted, bool &timeout);

            //!
            //! Check if there is a pending restart operation (but do not execute it).
            //! @return True if there is a pending restart operation.
            //!
            bool pendingRestart();

            //!
            //! Process a pending restart operation if there is one.
            //! @param [out] restarted Set to true if the plugin was restarted.
            //! @return True in case of success (no pending restart or successfully restarted)
            //! or false on fatal error (cannot even restart with the original parameters).
            //!
            bool processPendingRestart(bool& restarted);

        private:
            // Registry of plugin event handlers.
            const PluginEventHandlerRegistry& _handlers;

            // A structure which is used to handle a restart of the plugin.
            class RestartData;
            typedef SafePtr<RestartData,Mutex> RestartDataPtr;

            // The following private data must be accessed exclusively under the protection of the global mutex.
            // Implementation details: see the file src/docs/developing-plugins.dox.
            // [*] After initialization, these fields are read/written only in passPackets() and waitWork().
            Condition         _to_do;          // Notify processor to do something.
            size_t            _pkt_first;      // Starting index of packets area [*]
            size_t            _pkt_cnt;        // Size of packets area [*]
            bool              _input_end;      // No more packet after current ones [*]
            BitRate           _bitrate;        // Input bitrate (set by previous plugin) [*]
            BitRateConfidence _br_confidence;  // Input bitrate confidence (set by previous plugin) [*]
            bool              _restart;        // Restart the plugin asap using _restart_data
            RestartDataPtr    _restart_data;   // How to restart the plugin

            // Description of a restart operation.
            class RestartData
            {
                TS_NOBUILD_NOCOPY(RestartData);
            public:
                // Constructor.
                RestartData(const UStringVector& params, bool same, Report& rep);

                Report&       report;      // Report progress and error messages.
                bool          same_args;   // Use same args as previously.
                UStringVector args;        // New command line parameters for the plugin (read-only).
                Mutex         mutex;       // Mutex to protect the following fields.
                Condition     condition;   // Condition to report the end of restart (for the requesting thread).
                bool          completed;   // End of operation, restarted or aborted.
            };

            // Restart this plugin.
            void restart(const RestartDataPtr&);
        };
    }
}
