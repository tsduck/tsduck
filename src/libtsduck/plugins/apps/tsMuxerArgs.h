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
//!  Transport stream multiplexer command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Transport stream multiplexer command-line options.
    //! @ingroup plugin
    //!
    class TSDUCKDLL MuxerArgs
    {
    public:
        UString                appName;            //!< Application name, for help messages.
        PluginOptionsVector    inputs;             //!< Input plugins descriptions.
        PluginOptions          output;             //!< Output plugin description.
        BitRate                outputBitRate;      //!< Target output bitrate.
        BitRate                patBitRate;         //!< Bitrate of output PAT.
        BitRate                catBitRate;         //!< Bitrate of output CAT.
        BitRate                nitBitRate;         //!< Bitrate of output NIT.
        BitRate                sdtBitRate;         //!< Bitrate of output SDT.
        size_t                 lossyReclaim;       //!< When lossyInput is true and the input buffer is full, number of older packets to drop.
        bool                   lossyInput;         //!< If true, allow to lose input packet when the buffer is full.
        bool                   inputOnce;          //!< Terminate when all input plugins complete, do not restart plugins.
        bool                   outputOnce;         //!< Terminate when the output plugin fails, do not restart.
        bool                   ignoreConflicts;    //!< Ignore PID or service conflicts (inconsistent stream).
        MilliSecond            inputRestartDelay;  //!< When an input start fails, retry after that delay.
        MilliSecond            outputRestartDelay; //!< When the output start fails, retry after that delay.
        MicroSecond            cadence;            //!< Internal polling cadence in microseconds.
        size_t                 inBufferPackets;    //!< Input buffer size in packets.
        size_t                 outBufferPackets;   //!< Output buffer size in packets (default: N x inBufferPackets).
        size_t                 maxInputPackets;    //!< Maximum input packets to read at a time.
        size_t                 maxOutputPackets;   //!< Maximum output packets to send at a time.
        uint16_t               outputTSId;         //!< Output transport stream id.
        uint16_t               outputNetwId;       //!< Output original network id.
        TableScope             nitScope;           //!< Type of NIT to filter.
        TableScope             sdtScope;           //!< Type of SDT to filter.
        TableScope             eitScope;           //!< Type of EIT to filter.
        size_t                 timeInputIndex;     //!< Index of input plugin from which the TDT/TOT PID is used. By default, use the first found.
        DuckContext::SavedArgs duckArgs;           //!< Default TSDuck context options for all plugins. Each plugin can override them in its context.

        static constexpr size_t DEFAULT_MAX_INPUT_PACKETS = 128;      //!< Default maximum input packets to read at a time.
        static constexpr size_t MIN_INPUT_PACKETS = 1;                //!< Minimum input packets to read at a time.
        static constexpr size_t DEFAULT_MAX_OUTPUT_PACKETS = 128;     //!< Default maximum input packets to send at a time.
        static constexpr size_t MIN_OUTPUT_PACKETS = 1;               //!< Minimum input packets to send at a time.
        static constexpr size_t DEFAULT_BUFFERED_PACKETS = 512;       //!< Default input size buffer in packets.
        static constexpr size_t MIN_BUFFERED_PACKETS = 16;            //!< Minimum input size buffer in packets.
        static constexpr size_t DEFAULT_LOSSY_INPUT_PACKETS = 16;     //!< Default number of oldest input packets to drop with lossy input.
        static constexpr MilliSecond DEFAULT_RESTART_DELAY = 2000;    //!< Default input and output restart delay.
        static constexpr MicroSecond DEFAULT_CADENCE = 10000;         //!< Default cadence in microseconds.
        static constexpr BitRate::int_t MIN_PSI_BITRATE = 100;        //!< Minimum bitrate for global PSI/SI PID's.
        static constexpr BitRate::int_t DEFAULT_PSI_BITRATE = 15000;  //!< Default bitrate for global PSI/SI PID's.

        //!
        //! Constructor.
        //!
        MuxerArgs();

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
