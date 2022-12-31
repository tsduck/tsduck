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
//!  @ingroup mpeg
//!  DVB SimulCrypt ECMG <=> SCS protocol
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvProtocol.h"
#include "tstlvStreamMessage.h"
#include "tsSingletonManager.h"
#include "tsDVBCSA2.h"

namespace ts {
    //!
    //! Definitions of the DVB SimulCrypt ECMG <=> SCS protocol.
    //!
    namespace ecmgscs {

        //---------------------------------------------------------------------
        // Protocol-defined values
        //---------------------------------------------------------------------

        //! Current version of the ECMG <=> SCS protocol.
        const tlv::VERSION CURRENT_VERSION = 0x03;

        //!
        //! Check if a command tag is valid for the ECMG <=> SCS protocol.
        //! @param [in] tag Command tag.
        //! @return True if @a tag is a valid command for the ECMG <=> SCS protocol.
        //!
        TSDUCKDLL inline bool IsValidCommand(uint16_t tag)
        {
            return (tag >= 0x0001 && tag <= 0x0005) || (tag >= 0x0101 && tag <= 0x0106) || (tag >= 0x0201 && tag <= 0x0202);
        }

        //!
        //! All DVB-defined tags (commands and parameters).
        //! Tags is defined as struct instead of namespace to be used as traits/
        //!
        struct TSDUCKDLL Tags {
            //!
            //! ECMG <=> SCS command tags.
            //!
            enum Command : uint16_t {
                channel_setup                 = 0x0001,  //!< The channel_setup message tag.
                channel_test                  = 0x0002,  //!< The channel_test message tag.
                channel_status                = 0x0003,  //!< The channel_status message tag.
                channel_close                 = 0x0004,  //!< The channel_close message tag.
                channel_error                 = 0x0005,  //!< The channel_error message tag.
                stream_setup                  = 0x0101,  //!< The stream_setup message tag.
                stream_test                   = 0x0102,  //!< The stream_test message tag.
                stream_status                 = 0x0103,  //!< The stream_status message tag.
                stream_close_request          = 0x0104,  //!< The stream_close_request message tag.
                stream_close_response         = 0x0105,  //!< The stream_close_response message tag.
                stream_error                  = 0x0106,  //!< The stream_error message tag.
                CW_provision                  = 0x0201,  //!< The CW_provision message tag.
                ECM_response                  = 0x0202,  //!< The ECM_response message tag.
            };
            //!
            //! ECMG <=> SCS parameter tags.
            //!
            enum Parameter : uint16_t {
                Super_CAS_id                  = 0x0001,  //!< The Super_CAS_id parameter tag.
                section_TSpkt_flag            = 0x0002,  //!< The section_TSpkt_flag parameter tag.
                delay_start                   = 0x0003,  //!< The delay_start parameter tag.
                delay_stop                    = 0x0004,  //!< The delay_stop parameter tag.
                transition_delay_start        = 0x0005,  //!< The transition_delay_start parameter tag.
                transition_delay_stop         = 0x0006,  //!< The transition_delay_stop parameter tag.
                ECM_rep_period                = 0x0007,  //!< The ECM_rep_period parameter tag.
                max_streams                   = 0x0008,  //!< The max_streams parameter tag.
                min_CP_duration               = 0x0009,  //!< The min_CP_duration parameter tag.
                lead_CW                       = 0x000A,  //!< The lead_CW parameter tag.
                CW_per_msg                    = 0x000B,  //!< The CW_per_msg parameter tag.
                max_comp_time                 = 0x000C,  //!< The max_comp_time parameter tag.
                access_criteria               = 0x000D,  //!< The access_criteria parameter tag.
                ECM_channel_id                = 0x000E,  //!< The ECM_channel_id parameter tag.
                ECM_stream_id                 = 0x000F,  //!< The ECM_stream_id parameter tag.
                nominal_CP_duration           = 0x0010,  //!< The nominal_CP_duration parameter tag.
                access_criteria_transfer_mode = 0x0011,  //!< The access_criteria_transfer_mode parameter tag.
                CP_number                     = 0x0012,  //!< The CP_number parameter tag.
                CP_duration                   = 0x0013,  //!< The CP_duration parameter tag.
                CP_CW_combination             = 0x0014,  //!< The CP_CW_combination parameter tag.
                ECM_datagram                  = 0x0015,  //!< The ECM_datagram parameter tag.
                AC_delay_start                = 0x0016,  //!< The AC_delay_start parameter tag.
                AC_delay_stop                 = 0x0017,  //!< The AC_delay_stop parameter tag.
                CW_encryption                 = 0x0018,  //!< The CW_encryption parameter tag.
                ECM_id                        = 0x0019,  //!< The ECM_id parameter tag.
                error_status                  = 0x7000,  //!< The error_status parameter tag.
                error_information             = 0x7001,  //!< The error_information parameter tag.
            };
        };

