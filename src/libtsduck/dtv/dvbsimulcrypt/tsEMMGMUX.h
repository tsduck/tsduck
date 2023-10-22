//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup mpeg
//!  DVB SimulCrypt EMMG/PDG <=> MUX protocol
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvProtocol.h"
#include "tstlvStreamMessage.h"

namespace ts {
    //!
    //! Definitions of the DVB SimulCrypt EMMG <=> MUX protocol.
    //!
    namespace emmgmux {

        //---------------------------------------------------------------------
        // Protocol-defined values
        //---------------------------------------------------------------------

        //! Current version of the EMMG/PDG <=> MUX protocol
        const tlv::VERSION CURRENT_VERSION = 0x03;

        //!
        //! Check if a command tag is valid for the EMMG/PDG <=> MUX protocol.
        //! @param [in] tag Command tag.
        //! @return True if @a tag is a valid command for the EMMG/PDG <=> MUX protocol.
        //!
        TSDUCKDLL inline bool IsValidCommand(uint16_t tag)
        {
            return (tag >= 0x0011 && tag <= 0x0015) || (tag >= 0x0111 && tag <= 0x0118) || tag == 0x0211;
        }

        //!
        //! All DVB-defined tags (commands and parameters).
        //! Tags is defined as struct instead of namespace to be used as traits/
        //!
        struct TSDUCKDLL Tags {
            //!
            //! EMMG/PDG <=> MUX command tags.
            //!
            enum Command : uint16_t {
                channel_setup         = 0x0011,  //!< The channel_setup message tag.
                channel_test          = 0x0012,  //!< The channel_test message tag.
                channel_status        = 0x0013,  //!< The channel_status message tag.
                channel_close         = 0x0014,  //!< The channel_close message tag.
                channel_error         = 0x0015,  //!< The channel_error message tag.
                stream_setup          = 0x0111,  //!< The stream_setup message tag.
                stream_test           = 0x0112,  //!< The stream_test message tag.
                stream_status         = 0x0113,  //!< The stream_status message tag.
                stream_close_request  = 0x0114,  //!< The stream_close_request message tag.
                stream_close_response = 0x0115,  //!< The stream_close_response message tag.
                stream_error          = 0x0116,  //!< The stream_error message tag.
                stream_BW_request     = 0x0117,  //!< The stream_BW_request message tag.
                stream_BW_allocation  = 0x0118,  //!< The stream_BW_allocation message tag.
                data_provision        = 0x0211,  //!< The data_provision message tag.
            };
            //!
            //! EMMG/PDG <=> MUX parameter tags.
            //!
            enum Parameter : uint16_t {
                client_id             = 0x0001,  //!< The client_id parameter tag.
                section_TSpkt_flag    = 0x0002,  //!< The section_TSpkt_flag parameter tag.
                data_channel_id       = 0x0003,  //!< The data_channel_id parameter tag.
                data_stream_id        = 0x0004,  //!< The data_stream_id parameter tag.
                datagram              = 0x0005,  //!< The datagram parameter tag.
                bandwidth             = 0x0006,  //!< The bandwidth parameter tag.
                data_type             = 0x0007,  //!< The data_type parameter tag.
                data_id               = 0x0008,  //!< The data_id parameter tag.
                error_status          = 0x7000,  //!< The error_status parameter tag.
                error_information     = 0x7001,  //!< The error_information parameter tag.
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
                inv_data_stream_id    = 0x0005,  //!< The inv_data_stream_id error status value.
                inv_data_channel_id   = 0x0006,  //!< The inv_data_channel_id error status value.
                too_many_channels     = 0x0007,  //!< The too_many_channels error status value.
                too_many_stm_chan     = 0x0008,  //!< The too_many_stm_chan error status value.
                too_many_stm_mux      = 0x0009,  //!< The too_many_stm_mux error status value.
                inv_param_type        = 0x000A,  //!< The inv_param_type error status value.
                inv_param_length      = 0x000B,  //!< The inv_param_length error status value.
                missing_param         = 0x000C,  //!< The missing_param error status value.
                inv_param_value       = 0x000D,  //!< The inv_param_value error status value.
                inv_client_id         = 0x000E,  //!< The inv_client_id error status value.
                exceeded_bw           = 0x000F,  //!< The exceeded_bw error status value.
                inv_data_id           = 0x0010,  //!< The inv_data_id error status value.
                channel_id_in_use     = 0x0011,  //!< The channel_id_in_use error status value.
                stream_id_in_use      = 0x0012,  //!< The stream_id_in_use error status value.
                data_id_in_use        = 0x0013,  //!< The data_id_in_use error status value.
                client_id_in_use      = 0x0014,  //!< The client_id_in_use error status value.
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

