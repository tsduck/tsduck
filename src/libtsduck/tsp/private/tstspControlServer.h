//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor control command server.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSProcessorArgs.h"
#include "tstspInputExecutor.h"
#include "tstspProcessorExecutor.h"
#include "tstspOutputExecutor.h"
#include "tsTSPControlCommand.h"
#include "tsThread.h"
#include "tsTLSServer.h"

namespace ts::tsp {
    //!
    //! Transport stream processor control command server.
    //! This class is internal to the TSDuck library and cannot be called by applications.
    //! @ingroup libtsduck plugin
    //!
    class ControlServer: public CommandLineHandler, private Thread
    {
        TS_NOBUILD_NOCOPY(ControlServer);
    public:
        //!
        //! Constructor.
        //! @param [in] core Shared pointer to the transport stream processor internal core API.
        //! @param [in] tsp_options Command line options for tsp.
        //! @param [in] input Input plugin executor (start of plugin chain).
        //!
        ControlServer(const std::shared_ptr<tsp::Core>& core, TSProcessorArgs& tsp_options, InputExecutor* input);

        //!
        //! Destructor.
        //!
        virtual ~ControlServer() override;

        //!
        //! Open and start the command listener.
        //! @return True on success, false on error.
        //!
        bool open();

        //!
        //! Stop and close the command listener.
        //!
        void close();

    private:
        std::shared_ptr<tsp::Core> _core;
        TSProcessorArgs&  _tsp_options;
        std::atomic_bool  _is_open = false;
        std::atomic_bool  _terminate = false;
        Report            _log;
        TSPControlCommand _reference {_log};
        TCPServer         _tcp_server {&_log};
        TLSServer         _tls_server {&_log, _tsp_options.control};
        TLSConnection     _tls_client {&_log, _tsp_options.control};
        InputExecutor*    _input = nullptr;
        OutputExecutor*   _output = nullptr;
        std::vector<ProcessorExecutor*> _plugins {};  // Packet processing plugins

        // Implementation of Thread.
        virtual void main() override;

        // Command handlers.
        CommandStatus executeExit(const UString&, Args&);
        CommandStatus executeSetLog(const UString&, Args&);
        CommandStatus executeList(const UString&, Args&);
        void listOnePlugin(size_t index, UChar type, PluginExecutor* plugin, Report& report);
        CommandStatus executeSuspend(const UString&, Args&);
        CommandStatus executeResume(const UString&, Args&);
        CommandStatus executeSuspendResume(bool state, Args&);
        CommandStatus executeRestart(const UString&, Args&);
    };
}
