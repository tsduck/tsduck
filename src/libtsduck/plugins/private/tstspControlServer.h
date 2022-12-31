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
#include "tsMutex.h"
#include "tsTCPServer.h"
#include "tsReportWithPrefix.h"

namespace ts {
    namespace tsp {
        //!
        //! Transport stream processor control command server.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
        //!
        class ControlServer : public CommandLineHandler, private Thread
        {
            TS_NOBUILD_NOCOPY(ControlServer);
        public:
            //!
            //! Constructor.
            //! @param [in,out] options Command line options for tsp.
            //! @param [in,out] log Log report.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in] input Input plugin executor (start of plugin chain).
            //!
            ControlServer(TSProcessorArgs& options, Report& log, Mutex& global_mutex, InputExecutor* input);

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
            volatile bool     _is_open;
            volatile bool     _terminate;
            TSProcessorArgs&  _options;
            ReportWithPrefix  _log;
            TSPControlCommand _reference;
            TCPServer         _server;
            Mutex&            _mutex;
            InputExecutor*    _input;
            OutputExecutor*   _output;
            std::vector<ProcessorExecutor*> _plugins;  // Packet processing plugins

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
}
