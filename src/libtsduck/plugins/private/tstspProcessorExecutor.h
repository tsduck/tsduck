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
//!  Transport stream processor: Execution context of a packet processor plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginExecutor.h"
#include "tsProcessorPlugin.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp packet processor plugin.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
        //!
        class ProcessorExecutor: public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(ProcessorExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] plugin_index Index of command line options for this plugin in @a options.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            ProcessorExecutor(const TSProcessorArgs& options,
                              const PluginEventHandlerRegistry& handlers,
                              size_t plugin_index,
                              const ThreadAttributes& attributes,
                              Mutex& global_mutex,
                              Report* report);

            //!
            //! Virtual destructor.
            //!
            virtual ~ProcessorExecutor() override;

            // Overridden methods.
            virtual size_t pluginIndex() const override;

        private:
            ProcessorPlugin* _processor;
            const size_t     _plugin_index;

            // Inherited from Thread
            virtual void main() override;

            // Process packets one by one or using packet windows.
            void processIndividualPackets();
            void processPacketWindows(size_t window_size);
        };
    }
}
