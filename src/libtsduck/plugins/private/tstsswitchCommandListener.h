//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input switch (tsswitch) remote control command receiver.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputSwitcherArgs.h"
#include "tsThread.h"
#include "tsUDPReceiver.h"
#include "tsTLSServer.h"

namespace ts {
    namespace tsswitch {

        class Core;
        class Options;

        //!
        //! Input switch (tsswitch) remote control command receiver.
        //! @ingroup libtsduck plugin
        //!
        class CommandListener : private Thread
        {
            TS_NOBUILD_NOCOPY(CommandListener);
        public:
            //!
            //! Constructor.
            //! @param [in,out] core Command core instance.
            //! @param [in] opt Command line options.
            //! @param [in,out] log Log report.
            //!
            CommandListener(Core& core, const InputSwitcherArgs& opt, Report& log);

            //!
            //! Destructor.
            //!
            virtual ~CommandListener() override;

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
            Report&       _log;
            Core&         _core;
            const InputSwitcherArgs& _opt;
            UDPReceiver   _udp_server {_log};
            TLSServer     _tls_server {_opt.remote_control};
            TLSConnection _tls_client {_opt.remote_control};
            volatile bool _terminate = false;

            // Implementation of Thread.
            virtual void main() override;

            // Execute a remote command.
            bool execute(const IPSocketAddress& source, const UString& command);
        };
    }
}
