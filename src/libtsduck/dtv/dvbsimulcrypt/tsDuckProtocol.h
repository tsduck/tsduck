//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck tlv
//!  TSDuck internal messages, based on DVB SimulCrypt head-end TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstlvProtocol.h"
#include "tsTablesPtr.h"
#include "tsSimulCryptDate.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Definitions of the TSDuck internal messaging protocol.
    //!
    //! Note that none of the assigned values overlap with the message_type
    //! values which are defined in DVB Simulcrypt protocols. They are
    //! allocated in the "user defined" range. Thus, a generic TLV message
    //! parser can be use for both DVB and TSDuck interfaces.
    //!
    //! Definition of messages:
    //! @code
    //!
    //! MSG_LOG_SECTION
    //! Contains one section.
    //!     Parameter      Count
    //!     PRM_PID        0-1
    //!     PRM_TIMESTAMP  0-1
    //!     PRM_SECTION    1
    //!
    //! MSG_LOG_TABLE
    //! Contains one complete table (no missing section).
    //!     Parameter      Count
    //!     PRM_PID        0-1
    //!     PRM_TIMESTAMP  0-1
    //!     PRM_SECTION    1-n
    //!
    //! MSG_ECM
    //! Contains the section payload of a "fake" ECM as generated by tsecmg.
    //!     Parameter           Count
    //!     PRM_CW_EVEN         0-1
    //!     PRM_CW_ODD          0-1
    //!     PRM_ACCESS_CRITERIA 0-1
    //!
    //! MSG_ERROR
    //! Contains an error code.
    //!     Parameter           Count
    //!     PRM_ERROR_CODE      1
    //!
    //! @endcode
    //!
    //! Definition of parameters:
    //! @code
    //!
    //! PRM_PID
    //!     A 2-byte PID value.
    //!
    //! PRM_TIMESTAMP
    //!     A timestamp identifying the occurence of the event. Same format
    //!     as the activation_time in the EIS<=>SCS DVB Simulcrypt protocol:
    //!        year       2 bytes
    //!        month      1 byte
    //!        day        1 byte
    //!        hour       1 byte
    //!        minute     1 byte
    //!        second     1 byte
    //!        hundredth  1 byte
    //!
    //! PRM_SECTION
    //!     A complete section, including header.
    //!
    //! PRM_ERROR_CODE
    //!     A 2-byte error code.
    //!
    //! PRM_CW_EVEN
    //! PRM_CW_ODD
    //! PRM_ACCESS_CRITERIA
    //!     Binary data as sent by the SCS. Included in "fake" ECM's ty tsecmg.
    //!
    //! @endcode
    //!
    namespace duck {

        //---------------------------------------------------------------------
        // Protocol-defined values
        //---------------------------------------------------------------------

        //! Current version of the TSDuck internal messaging protocol
        const tlv::VERSION CURRENT_VERSION = 0x80;

        //!
        //! All TSDuck messaging tags (commands and parameters).
        //! Tags is defined as struct instead of namespace to be used as traits.
        //!
        struct TSDUCKDLL Tags {
            //!
            //! TSDuck messaging command tags.
            //!
            enum Command {
                MSG_LOG_SECTION = 0xAA01,  //!< Log a section.
                MSG_LOG_TABLE   = 0xAA02,  //!< Log a table.
                MSG_ECM         = 0xAA03,  //!< Fake ECM.
                MSG_ERROR       = 0xAA04,  //!< Error message.
            };
            //!
            //! TSDuck messaging parameter tags.
            //!
            enum Parameter {
                PRM_PID             = 0x0000,  //!< A PID value, 2 bytes.
                PRM_TIMESTAMP       = 0x0001,  //!< Timestamp, 8 bytes.
                PRM_SECTION         = 0x0002,  //!< Complete section.
                PRM_ERROR_CODE      = 0x0003,  //!< Error code.
                PRM_CW_EVEN         = 0x0010,  //!< Even control word.
                PRM_CW_ODD          = 0x0011,  //!< Odd control word.
                PRM_ACCESS_CRITERIA = 0x0012,  //!< Full access criteria.
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
                inv_message           = 0x0001,  //!< The inv_message error status value.
                inv_proto_version     = 0x0002,  //!< The inv_proto_version error status value.
                inv_message_type      = 0x0003,  //!< The inv_message_type error status value.
                message_too_long      = 0x0004,  //!< The message_too_long error status value.
                inv_param_type        = 0x0005,  //!< The inv_param_type error status value.
                inv_param_length      = 0x0006,  //!< The inv_param_length error status value.
                missing_param         = 0x0007,  //!< The missing_param error status value.
                inv_param_value       = 0x0008,  //!< The inv_param_value error status value.
                unknown_error         = 0x7000,  //!< The unknown_error error status value.
                unrecoverable_error   = 0x7001,  //!< The unrecoverable_error error status value.
            };
        };


        //---------------------------------------------------------------------
        //! Generic description of the TSDuck internal messaging protocol.
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
        // Definition of all TSDuck internal protocol messages
        //---------------------------------------------------------------------

        //!
        //! Message to log a section.
        //!
        class TSDUCKDLL LogSection : public tlv::Message
        {
            TS_VERSIONED_TLV_MESSAGE(LogSection, Tags::MSG_LOG_SECTION);
        public:
            std::optional<PID>            pid {};        //!< PID where the section was found.
            std::optional<SimulCryptDate> timestamp {};  //!< Date and time of the extraction.
            SectionPtr                    section {};    //!< Content of the section.
        };

        //!
        //! Message to log a table.
        //!
        class TSDUCKDLL LogTable : public tlv::Message
        {
            TS_VERSIONED_TLV_MESSAGE(LogTable, Tags::MSG_LOG_TABLE);
        public:
            std::optional<PID>            pid {};        //!< PID where the table was found.
            std::optional<SimulCryptDate> timestamp {};  //!< Date and time of the extraction.
            SectionPtrVector              sections {};   //!< All sections in the table.
        };

        //!
        //! Fake / demo  clear ECM.
        //!
        class TSDUCKDLL ClearECM : public tlv::Message
        {
            TS_VERSIONED_TLV_MESSAGE(ClearECM, Tags::MSG_ECM);
        public:
            ByteBlock cw_even {};          //!< Odd control word.
            ByteBlock cw_odd {};           //!< Odd control word.
            ByteBlock access_criteria {};  //!< Access criteria.
        };

        //!
        //! Error message.
        //!
        class TSDUCKDLL Error : public tlv::Message
        {
            TS_VERSIONED_TLV_MESSAGE(Error, Tags::MSG_ERROR);
        public:
            uint16_t error_status = 0;   //!< Error code.
        };


        //---------------------------------------------------------------------
        //! Generic "traits" for the TSDuck internal messaging protocol.
        //---------------------------------------------------------------------

        struct TSDUCKDLL Traits {
            using Tags = ts::duck::Tags;           //!< Actual set of tags for this protocol.
            using Errors = ts::duck::Errors;       //!< Actual set of error codes for this protocol.
            using Protocol = ts::duck::Protocol;   //!< Actual protocol description.
        };
    }
}
