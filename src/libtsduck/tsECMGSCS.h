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
//!  DVB SimulCrypt ECMG <=> SCS protocol
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLVProtocol.h"
#include "tsTLVStreamMessage.h"
#include "tsSingletonManager.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! Definitions of the DVB SimulCrypt ECMG <=> SCS protocol.
    //!
    namespace ecmgscs {

        //---------------------------------------------------------------------
        // Protocol-defined values
        //---------------------------------------------------------------------

        //! Current version of the ECMG <=> SCS protocol
        const tlv::VERSION CURRENT_VERSION = 0x03;

        //!
        //! All DVB-defined tags (commands and parameters).
        //! Tags is defined as struct instead of namespace to be used as traits/
        //!
        struct TSDUCKDLL Tags {
            //!
            //! ECMG <=> SCS command tags.
            //!
            enum Command {
                channel_setup                 = 0x0001,
                channel_test                  = 0x0002,
                channel_status                = 0x0003,
                channel_close                 = 0x0004,
                channel_error                 = 0x0005,
                stream_setup                  = 0x0101,
                stream_test                   = 0x0102,
                stream_status                 = 0x0103,
                stream_close_request          = 0x0104,
                stream_close_response         = 0x0105,
                stream_error                  = 0x0106,
                CW_provision                  = 0x0201,
                ECM_response                  = 0x0202,
            };
            //!
            //! ECMG <=> SCS parameter tags.
            //!
            enum Parameter {
                Super_CAS_id                  = 0x0001,
                section_TSpkt_flag            = 0x0002,
                delay_start                   = 0x0003,
                delay_stop                    = 0x0004,
                transition_delay_start        = 0x0005,
                transition_delay_stop         = 0x0006,
                ECM_rep_period                = 0x0007,
                max_streams                   = 0x0008,
                min_CP_duration               = 0x0009,
                lead_CW                       = 0x000A,
                CW_per_msg                    = 0x000B,
                max_comp_time                 = 0x000C,
                access_criteria               = 0x000D,
                ECM_channel_id                = 0x000E,
                ECM_stream_id                 = 0x000F,
                nominal_CP_duration           = 0x0010,
                access_criteria_transfer_mode = 0x0011,
                CP_number                     = 0x0012,
                CP_duration                   = 0x0013,
                CP_CW_combination             = 0x0014,
                ECM_datagram                  = 0x0015,
                AC_delay_start                = 0x0016,
                AC_delay_stop                 = 0x0017,
                CW_encryption                 = 0x0018,
                ECM_id                        = 0x0019,
                error_status                  = 0x7000,
                error_information             = 0x7001,
            };
        };

        //!
        //! All error status values
        //!
        struct TSDUCKDLL Errors {
            //!
            //! All error status values
            //!
            enum StatusValue {
                inv_message           = 0x0001,
                inv_proto_version     = 0x0002,
                inv_message_type      = 0x0003,
                message_too_long      = 0x0004,
                inv_Super_CAS_id      = 0x0005,
                inv_channel_id        = 0x0006,
                inv_stream_id         = 0x0007,
                too_many_channels     = 0x0008,
                too_many_stm_chan     = 0x0009,
                too_many_stm_ecmg     = 0x000A,
                not_enough_CW         = 0x000B,
                out_of_storage        = 0x000C,
                out_of_compute        = 0x000D,
                inv_param_type        = 0x000E,
                inv_param_length      = 0x000F,
                missing_param         = 0x0010,
                inv_param_value       = 0x0011,
                inv_ECM_id            = 0x0012,
                channel_id_in_use     = 0x0013,
                stream_id_in_use      = 0x0014,
                ECM_id_in_use         = 0x0015,
                unknown_error         = 0x7000,
                unrecoverable_error   = 0x7001,
            };
        };


        //---------------------------------------------------------------------
        //! Generic description of the ECMG <=> SCS protocol.
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
        // Definition of all ECMG <=> SCS protocol messages
        //---------------------------------------------------------------------

        //!
        //! ECMG <=> SCS channel_setup command
        //!
        class TSDUCKDLL ChannelSetup : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            uint32_t Super_CAS_id;
        public:
            ChannelSetup ();
            ChannelSetup (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS channel_test command
        //!
        class TSDUCKDLL ChannelTest : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
        public:
            ChannelTest ();
            ChannelTest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS channel_status command
        //!
        class TSDUCKDLL ChannelStatus : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            bool     section_TSpkt_flag;
            bool     has_AC_delay_start;
            int16_t  AC_delay_start;
            bool     has_AC_delay_stop;
            int16_t  AC_delay_stop;
            int16_t  delay_start;
            int16_t  delay_stop;
            bool     has_transition_delay_start;
            int16_t  transition_delay_start;
            bool     has_transition_delay_stop;
            int16_t  transition_delay_stop;
            uint16_t ECM_rep_period;
            uint16_t max_streams;
            uint16_t min_CP_duration;
            uint8_t  lead_CW;
            uint8_t  CW_per_msg;
            uint16_t max_comp_time;
        public:
            ChannelStatus ();
            ChannelStatus (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS channel_close command
        //!
        class TSDUCKDLL ChannelClose : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
        public:
            ChannelClose ();
            ChannelClose (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS channel_error command
        //!
        class TSDUCKDLL ChannelError : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            std::vector<uint16_t> error_status;
            std::vector<uint16_t> error_information;
        public:
            ChannelError ();
            ChannelError (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_setup command
        //!
        class TSDUCKDLL StreamSetup : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
            uint16_t ECM_id;
            uint16_t nominal_CP_duration;
        public:
            StreamSetup ();
            StreamSetup (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_test command
        //!
        class TSDUCKDLL StreamTest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id;  // (ECM_stream_id)
        public:
            StreamTest ();
            StreamTest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_status command
        //!
        class TSDUCKDLL StreamStatus : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id;  // (ECM_stream_id)
            uint16_t ECM_id;
            bool   access_criteria_transfer_mode;
        public:
            StreamStatus ();
            StreamStatus (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_close_request command
        //!
        class TSDUCKDLL StreamCloseRequest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
        public:
            StreamCloseRequest ();
            StreamCloseRequest (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_close_response command
        //!
        class TSDUCKDLL StreamCloseResponse : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
        public:
            StreamCloseResponse ();
            StreamCloseResponse (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS stream_error command
        //!
        class TSDUCKDLL StreamError : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
            std::vector<uint16_t> error_status;
            std::vector<uint16_t> error_information;
        public:
            StreamError ();
            StreamError (const tlv::MessageFactory& fact);
            virtual std::string dump (size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! A combination of CP number and CW for ECMG <=> SCS CW_provision command
        //!
        struct CPCWCombination
        {
            uint16_t  CP;  //! Crypto-period number.
            ByteBlock CW;  //! Control word.

            //!
            //! Constructor.
            //! @param [in] cpn Crypto-period number.
            //! @param [in] cwb Control word.
            //!
            CPCWCombination(uint16_t cpn, const ByteBlock& cwb) :
                CP(cpn),
                CW(cwb)
            {
            }

            //!
            //! Constructor.
            //! @param [in] cpn Crypto-period number.
            //! @param [in] cw_addr Control word address.
            //! @param [in] cw_size Control word size in bytes.
            //!
            CPCWCombination(uint16_t cpn = 0, const void* cw_addr = 0, size_t cw_size = CW_BYTES) :
                CP(cpn),
                CW(cw_addr, cw_addr != 0 ? cw_size : 0)
            {
            }
        };

        //!
        //! ECMG <=> SCS CW_provision command
        //!
        class TSDUCKDLL CWProvision : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
            uint16_t  CP_number;
            bool      has_CW_encryption;
            ByteBlock CW_encryption;
            std::vector<CPCWCombination> CP_CW_combination;
            bool      has_CP_duration;
            uint16_t  CP_duration;
            bool      has_access_criteria;
            ByteBlock access_criteria;
        public:
            CWProvision();
            CWProvision(const tlv::MessageFactory& fact);
            virtual std::string dump(size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };

        //!
        //! ECMG <=> SCS ECM_response command
        //!
        class TSDUCKDLL ECMResponse : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // (ECM_channel_id)
            // uint16_t stream_id; // (ECM_stream_id)
            uint16_t  CP_number;
            ByteBlock ECM_datagram;
        public:
            ECMResponse();
            ECMResponse(const tlv::MessageFactory& fact);
            virtual std::string dump(size_t indent = 0) const;
        protected:
            virtual void serializeParameters (tlv::Serializer& fact) const;
        };


        //---------------------------------------------------------------------
        //! Generic "traits" for the ECMG <=> SCS protocol.
        //---------------------------------------------------------------------

        struct TSDUCKDLL Traits {
            typedef ecmgscs::Tags     Tags;
            typedef ecmgscs::Errors   Errors;
            typedef ecmgscs::Protocol Protocol;

            typedef ecmgscs::ChannelSetup  ChannelSetup;
            typedef ecmgscs::ChannelTest   ChannelTest;
            typedef ecmgscs::ChannelStatus ChannelStatus;
            typedef ecmgscs::ChannelClose  ChannelClose;
            typedef ecmgscs::ChannelError  ChannelError;

            typedef ecmgscs::StreamSetup         StreamSetup;
            typedef ecmgscs::StreamTest          StreamTest;
            typedef ecmgscs::StreamStatus        StreamStatus;
            typedef ecmgscs::StreamCloseRequest  StreamCloseRequest;
            typedef ecmgscs::StreamCloseResponse StreamCloseResponse;
            typedef ecmgscs::StreamError         StreamError;
        };
    }
}
