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
        //! @ingroup plugin
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
