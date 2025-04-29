//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtsduck mpeg
//!  Generic DSM-CC definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! The protocolDiscriminator field is used to indicate that the message is a MPEG-2 DSM-CC message.
    //! @see ISO/IEC 13818-6, section 2.
    //!
    constexpr uint8_t DSMCC_PROTOCOL_DISCRIMINATOR = 0x11;

    //!
    //! DSM-CC types, in dsmccMessageHeader() structures.
    //! @see ISO/IEC 13818-6, table 2-2.
    //!
    enum : uint8_t {
        DSMCC_TYPE_UN_CONFIG_MESSAGE   = 0x01,  //!< User-to-Network configuration message.
        DSMCC_TYPE_UN_SESSION_MESSAGE  = 0x02,  //!< User-to-Network session message.
        DSMCC_TYPE_DOWNLOAD_MESSAGE    = 0x03,  //!< Download message.
        DSMCC_TYPE_SDB_CCP_MESSAGE     = 0x04,  //!< SDB Channel Change Protocol message.
        DSMCC_TYPE_UN_PASSTHRU_MESSAGE = 0x05,  //!< User-to-Network pass-thru message.
    };

    //!
    //! DSM-CC message id, in dsmccMessageHeader() structures.
    //!
    enum : uint16_t {
        DSMCC_MSGID_UNConfigRequest                    = 0x0001,  //!< UNConfigRequest
        DSMCC_MSGID_UNConfigConfirm                    = 0x0002,  //!< UNConfigConfirm
        DSMCC_MSGID_UNConfigIndication                 = 0x0003,  //!< UNConfigIndication
        DSMCC_MSGID_UNConfigResponse                   = 0x0004,  //!< UNConfigResponse
        DSMCC_MSGID_DownloadInfoRequest                = 0x1001,  //!< DownloadInfoRequest
        DSMCC_MSGID_DownloadInfoIndication             = 0x1002,  //!< DownloadInfoIndication
        DSMCC_MSGID_DII                                = 0x1002,  //!< DownloadInfoIndication (common name)
        DSMCC_MSGID_DownloadDataBlock                  = 0x1003,  //!< DownloadDataBlock
        DSMCC_MSGID_DDB                                = 0x1003,  //!< DownloadDataBlock (common name)
        DSMCC_MSGID_DownloadDataRequest                = 0x1004,  //!< DownloadDataRequest
        DSMCC_MSGID_DownloadCancel                     = 0x1005,  //!< DownloadCancel
        DSMCC_MSGID_DownloadServerInitiate             = 0x1006,  //!< DownloadServerInitiate
        DSMCC_MSGID_DSI                                = 0x1006,  //!< DownloadServerInitiate (common name)
        DSMCC_MSGID_ClientSessionSetUpRequest          = 0x4010,  //!< ClientSessionSetUpRequest
        DSMCC_MSGID_ClientSessionSetUpConfirm          = 0x4011,  //!< ClientSessionSetUpConfirm
        DSMCC_MSGID_ClientSessionReleaseRequest        = 0x4020,  //!< ClientSessionReleaseRequest
        DSMCC_MSGID_ClientSessionReleaseConfirm        = 0x4021,  //!< ClientSessionReleaseConfirm
        DSMCC_MSGID_ClientSessionReleaseIndication     = 0x4022,  //!< ClientSessionReleaseIndication
        DSMCC_MSGID_ClientSessionReleaseResponse       = 0x4023,  //!< ClientSessionReleaseResponse
        DSMCC_MSGID_ClientAddResourceIndication        = 0x4032,  //!< ClientAddResourceIndication
        DSMCC_MSGID_ClientAddResourceResponse          = 0x4033,  //!< ClientAddResourceResponse
        DSMCC_MSGID_ClientDeleteResourceIndication     = 0x4042,  //!< ClientDeleteResourceIndication
        DSMCC_MSGID_ClientDeleteResourceResponse       = 0x4043,  //!< ClientDeleteResourceResponse
        DSMCC_MSGID_ClientStatusRequest                = 0x4060,  //!< ClientStatusRequest
        DSMCC_MSGID_ClientStatusConfirm                = 0x4061,  //!< ClientStatusConfirm
        DSMCC_MSGID_ClientStatusIndication             = 0x4062,  //!< ClientStatusIndication
        DSMCC_MSGID_ClientStatusResponse               = 0x4063,  //!< ClientStatusResponse
        DSMCC_MSGID_ClientResetRequest                 = 0x4070,  //!< ClientResetRequest
        DSMCC_MSGID_ClientResetConfirm                 = 0x4071,  //!< ClientResetConfirm
        DSMCC_MSGID_ClientResetIndication              = 0x4072,  //!< ClientResetIndication
        DSMCC_MSGID_ClientResetResponse                = 0x4073,  //!< ClientResetResponse
        DSMCC_MSGID_ClientSessionProceedingIndication  = 0x4082,  //!< ClientSessionProceedingIndication
        DSMCC_MSGID_ClientConnectRequest               = 0x4090,  //!< ClientConnectRequest
        DSMCC_MSGID_ServerSessionSetUpIndication       = 0x8012,  //!< ServerSessionSetUpIndication
        DSMCC_MSGID_ServerSessionSetUpResponse         = 0x8013,  //!< ServerSessionSetUpResponse
        DSMCC_MSGID_ServerAddResourceRequest           = 0x8030,  //!< ServerAddResourceRequest
        DSMCC_MSGID_ServerAddResourceConfirm           = 0x8031,  //!< ServerAddResourceConfirm
        DSMCC_MSGID_ServerDeleteResourceRequest        = 0x8040,  //!< ServerDeleteResourceRequest
        DSMCC_MSGID_ServerDeleteResourceConfirm        = 0x8041,  //!< ServerDeleteResourceConfirm
        DSMCC_MSGID_ServerContinuousFeedSessionRequest = 0x8050,  //!< ServerContinuousFeedSessionRequest
        DSMCC_MSGID_ServerContinuousFeedSessionConfirm = 0x8051,  //!< ServerContinuousFeedSessionConfirm
        DSMCC_MSGID_ServerStatusRequest                = 0x8060,  //!< ServerStatusRequest
        DSMCC_MSGID_ServerStatusConfirm                = 0x8061,  //!< ServerStatusConfirm
        DSMCC_MSGID_ServerStatusIndication             = 0x8062,  //!< ServerStatusIndication
        DSMCC_MSGID_ServerStatusResponse               = 0x8063,  //!< ServerStatusResponse
        DSMCC_MSGID_ServerResetRequest                 = 0x8070,  //!< ServerResetRequest
        DSMCC_MSGID_ServerResetConfirm                 = 0x8071,  //!< ServerResetConfirm
        DSMCC_MSGID_ServerResetIndication              = 0x8072,  //!< ServerResetIndication
        DSMCC_MSGID_ServerResetResponse                = 0x8073,  //!< ServerResetResponse
        DSMCC_MSGID_ServerSessionProceedingIndication  = 0x8082,  //!< ServerSessionProceedingIndication
        DSMCC_MSGID_ServerConnectIndication            = 0x8092,  //!< ServerConnectIndication
        DSMCC_MSGID_ServerSessionTransferRequest       = 0x80a0,  //!< ServerSessionTransferRequest
        DSMCC_MSGID_ServerSessionTransferConfirm       = 0x80a1,  //!< ServerSessionTransferConfirm
        DSMCC_MSGID_ServerSessionTransferIndication    = 0x80a2,  //!< ServerSessionTransferIndication
        DSMCC_MSGID_ServerSessionTransferResponse      = 0x80a3,  //!< ServerSessionTransferResponse
        DSMCC_MSGID_ServerSessionInProgressRequest     = 0x80b0,  //!< ServerSessionInProgressRequest
    };

    //!
    //! DSM-CC tags.
    //! @see ISO/IEC 13818-6
    //!
    enum : uint32_t {
        DSMCC_TAG_INTERNET_IOP        = 0x00000000,  //!< TAG_INTERNET_IOP
        DSMCC_TAG_MULTIPLE_COMPONENTS = 0x00000001,  //!< TAG_MULTIPLE_COMPONENTS
        DSMCC_TAG_MIN                 = 0x49534F00,  //!< TAG_MIN
        DSMCC_TAG_CHILD               = 0x49534F01,  //!< TAG_CHILD
        DSMCC_TAG_OPTIONS             = 0x49534F02,  //!< TAG_OPTIONS
        DSMCC_TAG_LITE_MIN            = 0x49534F03,  //!< TAG_LITE_MIN
        DSMCC_TAG_LITE_CHILD          = 0x49534F04,  //!< TAG_LITE_CHILD
        DSMCC_TAG_LITE_OPTIONS        = 0x49534F05,  //!< TAG_LITE_OPTIONS (Lite Options Profile Body)
        DSMCC_TAG_BIOP                = 0x49534F06,  //!< TAG_BIOP (BIOP Profile Body).
        DSMCC_TAG_ONC                 = 0x49534F07,  //!< TAG_ONC
        DSMCC_TAG_CONN_BINDER         = 0x49534F40,  //!< TAG_ConnBinder (DSM::ConnBinder).
        DSMCC_TAG_ConnBinder          = 0x49534F40,  //!< TAG_ConnBinder
        DSMCC_TAG_IIOPAddr            = 0x49534F41,  //!< TAG_IIOPAddr
        DSMCC_TAG_Addr                = 0x49534F42,  //!< TAG_Addr
        DSMCC_TAG_NameId              = 0x49534F43,  //!< TAG_NameId
        DSMCC_TAG_IntfCode            = 0x49534F44,  //!< TAG_IntfCode
        DSMCC_TAG_ObjectKey           = 0x49534F45,  //!< TAG_ObjectKey
        DSMCC_TAG_ServiceLocation     = 0x49534F46,  //!< TAG_ServiceLocation
        DSMCC_TAG_OBJECT_LOCATION     = 0x49534F50,  //!< TAG_ObjectLocation (BIOP::ObjectLocation).
        DSMCC_TAG_ObjectLocation      = 0x49534F50,  //!< TAG_ObjectLocation
        DSMCC_TAG_Intf                = 0x49534F58,  //!< TAG_Intf
    };
}