        //!
        //! EMMG <=> MUX data types.
        //!
        struct TSDUCKDLL DataTypes {
            //!
            //! EMMG <=> MUX data types.
            //!
            enum : uint8_t {
                EMM     = 0x00,  //! Injected data are EMM.
                PRIVATE = 0x01,  //! Injected data are private.
                ECM     = 0x02,  //! Injected data are ECM, DVB-reserved.
            };
        };


        //---------------------------------------------------------------------
        //! Generic description of the EMMG/PDG <=> MUX protocol.
        //---------------------------------------------------------------------

        class TSDUCKDLL Protocol : public tlv::Protocol
        {
            TS_NOCOPY(Protocol);
        public:
            //!
            //! Default constructor.
            //!
            Protocol();

            // Implementation of pure virtual methods
            virtual void factory(const tlv::MessageFactory&, tlv::MessagePtr&) const override;
            virtual void buildErrorResponse(const tlv::MessageFactory&, tlv::MessagePtr&) const override;
            virtual UString name() const override;
        };


        //---------------------------------------------------------------------
        // Definition of all EMMG/PDG <=> MUX protocol messages
        //---------------------------------------------------------------------

        //!
        //! EMMG/PDG <=> MUX channel_setup command
        //!
        class TSDUCKDLL ChannelSetup : public tlv::ChannelMessage
        {
            TS_VERSIONED_TLV_MESSAGE(ChannelSetup, Tags::channel_setup);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;              // data_channel_id
            uint32_t client_id = 0;              //!< Client id.
            bool     section_TSpkt_flag = false; //!< Use TS packets or sections.
        };

        //!
        //! EMMG/PDG <=> MUX channel_test command
        //!
        class TSDUCKDLL ChannelTest : public tlv::ChannelMessage
        {
            TS_VERSIONED_TLV_MESSAGE(ChannelTest, Tags::channel_test);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            uint32_t client_id = 0;  //!< Client id.
        };

        //!
        //! EMMG/PDG <=> MUX channel_status command
        //!
        class TSDUCKDLL ChannelStatus : public tlv::ChannelMessage
        {
            TS_VERSIONED_TLV_MESSAGE(ChannelStatus, Tags::channel_status);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;              // data_channel_id
            uint32_t client_id = 0;              //!< Client id.
            bool     section_TSpkt_flag = false; //!< Use TS packets or sections.
        };

        //!
        //! EMMG/PDG <=> MUX channel_close command
        //!
        class TSDUCKDLL ChannelClose : public tlv::ChannelMessage
        {
            TS_VERSIONED_TLV_MESSAGE(ChannelClose, Tags::channel_close);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id; // data_channel_id
            uint32_t client_id = 0; //!< Client id.
        };

        //!
        //! EMMG/PDG <=> MUX channel_error command
        //!
        class TSDUCKDLL ChannelError : public tlv::ChannelMessage
        {
            TS_VERSIONED_TLV_MESSAGE(ChannelError, Tags::channel_error);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                     // data_channel_id
            uint32_t client_id = 0;                     //!< Client id.
            std::vector<uint16_t> error_status {};      //!< Error code.
            std::vector<uint16_t> error_information {}; //!< Error information.
        };

        //!
        //! EMMG/PDG <=> MUX stream_setup command
        //!
        class TSDUCKDLL StreamSetup : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamSetup, Tags::stream_setup);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            // uint16_t stream_id;   // data_stream_id
            uint32_t client_id = 0;  //!< Client id.
            uint16_t data_id = 0;    //!< Data id.
            uint8_t  data_type = 0;  //!< Data type.
        };

