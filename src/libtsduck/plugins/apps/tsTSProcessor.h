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
//!  Core of the TSP Transport Stream Processor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerRegistry.h"
#include "tsTSProcessorArgs.h"
#include "tsTSPacketMetadata.h"
#include "tsMutex.h"

namespace ts {

    // Forward class declaration for private part.
    //! @cond nodoxygen
    namespace tsp {
        class InputExecutor;
        class OutputExecutor;
        class ControlServer;
    }
    //! @endcond

    //!
    //! Core of the TSP Transport Stream Processor.
    //! This class is used by the @a tsp utility.
    //! It can also be used in other applications to run a chain of plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TSProcessor: public PluginEventHandlerRegistry
    {
        TS_NOBUILD_NOCOPY(TSProcessor);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        TSProcessor(Report& report);

        //!
        //! Destructor.
        //! It waits for termination of the TS processing if it is running.
        //!
        ~TSProcessor();

        //!
        //! Get a reference to the report object for the TS processor.
        //! @return A reference to the report object for the TS processor.
        //!
        Report& report() const { return _report; }

        //!
        //! Start the TS processing.
        //! @param [in] args Arguments and options.
        //! @return True on success, false on failure to start.
        //!
        bool start(const TSProcessorArgs& args);

        //!
        //! Check if the TS processing is started.
        //! @return True if the processing is in progress, false otherwise.
        //!
        bool isStarted();

        //!
        //! Abort the processing.
        //! The method can be invoked from any thread, including an interrupt handler for instance.
        //!
        void abort();

        //!
        //! Suspend the calling thread until TS processing is completed.
        //!
        void waitForTermination();

    private:
        // There is one global mutex for protected operations.
        // The resulting bottleneck of this single mutex is acceptable as long
        // as all protected operations are fast (pointer update, simple arithmetic).

        Report&               _report;           // Common log object.
        Mutex                 _mutex;            // Global mutex.
        volatile bool         _terminating;      // In the process of terminating everything.
        TSProcessorArgs       _args;             // Processing options.
        tsp::InputExecutor*   _input;            // Input processor execution thread.
        tsp::OutputExecutor*  _output;           // Output processor execution thread.
        tsp::ControlServer*   _control;          // TSP control command server thread.
        PacketBuffer*         _packet_buffer;    // Global TS packet buffer.
        PacketMetadataBuffer* _metadata_buffer;  // Global packet metabata buffer.

        // Deallocate and cleanup internal resources.
        void cleanupInternal();
    };
}
