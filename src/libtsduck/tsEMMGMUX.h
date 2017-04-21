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
//!
//!  @file
//!  DVB SimulCrypt EMMG/PDG <=> MUX protocol
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLVProtocol.h"
#include "tsTLVStreamMessage.h"
#include "tsSingletonManager.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! Definitions of the DVB SimulCrypt EMMG <=> MUX protocol.
    //!
    namespace emmgmux {

        //---------------------------------------------------------------------
        // Protocol-defined values
        //---------------------------------------------------------------------

        // Current version of the EMMG/PDG <=> MUX protocol
        const tlv::VERSION CURRENT_VERSION = 0x03;

        // All DVB-defined tags (commands and parameters).
        // Tags is defined as struct instead of namespace to be used as traits
        struct TSDUCKDLL Tags {
            enum Command {
                channel_setup         = 0x0011,
                channel_test          = 0x0012,
                channel_status        = 0x0013,
                channel_close         = 0x0014,
                channel_error         = 0x0015,
                stream_setup          = 0x0111,
                stream_test           = 0x0112,
                stream_status         = 0x0113,
                stream_close_request  = 0x0114,
                stream_close_response = 0x0115,
                stream_error          = 0x0116,
                stream_BW_request     = 0x0117,
                stream_BW_allocation  = 0x0118,
                data_provision        = 0x0211,
            };
            enum Parameter {
                client_id             = 0x0001,
                section_TSpkt_flag    = 0x0002,
                data_channel_id       = 0x0003,
                data_stream_id        = 0x0004,
                datagram              = 0x0005,
                bandwidth             = 0x0006,
                data_type             = 0x0007,
                data_id               = 0x0008,
                error_status          = 0x7000,
                error_information     = 0x7001,
            };
        };

        // All error status values
        struct TSDUCKDLL Errors {
            enum StatusValue {
                inv_message           = 0x0001,
                inv_proto_version     = 0x0002,
                inv_message_type      = 0x0003,
                message_too_long      = 0x0004,
                inv_data_stream_id    = 0x0005,
                inv_data_channel_id   = 0x0006,
                too_many_channels     = 0x0007,
                too_many_stm_chan     = 0x0008,
                too_many_stm_mux      = 0x0009,
                inv_param_type        = 0x000A,
                inv_param_length      = 0x000B,
                missing_param         = 0x000C,
                inv_param_value       = 0x000D,
                inv_client_id         = 0x000E,
                exceeded_bw           = 0x000F,
                inv_data_id           = 0x0010,
                channel_id_in_use     = 0x0011,
                stream_id_in_use      = 0x0012,
                data_id_in_use        = 0x0013,
                client_id_in_use      = 0x0014,
                unknown_error         = 0x7000,
                unrecoverable_error   = 0x7001,
            };
        };

        // EMMG <=> MUX data types
        struct TSDUCKDLL DataTypes {
            enum {
                EMM     = 0x00,
                PRIVATE = 0x01,
                ECM     = 0x02, // DVB-reserved
            };
        };


        //---------------------------------------------------------------------
        // Generic description of the EMMG/PDG <=> MUX protocol.
        //---------------------------------------------------------------------

        class TSDUCKDLL Protocol : public tlv::Protocol
        {
            // This class is a singleton. Use static Instance() method.
            tsDeclareSingleton (Protocol);

        public:
            // Implementation of pure virtual methods
            virtual void factory (const tlv::MessageFactory&, tlv::MessagePtr&) const;
            virtual void buildErrorResponse (const tlv::MessageFactory&, tlv::MessagePtr&) const;
        };


        //---------------------------------------------------------------------
        // Definition of all EMMG/PDG <=> MUX protocol messages
        //---------------------------------------------------------------------

        // channel_setup
        //--------------

        class TSDUCKDLL ChannelSetup : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            uint32_t client_id;
            bool   section_TSpkt_flag;
        public:
            ChannelSetup ();
            ChannelSetup (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // channel_test
        //-------------

        class TSDUCKDLL ChannelTest : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            uint32_t client_id;
        public:
            ChannelTest ();
            ChannelTest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // channel_status
        //---------------

        class TSDUCKDLL ChannelStatus : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            uint32_t client_id;
            bool   section_TSpkt_flag;
        public:
            ChannelStatus ();
            ChannelStatus (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // channel_close
        //--------------

        class TSDUCKDLL ChannelClose : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            uint32_t client_id;
        public:
            ChannelClose ();
            ChannelClose (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // channel_error
        //--------------

        class TSDUCKDLL ChannelError : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            uint32_t client_id;
            std::vector<uint16_t> error_status;
            std::vector<uint16_t> error_information;
        public:
            ChannelError ();
            ChannelError (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_setup
        //-------------

        class TSDUCKDLL StreamSetup : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
            uint16_t data_id;
            uint8_t  data_type;
        public:
            StreamSetup ();
            StreamSetup (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_test
        //------------

        class TSDUCKDLL StreamTest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
        public:
            StreamTest ();
            StreamTest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_status
        //--------------

        class TSDUCKDLL StreamStatus : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
            uint16_t data_id;
            uint8_t  data_type;
        public:
            StreamStatus ();
            StreamStatus (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_close_request
        //---------------------

        class TSDUCKDLL StreamCloseRequest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
        public:
            StreamCloseRequest ();
            StreamCloseRequest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_close_response
        //----------------------

        class TSDUCKDLL StreamCloseResponse : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
        public:
            StreamCloseResponse ();
            StreamCloseResponse (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_error
        //-------------

        class TSDUCKDLL StreamError : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
            std::vector<uint16_t> error_status;
            std::vector<uint16_t> error_information;
        public:
            StreamError ();
            StreamError (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_BW_request
        // -----------------

        class TSDUCKDLL StreamBWRequest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
            bool   has_bandwidth;
            uint16_t bandwidth;     // unit: kbits / second
        public:
            StreamBWRequest ();
            StreamBWRequest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // stream_BW_allocation
        // --------------------

        class TSDUCKDLL StreamBWAllocation : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id)
            // uint16_t stream_id;  // (data_stream_id)
            uint32_t client_id;
            bool   has_bandwidth;
            uint16_t bandwidth;     // unit: kbits / second
        public:
            StreamBWAllocation ();
            StreamBWAllocation (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        // data_provision
        // --------------

        class TSDUCKDLL DataProvision : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (data_channel_id, forbidden on UDP, required on TCP)
            // uint16_t stream_id;  // (data_stream_id,  forbidden on UDP, required on TCP)
            uint32_t client_id;
            uint16_t data_id;
            std::vector <ByteBlockPtr> datagram;
        public:
            DataProvision ();
            DataProvision (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };


        //---------------------------------------------------------------------
        // Generic "traits" for the EMMG/PDG <=> MUX protocol.
        //---------------------------------------------------------------------

        struct TSDUCKDLL Traits {
            typedef emmgmux::Tags     Tags;
            typedef emmgmux::Errors   Errors;
            typedef emmgmux::Protocol Protocol;

            typedef emmgmux::ChannelSetup  ChannelSetup;
            typedef emmgmux::ChannelTest   ChannelTest;
            typedef emmgmux::ChannelStatus ChannelStatus;
            typedef emmgmux::ChannelClose  ChannelClose;
            typedef emmgmux::ChannelError  ChannelError;

            typedef emmgmux::StreamSetup         StreamSetup;
            typedef emmgmux::StreamTest          StreamTest;
            typedef emmgmux::StreamStatus        StreamStatus;
            typedef emmgmux::StreamCloseRequest  StreamCloseRequest;
            typedef emmgmux::StreamCloseResponse StreamCloseResponse;
            typedef emmgmux::StreamError         StreamError;
        };
    }
}
