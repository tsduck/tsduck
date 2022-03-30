//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2022, Thierry Lelegard
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
#include "tsArgsSupplierInterface.h"
#include "tsUString.h"
#include "tsUDPSocket.h"

namespace ts {
    namespace json {

        // Forward declarations.
        class Value;
        class RunningDocument;

        //!
        //! Command line arguments for JSON reports (@c -\-json, @c -\-json-line, @c -\-json-udp).
        //! @ingroup cmd
        //!
        class TSDUCKDLL OutputArgs : public ArgsSupplierInterface
        {
            TS_NOCOPY(OutputArgs);
        public:
            //!
            //! Default constructor.
            //! @param [in] use_short_opt Define @c 'j' as short option for @c -\-json.
            //! @param [in] help Help text for option @c -\-json.
            //!
            OutputArgs(bool use_short_opt = false, const UString& help = UString());

            //!
            //! Set the help text for the @c -\-json option.
            //! Must be called before defineArgs().
            //! @param [in] text Help text for the @c -\-json option.
            //!
            void setHelp(const UString& text) { _json_help = text; }

            // Implementation of ArgsSupplierInterface.
            virtual void defineArgs(Args& args) override;
            virtual bool loadArgs(DuckContext& duck, Args& args) override;

            //!
            //! Check if any JSON output option is specified.
            //! @return True if any JSON output option is specified.
            //!
            bool useJSON() const { return _json_opt || _json_line || _json_udp; }

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
            bool              _use_short_opt;
            UString           _json_help;
            bool              _json_opt;         // Option --json
            bool              _json_line;        // Option --json-line
            bool              _json_udp;         // Option --json-udp
            UString           _line_prefix;      // Option --json-line="prefix"
            IPv4SocketAddress _udp_destination;  // UDP destination.
            IPv4Address       _udp_local;        // Name of outgoing local address.
            int               _udp_ttl;          // Time-to-live socket option.
            UDPSocket         _sock;             // Output UDP socket.

            // Issue a JSON report, except --json file.
            bool reportOthers(const json::Value& root, Report& rep);
        };
    }
}
