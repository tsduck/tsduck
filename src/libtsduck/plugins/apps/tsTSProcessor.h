//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

        Report&               _report;                     // Common log object.
        std::recursive_mutex  _global_mutex {};            // Global mutex.
        volatile bool         _terminating = false;        // In the process of terminating everything.
        TSProcessorArgs       _args {};                    // Processing options.
        tsp::InputExecutor*   _input = nullptr;            // Input processor execution thread.
        tsp::OutputExecutor*  _output = nullptr;           // Output processor execution thread.
        tsp::ControlServer*   _control = nullptr;          // TSP control command server thread.
        PacketBuffer*         _packet_buffer = nullptr;    // Global TS packet buffer.
        PacketMetadataBuffer* _metadata_buffer = nullptr;  // Global packet metabata buffer.

        // Deallocate and cleanup internal resources.
        void cleanupInternal();
    };
}
