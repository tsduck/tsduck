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
//!  Transport stream input switcher command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"
#include "tsIPv4SocketAddress.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Transport stream input switcher command-line options.
    //! @ingroup plugin
    //!
    class TSDUCKDLL InputSwitcherArgs
    {
    public:
        UString             appName;           //!< Application name, for help messages.
        bool                fastSwitch;        //!< Fast switch between input plugins.
        bool                delayedSwitch;     //!< Delayed switch between input plugins.
        bool                terminate;         //!< Terminate when one input plugin completes.
        bool                reusePort;         //!< Reuse-port socket option.
        size_t              firstInput;        //!< Index of first input plugin.
        size_t              primaryInput;      //!< Index of primary input plugin, NPOS if there is none.
        size_t              cycleCount;        //!< Number of input cycles to execute (0 = infinite).
        size_t              bufferedPackets;   //!< Input buffer size in packets.
        size_t              maxInputPackets;   //!< Maximum input packets to read at a time.
        size_t              maxOutputPackets;  //!< Maximum output packets to send at a time.
        UString             eventCommand;      //!< External shell command to run on an event.
        IPv4SocketAddress   eventUDP;          //!< Remote UDP socket address for event description.
        IPv4Address         eventLocalAddress; //!< Outgoing local interface for UDP event description.
        int                 eventTTL;          //!< Time-to-live socket option for event UDP.
        UString             eventUserData;     //!< User-defined data string in event messages.
        size_t              sockBuffer;        //!< Socket buffer size.
        IPv4SocketAddress   remoteServer;      //!< UDP server address for remote control.
        IPv4AddressSet      allowedRemote;     //!< Set of allowed remotes.
        MilliSecond         receiveTimeout;    //!< Receive timeout before switch (0=none).
        PluginOptionsVector inputs;            //!< Input plugins descriptions.
        PluginOptions       output;            //!< Output plugin description.

        static constexpr size_t      DEFAULT_MAX_INPUT_PACKETS = 128;  //!< Default maximum input packets to read at a time.
        static constexpr size_t      MIN_INPUT_PACKETS = 1;            //!< Minimum input packets to read at a time.
        static constexpr size_t      DEFAULT_MAX_OUTPUT_PACKETS = 128; //!< Default maximum input packets to send at a time.
        static constexpr size_t      MIN_OUTPUT_PACKETS = 1;           //!< Minimum input packets to send at a time.
        static constexpr size_t      DEFAULT_BUFFERED_PACKETS = 512;   //!< Default input size buffer in packets.
        static constexpr size_t      MIN_BUFFERED_PACKETS = 16;        //!< Minimum input size buffer in packets.
        static constexpr MilliSecond DEFAULT_RECEIVE_TIMEOUT = 2000;   //!< Default received timeout with --primary-input.

        //!
        //! Constructor.
        //!
        InputSwitcherArgs();

        //!
        //! Enforce default or minimum values.
        //!
        void enforceDefaults();

        //!
        //! Set the UDP destination for event reporting using strings.
        //! @param [in] destination Remote UDP socket address for event description. Empty to erase the destination.
        //! @param [in] local Outgoing local interface for UDP event description. Can be empty.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on failure.
        //!
        bool setEventUDP(const UString& destination, const UString& local, Report& report);

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);
    };
}