        //!
        //! EMMG/PDG <=> MUX stream_test command
        //!
        class TSDUCKDLL StreamTest : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamTest, Tags::stream_test);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            // uint16_t stream_id;   // data_stream_id
            uint32_t client_id = 0;  //!< Client id.
        };

        //!
        //! EMMG/PDG <=> MUX stream_status command
        //!
        class TSDUCKDLL StreamStatus : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamStatus, Tags::stream_status);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            // uint16_t stream_id;   // data_stream_id
            uint32_t client_id = 0;  //!< Client id.
            uint16_t data_id = 0;    //!< Data id.
            uint8_t  data_type = 0;  //!< Data type.
        };

        //!
        //! EMMG/PDG <=> MUX stream_close_request command
        //!
        class TSDUCKDLL StreamCloseRequest : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamCloseRequest, Tags::stream_close_request);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            // uint16_t stream_id;   // data_stream_id
            uint32_t client_id = 0;  //!< Client id.
        };

        //!
        //! EMMG/PDG <=> MUX stream_close_response command
        //!
        class TSDUCKDLL StreamCloseResponse : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamCloseResponse, Tags::stream_close_response);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;  // data_channel_id
            // uint16_t stream_id;   // data_stream_id
            uint32_t client_id = 0;  //!< Client id.
        };

        //!
        //! EMMG/PDG <=> MUX stream_error command
        //!
        class TSDUCKDLL StreamError : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamError, Tags::stream_error);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                     // data_channel_id
            // uint16_t stream_id;                      // data_stream_id
            uint32_t client_id = 0;                     //!< Client id.
            std::vector<uint16_t> error_status {};      //!< Error code.
            std::vector<uint16_t> error_information {}; //!< Error information.
        };

        //!
        //! EMMG/PDG <=> MUX stream_BW_request command
        //!
        class TSDUCKDLL StreamBWRequest : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamBWRequest, Tags::stream_BW_request);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;         // data_channel_id
            // uint16_t stream_id;          // data_stream_id
            uint32_t client_id = 0;         //!< Client id.
            bool     has_bandwidth = false; //!< Field bandwidth is valid.
            uint16_t bandwidth = 0;         //!< Bandwidth in kbits / second.
        };

        //!
        //! EMMG/PDG <=> MUX stream_BW_allocation command
        //!
        class TSDUCKDLL StreamBWAllocation : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(StreamBWAllocation, Tags::stream_BW_allocation);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;         // data_channel_id
            // uint16_t stream_id;          // data_stream_id
            uint32_t client_id = 0;         //!< Client id.
            bool     has_bandwidth = false; //!< Field bandwidth is valid.
            uint16_t bandwidth = 0;         //!< Bandwidth in kbits / second.
        };

        //!
        //! EMMG/PDG <=> MUX data_provision command
        //!
        class TSDUCKDLL DataProvision : public tlv::StreamMessage
        {
            TS_VERSIONED_TLV_MESSAGE(DataProvision, Tags::data_provision);
        public:
            // Protocol-documented fields:
            // uint16_t channel_id;                  // data_channel_id, forbidden on UDP, required on TCP
            // uint16_t stream_id;                   // data_stream_id,  forbidden on UDP, required on TCP
            uint32_t client_id = 0;                  //!< Client id.
            uint16_t data_id = 0;                    //!< Data id (DataTypes).
            std::vector <ByteBlockPtr> datagram {};  //!< EMM or private data to send.
        };


        //---------------------------------------------------------------------
        //! Generic "traits" for the EMMG/PDG <=> MUX protocol.
        //---------------------------------------------------------------------

        struct TSDUCKDLL Traits {
            typedef emmgmux::Tags     Tags;       //!< Actual set of tags for this protocol.
            typedef emmgmux::Errors   Errors;     //!< Actual set of error codes for this protocol.
            typedef emmgmux::Protocol Protocol;   //!< Actual protocol description.

            typedef emmgmux::ChannelSetup  ChannelSetup;   //!< Actual ChannelSetup message for this protocol.
            typedef emmgmux::ChannelTest   ChannelTest;    //!< Actual ChannelTest message for this protocol.
            typedef emmgmux::ChannelStatus ChannelStatus;  //!< Actual ChannelStatus message for this protocol.
            typedef emmgmux::ChannelClose  ChannelClose;   //!< Actual ChannelClose message for this protocol.
            typedef emmgmux::ChannelError  ChannelError;   //!< Actual ChannelError message for this protocol.

            typedef emmgmux::StreamSetup         StreamSetup;         //!< Actual StreamSetup message for this protocol.
            typedef emmgmux::StreamTest          StreamTest;          //!< Actual StreamTest message for this protocol.
            typedef emmgmux::StreamStatus        StreamStatus;        //!< Actual StreamStatus message for this protocol.
            typedef emmgmux::StreamCloseRequest  StreamCloseRequest;  //!< Actual StreamCloseRequest message for this protocol.
            typedef emmgmux::StreamCloseResponse StreamCloseResponse; //!< Actual StreamCloseResponse message for this protocol.
            typedef emmgmux::StreamError         StreamError;         //!< Actual StreamError message for this protocol.
        };
    }
}
