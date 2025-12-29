//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input switch (tsswitch) event dispatcher.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputSwitcherArgs.h"
#include "tsUDPSocket.h"
#include "tsjsonObject.h"

namespace ts {
    namespace tsswitch {
        //!
        //! Input switch (tsswitch) event dispatcher.
        //! @ingroup libtsduck plugin
        //!
        class EventDispatcher
        {
            TS_NOBUILD_NOCOPY(EventDispatcher);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in,out] log Log report.
            //!
            EventDispatcher(const InputSwitcherArgs& opt, Report& log);

            //!
            //! Signal a "new input" event.
            //! @param [in] oldPluginIndex Index of the input plugin before the switch.
            //! @param [in] newPluginIndex Index of the input plugin after the switch.
            //! @return True on success, false on error.
            //!
            bool signalNewInput(size_t oldPluginIndex, size_t newPluginIndex);

        private:
            const InputSwitcherArgs& _opt;
            Report&   _log;
            bool      _sendCommand;
            bool      _sendUDP;
            UString   _userData;
            UDPSocket _socket;

            // Send command and UDP message.
            bool sendCommand(const UString& eventName, const UString& otherParameters = UString());
            bool sendUDP(const UString& eventName, json::Object& object);
        };
    }
}
