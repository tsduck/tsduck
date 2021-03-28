//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Transport stream multiplexer command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsSupplierInterface.h"
#include "tsPluginOptions.h"

namespace ts {
    //!
    //! Transport stream multiplexer command-line options.
    //! @ingroup plugin
    //!
    class TSDUCKDLL MuxerArgs: public ArgsSupplierInterface
    {
    public:
        UString             appName;            //!< Application name, for help messages.
        PluginOptionsVector inputs;             //!< Input plugins descriptions.
        PluginOptions       output;             //!< Output plugin description.
        bool                lossyInput;         //!< If true, allow to lose input packet when the buffer is full.
        bool                inputOnce;          //!< Terminate when all input plugins complete, do not restart plugins.
        bool                outputOnce;         //!< Terminate when the output plugin fails, do not restart.
        MilliSecond         inputRestartDelay;  //!< When an input start fails, retry after that delay.
        MilliSecond         outputRestartDelay; //!< When the output start fails, retry after that delay.
        size_t              inBufferPackets;    //!< Input buffer size in packets.
        size_t              outBufferPackets;   //!< Output buffer size in packets (default: N x inBufferPackets).
        size_t              maxInputPackets;    //!< Maximum input packets to read at a time.
        size_t              maxOutputPackets;   //!< Maximum output packets to send at a time.

        static constexpr size_t DEFAULT_MAX_INPUT_PACKETS = 128;    //!< Default maximum input packets to read at a time.
        static constexpr size_t MIN_INPUT_PACKETS = 1;              //!< Minimum input packets to read at a time.
        static constexpr size_t DEFAULT_MAX_OUTPUT_PACKETS = 128;   //!< Default maximum input packets to send at a time.
        static constexpr size_t MIN_OUTPUT_PACKETS = 1;             //!< Minimum input packets to send at a time.
        static constexpr size_t DEFAULT_BUFFERED_PACKETS = 512;     //!< Default input size buffer in packets.
        static constexpr size_t MIN_BUFFERED_PACKETS = 16;          //!< Minimum input size buffer in packets.
        static constexpr MilliSecond DEFAULT_RESTART_DELAY = 2000;  //!< Default input and output restart delay.

        //!
        //! Constructor.
        //!
        MuxerArgs();

        //!
        //! Enforce default or minimum values.
        //!
        void enforceDefaults();

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;
    };
}
