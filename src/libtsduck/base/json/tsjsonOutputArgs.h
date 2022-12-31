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
            OutputArgs();

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
            bool              _json_opt;          // Option --json
            bool              _json_line;         // Option --json-line
            bool              _json_tcp;          // Option --json-tcp
            bool              _json_tcp_keep;     // Option --json-tcp-keep
            bool              _json_udp;          // Option --json-udp
            UString           _line_prefix;       // Option --json-line="prefix"
            IPv4SocketAddress _tcp_destination;   // TCP destination.
            IPv4SocketAddress _udp_destination;   // UDP destination.
            IPv4Address       _udp_local;         // Name of outgoing local address.
            int               _udp_ttl;           // Time-to-live socket option.
            size_t            _sock_buffer_size;  // Socket buffer size (TCP and UDP).
            UDPSocket         _udp_sock;          // Output UDP socket.
            TelnetConnection  _tcp_sock;          // Output TCP socket.

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