        //!
        //! All error status values
        //!
        struct TSDUCKDLL Errors {
            //!
            //! All error status values
            //!
            enum StatusValue : uint16_t {
                inv_message           = 0x0001,  //!< The inv_message error status value.
                inv_proto_version     = 0x0002,  //!< The inv_proto_version error status value.
                inv_message_type      = 0x0003,  //!< The inv_message_type error status value.
                message_too_long      = 0x0004,  //!< The message_too_long error status value.
                inv_Super_CAS_id      = 0x0005,  //!< The inv_Super_CAS_id error status value.
                inv_channel_id        = 0x0006,  //!< The inv_channel_id error status value.
                inv_stream_id         = 0x0007,  //!< The inv_stream_id error status value.
                too_many_channels     = 0x0008,  //!< The too_many_channels error status value.
                too_many_stm_chan     = 0x0009,  //!< The too_many_stm_chan error status value.
                too_many_stm_ecmg     = 0x000A,  //!< The too_many_stm_ecmg error status value.
                not_enough_CW         = 0x000B,  //!< The not_enough_CW error status value.
                out_of_storage        = 0x000C,  //!< The out_of_storage error status value.
                out_of_compute        = 0x000D,  //!< The out_of_compute error status value.
                inv_param_type        = 0x000E,  //!< The inv_param_type error status value.
                inv_param_length      = 0x000F,  //!< The inv_param_length error status value.
                missing_param         = 0x0010,  //!< The missing_param error status value.
                inv_param_value       = 0x0011,  //!< The inv_param_value error status value.
                inv_ECM_id            = 0x0012,  //!< The inv_ECM_id error status value.
                channel_id_in_use     = 0x0013,  //!< The channel_id_in_use error status value.
                stream_id_in_use      = 0x0014,  //!< The stream_id_in_use error status value.
                ECM_id_in_use         = 0x0015,  //!< The ECM_id_in_use error status value.
                unknown_error         = 0x7000,  //!< The unknown_error error status value.
                unrecoverable_error   = 0x7001,  //!< The unrecoverable_error error status value.
            };
            //!
            //! Return a message for a given protocol error status.
            //! @param [in] status Status code.
            //! @return Error message.
            //!
            static UString Name(uint16_t status);
        };


        //---------------------------------------------------------------------
        //! Generic description of the ECMG <=> SCS protocol.
        //---------------------------------------------------------------------

        class TSDUCKDLL Protocol : public tlv::Protocol
        {
            // This class is a singleton. Use static Instance() method.
            TS_DECLARE_SINGLETON(Protocol);

        public:
            // Implementation of pure virtual methods
            virtual void factory(const tlv::MessageFactory&, tlv::MessagePtr&) const override;
            virtual void buildErrorResponse(const tlv::MessageFactory&, tlv::MessagePtr&) const override;
            virtual UString name() const override;
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
            // uint16_t channel_id; // ECM_channel_id
            uint32_t Super_CAS_id;  //!< Super CAS id.

