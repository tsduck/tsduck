//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  The class ts::tlv::Protocol defines the syntax of a TLV protocol.
//  Used by ts::tlv::MessageFactory to validate a message.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTLVMessage.h"

namespace ts {
    namespace tlv {

        class MessageFactory;

        class TSDUCKDLL Protocol
        {
        public:
            // Constructors. Specify a version number for "messages" (version + compound TLV).
            Protocol () : _has_version (false), _version (0) {}
            Protocol (VERSION v) : _has_version (true), _version (v) {}

            // Get protocol version
            bool hasVersion() const {return _has_version;}
            VERSION version() const {return _version;}
            void setVersion (VERSION v) {_has_version = true; _version = v;}

            // This method declares a command tag in the protocol.
            // Required only for commands without parameters.
            void add (TAG cmd_tag) {_commands[cmd_tag];}

            // This method declares a command tag in the protocol and
            // one of its parameters. Must be invoked for each parameter
            // of each command. Min_size and max_size define the allowed
            // sizes for the parameter value. Min_count and Max_count
            // define the minimum and maximum number of occurences of
            // this parameter in the command.
            void add (TAG cmd_tag, TAG param_tag,
                      size_t min_size, size_t max_size,
                      size_t min_count, size_t max_count)
            {
                Parameter p = {0, min_size, max_size, min_count, max_count};
                _commands[cmd_tag].params[param_tag] = p;
            }

            // Same but the parameter is a compound TLV structure.
            void add (TAG cmd_tag, TAG param_tag,
                      const Protocol* compound,
                      size_t min_count, size_t max_count)
            {
                Parameter p = {compound, 0, 0, min_count, max_count};
                _commands[cmd_tag].params[param_tag] = p;
            }

            // Virtual destructor
            virtual ~Protocol() {}

            // This method is invoked by the MessageFactory after analysis
            // of the command and parameters. All actual parameters
            // have been checked for consistency with the protocol.
            // The returned Message is allocated using the new operator.
            virtual void factory (const MessageFactory&, MessagePtr&) const = 0;

            // This method creates an error response from the result of
            // the analysis of a faulty incoming message.
            // The returned Message is allocated using the new operator.
            virtual void buildErrorResponse (const MessageFactory&, MessagePtr&) const = 0;
                
        private:
            // Unreachable constructors and operators
            Protocol (const Protocol&);
            Protocol& operator= (const Protocol&);

            // The class MessageFactory acceeses the internal representation of the protocol.
            friend class MessageFactory;

            // Description of a parameter:
            struct Parameter {
                const Protocol* compound; // Compound TLV parameter (or 0)
                size_t min_size;          // Min size (if compound == 0)
                size_t max_size;          // Max size (if compound == 0)
                size_t min_count;         // Min occurence count
                size_t max_count;         // Max occurence count
            };
            typedef std::map<TAG,Parameter> ParameterMap;

            // Description of a command:
            struct Command {
                ParameterMap params;
            };
            typedef std::map<TAG,Command> CommandMap;

            // Protocol private members
            bool       _has_version;
            VERSION    _version;
            CommandMap _commands;
        };
    }
}
