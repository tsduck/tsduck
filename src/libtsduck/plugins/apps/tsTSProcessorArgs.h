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
//!  Transport stream processor command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"
#include "tsIPv4Address.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Transport stream processor options and their command line options.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TSProcessorArgs
    {
    public:
        UString           app_name;         //!< Application name, for help messages.
        bool              ignore_jt;        //!< Ignore "joint termination" options in plugins.
        bool              log_plugin_index; //!< Log plugin index with plugin name.
        size_t            ts_buffer_size;   //!< Size in bytes of the global TS packet buffer.
        size_t            max_flush_pkt;    //!< Max processed packets before flush.
        size_t            max_input_pkt;    //!< Max packets per input operation.
        size_t            max_output_pkt;   //!< Max packets per outsput operation.
        size_t            init_input_pkt;   //!< Initial number of input packets to read before starting the processing (zero means default).
        size_t            instuff_nullpkt;  //!< Add input stuffing: add @a instuff_nullpkt null packets every @a instuff_inpkt input packets.
        size_t            instuff_inpkt;    //!< Add input stuffing: add @a instuff_nullpkt null packets every @a instuff_inpkt input packets.
        size_t            instuff_start;    //!< Add input stuffing: add @a instuff_start null packets before actual input.
        size_t            instuff_stop;     //!< Add input stuffing: add @a instuff_end null packets after end of actual input.
        BitRate           fixed_bitrate;    //!< Fixed input bitrate (user-specified).
        MilliSecond       bitrate_adj;      //!< Bitrate adjust interval.
        PacketCounter     init_bitrate_adj; //!< As long as input bitrate is unknown, reevaluate periodically.
        Tristate          realtime;         //!< Use real-time options.
        MilliSecond       receive_timeout;  //!< Timeout on input operations.
        MilliSecond       final_wait;       //!< Time to wait after last input packet. Zero means infinite, negative means none.
        uint16_t          control_port;     //!< TCP server port for control commands.
        IPv4Address       control_local;    //!< Local interface on which to listen for control commands.
        bool              control_reuse;    //!< Set the 'reuse port' socket option on the control TCP server port.
        IPv4AddressVector control_sources;  //!< Remote IP addresses which are allowed to send control commands.
        MilliSecond       control_timeout;  //!< Reception timeout in milliseconds for control commands.
        DuckContext::SavedArgs duck_args;   //!< Default TSDuck context options for all plugins. Each plugin can override them in its context.
        PluginOptions          input;       //!< Input plugin description.
        PluginOptionsVector    plugins;     //!< Packet processor plugins descriptions.
        PluginOptions          output;      //!< Output plugin description.

        static constexpr size_t DEFAULT_BUFFER_SIZE = 16 * 1000000;  //!< Default size in bytes of global TS buffer.
        static constexpr size_t MIN_BUFFER_SIZE = 18800;             //!< Minimum size in bytes of global TS buffer.

        //!
        //! Constructor.
        //!
        TSProcessorArgs();

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

        //!
        //! Apply default values to options which were not specified on the command line.
        //! @param [in] realtime If true, apply real-time defaults. If false, apply offline defaults.
        //!
        void applyDefaults(bool realtime);
    };
}
