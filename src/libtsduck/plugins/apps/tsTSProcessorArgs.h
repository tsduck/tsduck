//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"
#include "tsIPAddress.h"

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
        UString           app_name {};              //!< Application name, for help messages.
        bool              ignore_jt = false;        //!< Ignore "joint termination" options in plugins.
        bool              log_plugin_index = false; //!< Log plugin index with plugin name.
        size_t            ts_buffer_size = DEFAULT_BUFFER_SIZE; //!< Size in bytes of the global TS packet buffer.
        size_t            max_flush_pkt = 0;        //!< Max processed packets before flush.
        size_t            max_input_pkt = 0;        //!< Max packets per input operation.
        size_t            max_output_pkt = NPOS;    //!< Max packets per outsput operation. NPOS means unlimited.
        size_t            init_input_pkt = 0;       //!< Initial number of input packets to read before starting the processing (zero means default).
        size_t            instuff_nullpkt = 0;      //!< Add input stuffing: add @a instuff_nullpkt null packets every @a instuff_inpkt input packets.
        size_t            instuff_inpkt = 0;        //!< Add input stuffing: add @a instuff_nullpkt null packets every @a instuff_inpkt input packets.
        size_t            instuff_start = 0;        //!< Add input stuffing: add @a instuff_start null packets before actual input.
        size_t            instuff_stop = 0;         //!< Add input stuffing: add @a instuff_end null packets after end of actual input.
        BitRate           fixed_bitrate = 0;        //!< Fixed input bitrate (user-specified).
        cn::milliseconds  bitrate_adj = DEFAULT_BITRATE_INTERVAL; //!< Bitrate adjust interval.
        PacketCounter     init_bitrate_adj = DEFAULT_INIT_BITRATE_PKT_INTERVAL; //!< As long as input bitrate is unknown, reevaluate periodically.
        Tristate          realtime = Tristate::Maybe; //!< Use real-time options.
        cn::milliseconds  receive_timeout {}; //!< Timeout on input operations.
        cn::milliseconds  final_wait = cn::milliseconds(-1); //!< Time to wait after last input packet. Zero means infinite, negative means none.
        uint16_t          control_port = 0;         //!< TCP server port for control commands.
        IPAddress         control_local {};         //!< Local interface on which to listen for control commands.
        bool              control_reuse = false;    //!< Set the 'reuse port' socket option on the control TCP server port.
        IPAddressVector   control_sources {};       //!< Remote IP addresses which are allowed to send control commands.
        cn::milliseconds  control_timeout = DEFAULT_CONTROL_TIMEOUT; //!< Reception timeout in milliseconds for control commands.
        DuckContext::SavedArgs duck_args {};        //!< Default TSDuck context options for all plugins. Each plugin can override them in its context.
        PluginOptions          input {};            //!< Input plugin description.
        PluginOptionsVector    plugins {};          //!< Packet processor plugins descriptions.
        PluginOptions          output {};           //!< Output plugin description.

        static constexpr size_t DEFAULT_BUFFER_SIZE = 16 * 1000000;               //!< Default size in bytes of global TS buffer.
        static constexpr size_t MIN_BUFFER_SIZE = 18800;                          //!< Minimum size in bytes of global TS buffer.
        static constexpr PacketCounter DEFAULT_INIT_BITRATE_PKT_INTERVAL = 1000;  //!< Default initial bitrate reevaluation interval, in packets.
        static constexpr cn::milliseconds DEFAULT_BITRATE_INTERVAL = cn::milliseconds(5000);  //!< Default bitrate adjustment interval, in milliseconds.
        static constexpr cn::milliseconds DEFAULT_CONTROL_TIMEOUT = cn::milliseconds(5000);   //!< Default control command reception timeout, in milliseconds.

        //!
        //! Constructor.
        //!
        TSProcessorArgs() = default;

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