            //!
            //! Default constructor.
            //!
            ChannelSetup();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ChannelSetup(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS channel_test command
        //!
        class TSDUCKDLL ChannelTest : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // ECM_channel_id

            //!
            //! Default constructor.
            //!
            ChannelTest();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ChannelTest(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS channel_status command
        //!
        class TSDUCKDLL ChannelStatus : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;              // ECM_channel_id
            bool     section_TSpkt_flag;         //!< Field section_TSpkt_flag.
            bool     has_AC_delay_start;         //!< Field AC_delay_start is valid.
            int16_t  AC_delay_start;             //!< Field AC_delay_start.
            bool     has_AC_delay_stop;          //!< Field AC_delay_stop is valid.
            int16_t  AC_delay_stop;              //!< Field AC_delay_stop.
            int16_t  delay_start;                //!< Field delay_start.
            int16_t  delay_stop;                 //!< Field delay_stop.
            bool     has_transition_delay_start; //!< Field transition_delay_start is valid.
            int16_t  transition_delay_start;     //!< Field transition_delay_start.
            bool     has_transition_delay_stop;  //!< Field transition_delay_stop is valid.
            int16_t  transition_delay_stop;      //!< Field transition_delay_stop.
            uint16_t ECM_rep_period;             //!< Field ECM_rep_period.
            uint16_t max_streams;                //!< Field max_streams.
            uint16_t min_CP_duration;            //!< Field min_CP_duration.
            uint8_t  lead_CW;                    //!< Field lead_CW.
            uint8_t  CW_per_msg;                 //!< Field CW_per_msg.
            uint16_t max_comp_time;              //!< Field max_comp_time.

            //!
            //! Default constructor.
            //!
            ChannelStatus();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ChannelStatus(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS channel_close command
        //!
        class TSDUCKDLL ChannelClose : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // ECM_channel_id

            //!
            //! Default constructor.
            //!
            ChannelClose();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ChannelClose(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS channel_error command
        //!
        class TSDUCKDLL ChannelError : public tlv::ChannelMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                  // ECM_channel_id
            std::vector<uint16_t> error_status;      //!< Error code.
            std::vector<uint16_t> error_information; //!< Error information.

            //!
            //! Default constructor.
            //!
            ChannelError();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ChannelError(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_setup command
        //!
        class TSDUCKDLL StreamSetup : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;        // ECM_channel_id
            // uint16_t stream_id;         // ECM_stream_id
            uint16_t ECM_id;               //!< ECM id.
            uint16_t nominal_CP_duration;  //!< Nominal CP duration.

            //!
            //! Default constructor.
            //!
            StreamSetup();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamSetup(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_test command
        //!
        class TSDUCKDLL StreamTest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // ECM_channel_id
            // uint16_t stream_id;  // ECM_stream_id

            //!
            //! Default constructor.
            //!
            StreamTest();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamTest(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_status command
        //!
        class TSDUCKDLL StreamStatus : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                  // ECM_channel_id
            // uint16_t stream_id;                   // ECM_stream_id
            uint16_t ECM_id;                         //!< ECM id.
            bool     access_criteria_transfer_mode;  //!< Access Criteria transfer mode.

            //!
            //! Default constructor.
            //!
            StreamStatus();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamStatus(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_close_request command
        //!
        class TSDUCKDLL StreamCloseRequest : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // ECM_channel_id
            // uint16_t stream_id;  // ECM_stream_id

            //!
            //! Default constructor.
            //!
            StreamCloseRequest();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamCloseRequest(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_close_response command
        //!
        class TSDUCKDLL StreamCloseResponse : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // ECM_channel_id
            // uint16_t stream_id;  // ECM_stream_id

            //!
            //! Default constructor.
            //!
            StreamCloseResponse();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamCloseResponse(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS stream_error command
        //!
        class TSDUCKDLL StreamError : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                  // ECM_channel_id
            // uint16_t stream_id;                   // ECM_stream_id
            std::vector<uint16_t> error_status;      //!< Error code.
            std::vector<uint16_t> error_information; //!< Error information.

            //!
            //! Default constructor.
            //!
            StreamError();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            StreamError(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! A combination of CP number and CW for ECMG <=> SCS CW_provision command
        //!
        struct CPCWCombination
        {
            uint16_t  CP;  //!< Crypto-period number.
            ByteBlock CW;  //!< Control word.

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
            CPCWCombination(uint16_t cpn = 0, const void* cw_addr = nullptr, size_t cw_size = DVBCSA2::KEY_SIZE) :
                CP(cpn),
                CW(cw_addr, cw_addr != nullptr ? cw_size : 0)
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
            // uint16_t channel_id;                         // ECM_channel_id
            // uint16_t stream_id;                          // ECM_stream_id
            uint16_t  CP_number;                            //!< CP number.
            bool      has_CW_encryption;                    //!< Field CW_encryption is valid.
            ByteBlock CW_encryption;                        //!< Field CW_encryption.
            std::vector<CPCWCombination> CP_CW_combination; //!< CP/CW combinations.
            bool      has_CP_duration;                      //!< Field CP_duration is valid.
            uint16_t  CP_duration;                          //!< CP duration.
            bool      has_access_criteria;                  //!< Field access_criteria is valid.
            ByteBlock access_criteria;                      //!< Access Criteria.

            //!
            //! Default constructor.
            //!
            CWProvision();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            CWProvision(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };

        //!
        //! ECMG <=> SCS ECM_response command
        //!
        class TSDUCKDLL ECMResponse : public tlv::StreamMessage
        {
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;   // ECM_channel_id
            // uint16_t stream_id;    // ECM_stream_id
            uint16_t  CP_number;      //!< CP number.
            ByteBlock ECM_datagram;   //!< ECM packets or section.

            //!
            //! Default constructor.
            //!
            ECMResponse();
            //!
            //! Constructor from a message factory.
            //! @param [in] fact Message factory.
            //!
            ECMResponse(const tlv::MessageFactory& fact);

            // Implementation of Message.
            virtual UString dump(size_t indent = 0) const override;

        protected:
            // Implementation of Message.
            virtual void serializeParameters(tlv::Serializer& fact) const override;
        };


        //---------------------------------------------------------------------
        //! Generic "traits" for the ECMG <=> SCS protocol.
        //---------------------------------------------------------------------

        struct TSDUCKDLL Traits {
            typedef ecmgscs::Tags     Tags;       //!< Actual set of tags for this protocol.
            typedef ecmgscs::Errors   Errors;     //!< Actual set of error codes for this protocol.
            typedef ecmgscs::Protocol Protocol;   //!< Actual protocol description.

            typedef ecmgscs::ChannelSetup  ChannelSetup;   //!< Actual ChannelSetup message for this protocol.
            typedef ecmgscs::ChannelTest   ChannelTest;    //!< Actual ChannelTest message for this protocol.
            typedef ecmgscs::ChannelStatus ChannelStatus;  //!< Actual ChannelStatus message for this protocol.
            typedef ecmgscs::ChannelClose  ChannelClose;   //!< Actual ChannelClose message for this protocol.
            typedef ecmgscs::ChannelError  ChannelError;   //!< Actual ChannelError message for this protocol.

            typedef ecmgscs::StreamSetup         StreamSetup;         //!< Actual StreamSetup message for this protocol.
            typedef ecmgscs::StreamTest          StreamTest;          //!< Actual StreamTest message for this protocol.
            typedef ecmgscs::StreamStatus        StreamStatus;        //!< Actual StreamStatus message for this protocol.
            typedef ecmgscs::StreamCloseRequest  StreamCloseRequest;  //!< Actual StreamCloseRequest message for this protocol.
            typedef ecmgscs::StreamCloseResponse StreamCloseResponse; //!< Actual StreamCloseResponse message for this protocol.
            typedef ecmgscs::StreamError         StreamError;         //!< Actual StreamError message for this protocol.
        };
    }
}
