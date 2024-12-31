//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream multiplexer command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"
#include "tsAbstractTable.h"

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
        UString             appName {};            //!< Application name, for help messages.
        PluginOptionsVector inputs {};             //!< Input plugins descriptions.
        PluginOptions       output {};             //!< Output plugin description.
        BitRate             outputBitRate = 0;      //!< Target output bitrate.
        BitRate             patBitRate = DEFAULT_PSI_BITRATE;              //!< Bitrate of output PAT.
        BitRate             catBitRate = DEFAULT_PSI_BITRATE;              //!< Bitrate of output CAT.
        BitRate             nitBitRate = DEFAULT_PSI_BITRATE;              //!< Bitrate of output NIT.
        BitRate             sdtBitRate = DEFAULT_PSI_BITRATE;              //!< Bitrate of output SDT.
        size_t              lossyReclaim = DEFAULT_LOSSY_INPUT_PACKETS;    //!< When lossyInput is true and the input buffer is full, number of older packets to drop.
        bool                lossyInput = false;                            //!< If true, allow to lose input packet when the buffer is full.
        bool                inputOnce = false;                             //!< Terminate when all input plugins complete, do not restart plugins.
        bool                outputOnce = false;                            //!< Terminate when the output plugin fails, do not restart.
        bool                ignoreConflicts = false;                       //!< Ignore PID or service conflicts (inconsistent stream).
        cn::milliseconds    inputRestartDelay = DEFAULT_RESTART_DELAY;     //!< When an input start fails, retry after that delay.
        cn::milliseconds    outputRestartDelay = DEFAULT_RESTART_DELAY;    //!< When the output start fails, retry after that delay.
        cn::microseconds    cadence = DEFAULT_CADENCE;                     //!< Internal polling cadence in microseconds.
        size_t              inBufferPackets = DEFAULT_BUFFERED_PACKETS;    //!< Input buffer size in packets.
        size_t              outBufferPackets = DEFAULT_BUFFERED_PACKETS;   //!< Output buffer size in packets (default: N x inBufferPackets).
        size_t              maxInputPackets = DEFAULT_MAX_INPUT_PACKETS;   //!< Maximum input packets to read at a time.
        size_t              maxOutputPackets = DEFAULT_MAX_OUTPUT_PACKETS; //!< Maximum output packets to send at a time.
        uint16_t            outputTSId = 0;                 //!< Output transport stream id.
        uint16_t            outputNetwId = 0;               //!< Output original network id.
        TableScope          nitScope = TableScope::ACTUAL;  //!< Type of NIT to filter.
        TableScope          sdtScope = TableScope::ACTUAL;  //!< Type of SDT to filter.
        TableScope          eitScope = TableScope::ACTUAL;  //!< Type of EIT to filter.
        size_t              timeInputIndex = NPOS;          //!< Index of input plugin from which the TDT/TOT PID is used. By default, use the first found.
        DuckContext::SavedArgs duckArgs {};                 //!< Default TSDuck context options for all plugins. Each plugin can override them in its context.

        static constexpr size_t DEFAULT_MAX_INPUT_PACKETS = 128;      //!< Default maximum input packets to read at a time.
        static constexpr size_t MIN_INPUT_PACKETS = 1;                //!< Minimum input packets to read at a time.
        static constexpr size_t DEFAULT_MAX_OUTPUT_PACKETS = 128;     //!< Default maximum input packets to send at a time.
        static constexpr size_t MIN_OUTPUT_PACKETS = 1;               //!< Minimum input packets to send at a time.
        static constexpr size_t DEFAULT_BUFFERED_PACKETS = 512;       //!< Default input size buffer in packets.
        static constexpr size_t MIN_BUFFERED_PACKETS = 16;            //!< Minimum input size buffer in packets.
        static constexpr size_t DEFAULT_LOSSY_INPUT_PACKETS = 16;     //!< Default number of oldest input packets to drop with lossy input.
        static constexpr cn::milliseconds DEFAULT_RESTART_DELAY = cn::seconds(2);  //!< Default input and output restart delay.
        static constexpr cn::microseconds DEFAULT_CADENCE = cn::milliseconds(10);  //!< Default cadence.
        static constexpr BitRate::int_t MIN_PSI_BITRATE = 100;        //!< Minimum bitrate for global PSI/SI PID's.
        static constexpr BitRate::int_t DEFAULT_PSI_BITRATE = 15000;  //!< Default bitrate for global PSI/SI PID's.

        //!
        //! Constructor.
        //!
        MuxerArgs() = default;

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
