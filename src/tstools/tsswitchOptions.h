//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  Input switch (tsswitch) command-line options.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsWithPlugins.h"
#include "tsSocketAddress.h"

namespace ts {
    //!
    //! Input switch (tsswitch) namespace.
    //!
    namespace tsswitch {
        //!
        //! Input switch (tsswitch) command-line options.
        //! @ingroup plugin
        //!
        class Options: public ArgsWithPlugins
        {
            TS_NOBUILD_NOCOPY(Options);
        public:
            bool          fastSwitch;        //!< Fast switch between input plugins.
            bool          delayedSwitch;     //!< Delayed switch between input plugins.
            bool          terminate;         //!< Terminate when one input plugin completes.
            bool          monitor;           //!< Run a resource monitoring thread.
            bool          logTimeStamp;      //!< Add time stamps in log messages.
            bool          logSynchronous;    //!< Synchronous log.
            bool          reusePort;         //!< Reuse-port socket option.
            size_t        firstInput;        //!< Index of first input plugin.
            size_t        primaryInput;      //!< Index of primary input plugin, NPOS if there is none.
            size_t        cycleCount;        //!< Number of input cycles to execute.
            size_t        logMaxBuffer;      //!< Maximum buffered log messages.
            size_t        bufferedPackets;   //!< Input buffer size in packets.
            size_t        maxInputPackets;   //!< Maximum input packets to read at a time.
            size_t        maxOutputPackets;  //!< Maximum input packets to read at a time.
            size_t        sockBuffer;        //!< Socket buffer size.
            SocketAddress remoteServer;      //!< UDP server addres for remote control.
            IPAddressSet  allowedRemote;     //!< Set of allowed remotes.
            MilliSecond   receiveTimeout;    //!< Receive timeout before switch (0=none).

            //!
            //! Constructor.
            //! @param [in] argc Number of arguments from command line.
            //! @param [in] argv Arguments from command line.
            //!
            Options(int argc, char *argv[]);

            //!
            //! Virtual destructor.
            //!
            virtual ~Options();
        };
    }
}
