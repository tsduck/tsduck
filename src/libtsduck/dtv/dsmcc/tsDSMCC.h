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
    //! Fixed size in bytes of a DSM-CC serverId.
    //!
    constexpr size_t DSMCC_SERVER_ID_SIZE = 20;

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

    //!
    //! DSM-CC tap use.
    //! @see ISO/IEC 13818-6, section 6.6.1.
    //!
    enum : uint16_t {
        DSMCC_TAPUSE_UNKNOWN_USE               =  0,  //!< Unknown use
        DSMCC_TAPUSE_MPEG_TS_UP_USE            =  1,  //!< MPEG transport upstream from Client
        DSMCC_TAPUSE_MPEG_TS_DOWN_USE          =  2,  //!< MPEG transport downstream to Client
        DSMCC_TAPUSE_MPEG_ES_UP_USE            =  3,  //!< MPEG elementary upstream from Client
        DSMCC_TAPUSE_MPEG_ES_DOWN_USE          =  4,  //!< MPEG elementary downstream to Client
        DSMCC_TAPUSE_DOWNLOAD_CTRL_USE         =  5,  //!< control request/response
        DSMCC_TAPUSE_DOWNLOAD_CTRL_UP_USE      =  6,  //!< control request from Client
        DSMCC_TAPUSE_DOWNLOAD_CTRL_DOWN_USE    =  7,  //!< control response to Client
        DSMCC_TAPUSE_DOWNLOAD_DATA_USE         =  8,  //!< data request/response
        DSMCC_TAPUSE_DOWNLOAD_DATA_UP_USE      =  9,  //!< data response upstream from Client
        DSMCC_TAPUSE_DOWNLOAD_DATA_DOWN_USE    = 10,  //!< data block downstream to Client
        DSMCC_TAPUSE_STR_NPT_USE               = 11,  //!< NPT Descriptors
        DSMCC_TAPUSE_STR_STATUS_AND_EVENT_USE  = 12,  //!< Stream Mode and Event Descriptors
        DSMCC_TAPUSE_STR_EVENT_USE             = 13,  //!< Stream Event Descriptor
        DSMCC_TAPUSE_STR_STATUS_USE            = 14,  //!< Stream Mode Descriptor
        DSMCC_TAPUSE_RPC_USE                   = 15,  //!< RPC bi-directional
        DSMCC_TAPUSE_IP_USE                    = 16,  //!< IP bi-directional
        DSMCC_TAPUSE_SDB_CTRL_USE              = 17,  //!< control channel for Switched Digital Broadcast
        DSMCC_TAPUSE_T120_TAP1                 = 18,  //!< reserved for use and definition by T.120
        DSMCC_TAPUSE_T120_TAP2                 = 19,  //!< reserved for use and definition by T.120
        DSMCC_TAPUSE_T120_TAP3                 = 20,  //!< reserved for use and definition by T.120
        DSMCC_TAPUSE_T120_TAP4                 = 21,  //!< reserved for use and definition by T.120
        DSMCC_TAPUSE_BIOP_DELIVERY_PARA_USE    = 22,  //!< Module delivery parameters
        DSMCC_TAPUSE_BIOP_OBJECT_USE           = 23,  //!< BIOP objects in Modules
        DSMCC_TAPUSE_BIOP_ES_USE               = 24,  //!< Elementary Stream
        DSMCC_TAPUSE_BIOP_PROGRAM_USE          = 25,  //!< Program
        DSMCC_TAPUSE_BIOP_DNL_CTRL_USE         = 26,  //!< Download control messages
    };

    //!
    //! DSM-CC Tap selector types.
    //! @see ISO/IEC 13818-6, section 5.6.1.1.
    //! @see ATSC A/90, section 12.2.2.1.
    //!
    enum : uint16_t {
        DSMCC_TAPSELTYPE_MESSAGE = 1,  //!< Message selector, contains a transaction id and a timeout.
    };

    //!
    //! DSM-CC descriptorType in a compatibilityDescriptor().
    //! @see ISO/IEC 13818-6, 6.1
    //!
    enum : uint8_t {
        DSMCC_DTYPE_PAD      = 0x00,  //!< Pad descriptor.
        DSMCC_DTYPE_HARDWARE = 0x01,  //!< System Hardware descriptor.
        DSMCC_DTYPE_SOFTWARE = 0x02,  //!< System Software descriptor.
    };

    //!
    //! DSM-CC specifierType in a compatibilityDescriptor().
    //! @see ISO/IEC 13818-6, 6.1
    //!
    enum : uint8_t {
        DSMCC_SPTYPE_OUI = 0x01,  //!< IEEE OUI.
    };

    //!
    //! DSM-CC resourceDescriptorType in a commonDescriptorHeader().
    //! @see ISO/IEC 13818-6, 4.7.5
    //!
    enum : uint16_t {
        DSMCC_RDTYPE_RESERVED                = 0x0000,  //!< Reserved
        DSMCC_RDTYPE_CONTINUOUS_FEED_SESSION = 0x0001,  //!< ContinuousFeedSession
        DSMCC_RDTYPE_ATM_CONNECTION          = 0x0002,  //!< AtmConnection
        DSMCC_RDTYPE_MPEG_PROGRAM            = 0x0003,  //!< MpegProgram
        DSMCC_RDTYPE_PHYSICAL_CHANNEL        = 0x0004,  //!< PhysicalChannel
        DSMCC_RDTYPE_TS_UPSTREAM_BANDWIDTH   = 0x0005,  //!< TSUpstreamBandwidth
        DSMCC_RDTYPE_TS_DOWNSTREAM_BANDWIDTH = 0x0006,  //!< TSDownstreamBandwidth
        DSMCC_RDTYPE_ATM_SVC_CONNECTION      = 0x0007,  //!< AtmSvcConnection
        DSMCC_RDTYPE_CONNECTION_NOTIFY       = 0x0008,  //!< ConnectionNotify
        DSMCC_RDTYPE_IP                      = 0x0009,  //!< IP
        DSMCC_RDTYPE_CLIENT_TDMA_ASSIGNMENT  = 0x000A,  //!< ClientTDMAAssignment
        DSMCC_RDTYPE_PSTN_SETUP              = 0x000B,  //!< PSTNSetup
        DSMCC_RDTYPE_NISDN_SETUP             = 0x000C,  //!< NISDNSetup
        DSMCC_RDTYPE_NISDN_CONNECTION        = 0x000D,  //!< NISDNConnection
        DSMCC_RDTYPE_Q922_CONNECTIONS        = 0x000E,  //!< Q.922Connections
        DSMCC_RDTYPE_HEADEND_LIST            = 0x000F,  //!< HeadEndList
        DSMCC_RDTYPE_ATM_VC_CONNECTION       = 0x0010,  //!< AtmVcConnection
        DSMCC_RDTYPE_SDB_CONTINUOUS_FEED     = 0x0011,  //!< SdbContinuousFeed
        DSMCC_RDTYPE_SDB_ASSOCIATIONS        = 0x0012,  //!< SdbAssociations
        DSMCC_RDTYPE_SDB_ENTITLEMENT         = 0x0013,  //!< SdbEntitlement
        DSMCC_RDTYPE_SHARED_RESOURCE         = 0x7FFE,  //!< SharedResource
        DSMCC_RDTYPE_SHARED_REQUEST_ID       = 0x7FFF,  //!< SharedRequestId
        DSMCC_RDTYPE_TYPE_OWNER              = 0xFFFF,  //!< TypeOwner
    };
}
