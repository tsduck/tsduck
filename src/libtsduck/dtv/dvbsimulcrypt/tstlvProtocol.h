//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class which defines the syntax of a TLV protocol.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvMessage.h"

namespace ts {
    namespace tlv {

        class MessageFactory;

        //!
        //! The class ts::tlv::Protocol defines the syntax of a TLV protocol.
        //! Used by ts::tlv::MessageFactory to validate a message.
        //! @ingroup tlv
        //!
        class TSDUCKDLL Protocol
        {
            TS_NOCOPY(Protocol);
        public:
            //!
            //! Constructor for a protocol without version number.
            //!
            Protocol() = default;

            //!
            //! Constructor for a protocol with version number.
            //! The message format is version + compound TLV.
            //! @param [in] v Expected protocol version.
            //!
            Protocol(VERSION v);

            //!
            //! Check if the protocol has a protocol version number.
            //! @return True if the message has a protocol version number.
            //!
            bool hasVersion() const { return _has_version; }

            //!
            //! Get the protocol version number.
            //! @return The protocol version number.
            //!
            VERSION version() const { return _version; }

            //!
            //! Change the protocol version number.
            //! @param [in] v The new protocol version number.
            //!
            void setVersion(VERSION v)
            {
                _has_version = true;
                _version = v;
            }

            //!
            //! This method declares a command tag in the protocol.
            //! Required only for commands without parameters.
            //! @param [in] cmd_tag Message tag.
            //!
            void add(TAG cmd_tag) {_commands[cmd_tag];}

            //!
            //! This method declares a command tag in the protocol and one of its parameters.
            //! Must be invoked for each parameter of each command.
            //! @param [in] cmd_tag Message tag.
            //! @param [in] param_tag Parameter tag.
            //! @param [in] min_size Minimum allowed size for the parameter value.
            //! @param [in] max_size Maximum allowed size for the parameter value.
            //! @param [in] min_count Minimum number of occurences of this parameter in the command.
            //! @param [in] max_count Maximum number of occurences of this parameter in the command.
            //!
            void add(TAG cmd_tag, TAG param_tag, size_t min_size, size_t max_size, size_t min_count, size_t max_count);

            //!
            //! This method declares a command tag in the protocol and one of its parameters.
            //! Same as add() but with a parameter which is a compound TLV structure.
            //! @param [in] cmd_tag Message tag.
            //! @param [in] param_tag Parameter tag.
            //! @param [in] compound Protocol describing the compound TLV structure.
            //! @param [in] min_count Minimum number of occurences of this parameter in the command.
            //! @param [in] max_count Maximum number of occurences of this parameter in the command.
            //!
            void add(TAG cmd_tag, TAG param_tag, const Protocol* compound, size_t min_count, size_t max_count);

            //!
            //! Virtual destructor.
            //!
            virtual ~Protocol();

            //!
            //! Generic factory method.
            //! This pure virtual method must be implemented by subclasses.
            //! This method is invoked by the MessageFactory after analysis
            //! of the command and parameters. All actual parameters
            //! have been checked for consistency with the protocol.
            //! @param [in] mf The message factory which analyzed the binary message.
            //! @param [out] msg Safe pointer to the new message.
            //!
            virtual void factory(const MessageFactory& mf, MessagePtr& msg) const = 0;

            //!
            //! Error response creation.
            //! This method creates an error response from the result of
            //! the analysis of a faulty incoming message.
            //! @param [in] mf The message factory which analyzed the binary message.
            //! @param [out] msg Safe pointer to the new error message.
            //!
            virtual void buildErrorResponse(const MessageFactory& mf, MessagePtr& msg) const = 0;

            //!
            //! Get the protocol name (for information only).
            //! @return The protocol name.
            //!
            virtual UString name() const = 0;

        private:
            // The class MessageFactory acceeses the internal representation of the protocol.
            friend class MessageFactory;

            // Description of a parameter:
            struct Parameter {
                const Protocol* compound;  // Compound TLV parameter (or nullptr)
                size_t min_size;           // Min size (if compound == nullptr)
                size_t max_size;           // Max size (if compound == nullptr)
                size_t min_count;          // Min occurence count
                size_t max_count;          // Max occurence count
            };
            typedef std::map<TAG,Parameter> ParameterMap;

            // Description of a command:
            struct Command {
                Command() = default;
                ParameterMap params {};
            };
            typedef std::map<TAG,Command> CommandMap;

            // Protocol private members
            bool       _has_version = false;
            VERSION    _version = 0;
            CommandMap _commands {};
        };
    }
}
