//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
#include "tsArgsSupplierInterface.h"
#include "tsPluginOptions.h"
#include "tsSocketAddress.h"

namespace ts {
    //!
    //! Transport stream input switcher command-line options.
    //! @ingroup plugin
    //!
    class TSDUCKDLL InputSwitcherArgs: public ArgsSupplierInterface
    {
    public:
        UString             appName;           //!< Application name, for help messages.
        bool                fastSwitch;        //!< Fast switch between input plugins.
        bool                delayedSwitch;     //!< Delayed switch between input plugins.
        bool                terminate;         //!< Terminate when one input plugin completes.
        bool                monitor;           //!< Run a resource monitoring thread.
        bool                reusePort;         //!< Reuse-port socket option.
        size_t              firstInput;        //!< Index of first input plugin.
        size_t              primaryInput;      //!< Index of primary input plugin, NPOS if there is none.
        size_t              cycleCount;        //!< Number of input cycles to execute.
        size_t              bufferedPackets;   //!< Input buffer size in packets.
        size_t              maxInputPackets;   //!< Maximum input packets to read at a time.
        size_t              maxOutputPackets;  //!< Maximum input packets to send at a time.
        size_t              sockBuffer;        //!< Socket buffer size.
        SocketAddress       remoteServer;      //!< UDP server addres for remote control.
        IPAddressSet        allowedRemote;     //!< Set of allowed remotes.
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
        //! Copy constructor.
        //! Default or minimum values are enforced.
        //! @param [in] other instance to copy.
        //!
        InputSwitcherArgs(const InputSwitcherArgs& other);

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;
    };
}
