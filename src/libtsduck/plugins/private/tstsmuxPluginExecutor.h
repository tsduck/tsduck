//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Multiplexer (tsmux) plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginThread.h"
#include "tsMuxerArgs.h"
#include "tsPluginEventHandlerRegistry.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsMutex.h"
#include "tsCondition.h"

namespace ts {
    namespace tsmux {
        //!
        //! Execution context of a tsmux plugin.
        //! @ingroup plugin
        //!
        class PluginExecutor : public PluginThread
        {
            TS_NOBUILD_NOCOPY(PluginExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] type Plugin type.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] log Log report.
            //!
            PluginExecutor(const MuxerArgs& opt,
                           const PluginEventHandlerRegistry& handlers,
                           PluginType type,
                           const PluginOptions& pl_options,
                           const ThreadAttributes& attributes,
                           Report& log);

            //!
            //! Destructor.
            //!
            virtual ~PluginExecutor() override;

            //!
            //! Request the termination of the thread.
            //! Actual termination will occur after completion of the current input/output operation if there is one in progress.
            //!
            virtual void terminate();

            // Implementation of TSP. We do not use "joint termination" in tsmux.
            virtual void useJointTermination(bool) override;
            virtual void jointTerminate() override;
            virtual bool useJointTermination() const override;
            virtual bool thisJointTerminated() const override;
            virtual size_t pluginCount() const override;
            virtual void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;

        protected:
            const MuxerArgs&       _opt;            //!< Command line options.
            Mutex                  _mutex;          //!< Protects modifications in the buffer.
            Condition              _got_packets;    //!< Wake-up condition: there are new packets in the buffer.
            Condition              _got_freespace;  //!< Wake-up condition: there are more free packets in the buffer.
            volatile bool          _terminate;      //!< Termination request, sometimes accessed outside mutex, goes from false to true only once.
            size_t                 _packets_first;  //!< Index in the buffer of the first packet.
            size_t                 _packets_count;  //!< Number of packets to output.
            const size_t           _buffer_size;    //!< Size of the packet buffer.
            TSPacketVector         _packets;        //!< Input or output packet circular buffer.
            TSPacketMetadataVector _metadata;       //!< Input or output metadata circular buffer.

        private:
            const PluginEventHandlerRegistry& _handlers;  //!< Registry of event handlers.
        };
    }
}
