//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of the TS multiplexer (command tsmux).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerRegistry.h"
#include "tsMuxerArgs.h"

namespace ts {

    // Used in private part.
    namespace tsmux {
        class Core;
    }

    //!
    //! Implementation of the TS multiplexer.
    //! This class is used by the @a tsmux utility.
    //! It can also be used in other applications to multiplex input streams.
    //! @ingroup plugin
    //!
    class TSDUCKDLL Muxer: public PluginEventHandlerRegistry
    {
        TS_NOBUILD_NOCOPY(Muxer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        Muxer(Report& report);

        //!
        //! Destructor.
        //! It waits for termination of the session if it is running.
        //!
        ~Muxer();

        //!
        //! Get a reference to the report object for the multiplexer.
        //! @return A reference to the report object.
        //!
        Report& report() const { return _report; }

        //!
        //! Start the multiplexer session.
        //! @param [in] args Arguments and options.
        //! @return True on success, false on failure to start.
        //!
        bool start(const MuxerArgs& args);

        //!
        //! Stop the multiplexer.
        //!
        void stop();

        //!
        //! Suspend the calling thread until the multiplexer is completed.
        //!
        void waitForTermination();

    private:
        Report&      _report;
        MuxerArgs    _args;
        tsmux::Core* _core;
    };
}
