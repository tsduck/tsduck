//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for JSON reports.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsUDPSocket.h"
#include "tsTelnetConnection.h"

namespace ts {

    // Forward declarations.
    class Args;
    class DuckContext;

    namespace json {

        // Forward declarations.
        class Value;
        class RunningDocument;

        //!
        //! Command line arguments for JSON reports (@c -\-json, @c -\-json-line, @c -\-json-udp).
        //! @ingroup cmd
        //!
        class TSDUCKDLL OutputArgs
        {
            TS_NOCOPY(OutputArgs);
        public:
            //!
            //! Default constructor.
            //!
            OutputArgs() = default;

            //!
            //! Virtual destructor.
            //!
            virtual ~OutputArgs();

            //!
            //! Add command line option definitions in an Args.
            //! @param [in,out] args Command line arguments to update.
            //! @param [in] use_short_opt Define @c 'j' as short option for @c -\-json.
            //! @param [in] help Help text for option @c -\-json.
            //!
            void defineArgs(Args& args, bool use_short_opt, const UString& help = UString());

            //!
            //! Load arguments from command line.
            //! Args error indicator is set in case of incorrect arguments.
            //! @param [in,out] duck TSDuck execution context.
            //! @param [in,out] args Command line arguments.
            //! @return True on success, false on error in argument line.
            //!
            bool loadArgs(DuckContext& duck, Args& args);

            //!
            //! Check if any JSON output option is specified.
            //! @return True if any JSON output option is specified.
            //!
            bool useJSON() const { return _json_opt || _json_line || _json_tcp || _json_udp; }

            //!
            //! Check if JSON file output option is specified.
            //! @return True if JSON file output option is specified.
            //!
            bool useFile() const { return _json_opt; }

            //!
            //! Issue a JSON report according to options.
            //! @param [in] root JSON root object.
            //! @param [in] stm Output stream when @c -\-json is specified.
            //! @param [in] rep Logger to report errors or output one-line JSON when @c -\-json-line is specified.
            //! @return True on success, false on error.
            //!
            bool report(const json::Value& root, std::ostream& stm, Report& rep);

            //!
            //! Issue a JSON report according to options.
            //! @param [in] root JSON root object.
            //! @param [in] doc Output running document when @c -\-json is specified.
            //! @param [in] rep Logger to report errors or output one-line JSON when @c -\-json-line is specified.
            //! @return True on success, false on error.
            //!
            bool report(const json::Value& root, json::RunningDocument& doc, Report& rep);

        private:
            bool              _json_opt = false;      // Option --json
            bool              _json_line = false;     // Option --json-line
            bool              _json_tcp = false;      // Option --json-tcp
            bool              _json_tcp_keep = false; // Option --json-tcp-keep
            bool              _json_udp = false;      // Option --json-udp
            UString           _line_prefix {};        // Option --json-line="prefix"
            IPv4SocketAddress _tcp_destination {};    // TCP destination.
            IPv4SocketAddress _udp_destination {};    // UDP destination.
            IPv4Address       _udp_local {};          // Name of outgoing local address.
            int               _udp_ttl = 0;           // Time-to-live socket option.
            size_t            _sock_buffer_size = 0;  // Socket buffer size (TCP and UDP).
            UDPSocket         _udp_sock {};           // Output UDP socket.
            TelnetConnection  _tcp_sock {};           // Output TCP socket.

            // Issue a JSON report, except --json file.
            bool reportOthers(const json::Value& root, Report& rep);

            // Open/close the UDP socket.
            bool udpOpen(Report& rep);
            bool udpClose(Report& rep);

            // Connect/disconnect the TCP session.
            bool tcpConnect(Report& rep);
            bool tcpDisconnect(bool force, Report& rep);
        };
    }
}
