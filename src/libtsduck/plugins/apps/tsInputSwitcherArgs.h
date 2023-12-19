//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        UString             appName {};            //!< Application name, for help messages.
        bool                fastSwitch = false;    //!< Fast switch between input plugins.
        bool                delayedSwitch = false; //!< Delayed switch between input plugins.
        bool                terminate = false;     //!< Terminate when one input plugin completes.
        bool                reusePort = false;     //!< Reuse-port socket option.
        size_t              firstInput = 0;        //!< Index of first input plugin.
        size_t              primaryInput = NPOS;   //!< Index of primary input plugin, NPOS if there is none.
        size_t              cycleCount = 1;        //!< Number of input cycles to execute (0 = infinite).
        size_t              bufferedPackets = 0;   //!< Input buffer size in packets.
        size_t              maxInputPackets = 0;   //!< Maximum input packets to read at a time.
        size_t              maxOutputPackets = 0;  //!< Maximum output packets to send at a time.
        UString             eventCommand {};       //!< External shell command to run on an event.
        IPv4SocketAddress   eventUDP {};           //!< Remote UDP socket address for event description.
        IPv4Address         eventLocalAddress {};  //!< Outgoing local interface for UDP event description.
        int                 eventTTL = 0;          //!< Time-to-live socket option for event UDP.
        UString             eventUserData {};      //!< User-defined data string in event messages.
        size_t              sockBuffer = 0;        //!< Socket buffer size.
        IPv4SocketAddress   remoteServer {};       //!< UDP server address for remote control.
        IPv4AddressSet      allowedRemote {};      //!< Set of allowed remotes.
        cn::milliseconds    receiveTimeout {};     //!< Receive timeout before switch (0=none).
        PluginOptionsVector inputs {};             //!< Input plugins descriptions.
        PluginOptions       output {};             //!< Output plugin description.

        static constexpr size_t DEFAULT_MAX_INPUT_PACKETS = 128;  //!< Default maximum input packets to read at a time.
        static constexpr size_t MIN_INPUT_PACKETS = 1;            //!< Minimum input packets to read at a time.
        static constexpr size_t DEFAULT_MAX_OUTPUT_PACKETS = 128; //!< Default maximum input packets to send at a time.
        static constexpr size_t MIN_OUTPUT_PACKETS = 1;           //!< Minimum input packets to send at a time.
        static constexpr size_t DEFAULT_BUFFERED_PACKETS = 512;   //!< Default input size buffer in packets.
        static constexpr size_t MIN_BUFFERED_PACKETS = 16;        //!< Minimum input size buffer in packets.
        static constexpr cn::milliseconds DEFAULT_RECEIVE_TIMEOUT = cn::milliseconds(2000); //!< Default received timeout with --primary-input.

        //!
        //! Constructor.
        //!
        InputSwitcherArgs() = default;

        //!
        //! Enforce default or minimum values.
        //!
        void enforceDefaults();

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
