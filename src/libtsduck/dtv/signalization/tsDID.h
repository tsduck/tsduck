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
//!  MPEG PSI/SI descriptors identifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"
#include "tsDescriptorContext.h"

namespace ts {

    class DuckContext;

    //!
    //! Descriptor identifier.
    //!
    using DID = uint8_t;

    //!
    //! Maximum number of DID values.
    //!
    constexpr size_t DID_MAX = 0x100;

    //!
    //! Descriptor tag values (descriptor identification, DID)
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.1.
    //! @see ETSI EN 300 468, 6.1.
    //!
    enum : DID {

        DID_NULL = 0xFF,  //!< Invalid DID value, can be used as placeholder.

        // Valid in all MPEG contexts:

        DID_MPEG_VIDEO               = 0x02,  //!< DID for video_stream_descriptor
        DID_MPEG_AUDIO               = 0x03,  //!< DID for audio_stream_descriptor
        DID_MPEG_HIERARCHY           = 0x04,  //!< DID for hierarchy_descriptor
        DID_MPEG_REGISTRATION        = 0x05,  //!< DID for registration_descriptor
        DID_MPEG_DATA_ALIGN          = 0x06,  //!< DID for data_stream_alignment_descriptor
        DID_MPEG_TGT_BG_GRID         = 0x07,  //!< DID for target_background_grid_descriptor
        DID_MPEG_VIDEO_WIN           = 0x08,  //!< DID for video_window_descriptor
        DID_MPEG_CA                  = 0x09,  //!< DID for CA_descriptor
        DID_MPEG_LANGUAGE            = 0x0A,  //!< DID for ISO_639_language_descriptor
        DID_MPEG_SYS_CLOCK           = 0x0B,  //!< DID for system_clock_descriptor
        DID_MPEG_MUX_BUF_USE         = 0x0C,  //!< DID for multiplex_buffer_utilization_desc
        DID_MPEG_COPYRIGHT           = 0x0D,  //!< DID for copyright_descriptor
        DID_MPEG_MAX_BITRATE         = 0x0E,  //!< DID for maximum bitrate descriptor
        DID_MPEG_PRIV_DATA_IND       = 0x0F,  //!< DID for private data indicator descriptor
        DID_MPEG_SMOOTH_BUF          = 0x10,  //!< DID for smoothing buffer descriptor
        DID_MPEG_STD                 = 0x11,  //!< DID for STD_descriptor
        DID_MPEG_IBP                 = 0x12,  //!< DID for IBP_descriptor
        DID_MPEG_CAROUSEL_IDENTIFIER = 0x13,  //!< DID for DSM-CC carousel identifier descriptor
        DID_MPEG_ASSOCIATION_TAG     = 0x14,  //!< DID for DSM-CC association tag descriptor
        DID_MPEG_DEFERRED_ASSOC_TAGS = 0x15,  //!< DID for DSM-CC deferred association tags descriptor
        DID_MPEG_NPT_REFERENCE       = 0x17,  //!< DID for DSM-CC NPT reference descriptor
        DID_MPEG_NPT_ENDPOINT        = 0x18,  //!< DID for DSM-CC NPT endpoint descriptor
        DID_MPEG_STREAM_MODE         = 0x19,  //!< DID for DSM-CC stream mode descriptor
        DID_MPEG_STREAM_EVENT        = 0x1A,  //!< DID for DSM-CC stream event descriptor
        DID_MPEG_MPEG4_VIDEO         = 0x1B,  //!< DID for MPEG-4_video_descriptor
        DID_MPEG_MPEG4_AUDIO         = 0x1C,  //!< DID for MPEG-4_audio_descriptor
        DID_MPEG_IOD                 = 0x1D,  //!< DID for IOD_descriptor
        DID_MPEG_SL                  = 0x1E,  //!< DID for SL_descriptor
        DID_MPEG_FMC                 = 0x1F,  //!< DID for FMC_descriptor
        DID_MPEG_EXT_ES_ID           = 0x20,  //!< DID for External_ES_id_descriptor
        DID_MPEG_MUXCODE             = 0x21,  //!< DID for MuxCode_descriptor
        DID_MPEG_M4MUX_BUFFER_SIZE   = 0x22,  //!< DID for M4MuxBufferSize_descriptor
        DID_MPEG_MUX_BUFFER          = 0x23,  //!< DID for MultiplexBuffer_descriptor
        DID_MPEG_CONTENT_LABELLING   = 0x24,  //!< DID for Content_labelling_descriptor
        DID_MPEG_METADATA_POINTER    = 0x25,  //!< DID for metadata_pointer_descriptor
        DID_MPEG_METADATA            = 0x26,  //!< DID for metadata_descriptor
        DID_MPEG_METADATA_STD        = 0x27,  //!< DID for metadata_STD_descriptor
        DID_MPEG_AVC_VIDEO           = 0x28,  //!< DID for AVC_video_descriptor
        DID_MPEG_MPEG2_IPMP          = 0x29,  //!< DID for MPEG-2_IPMP_descriptor
        DID_MPEG_AVC_TIMING_HRD      = 0x2A,  //!< DID for AVC_timing_and_HRD_descriptor
        DID_MPEG_MPEG2_AAC_AUDIO     = 0x2B,  //!< DID for MPEG-2 AAC Audio descriptor
        DID_MPEG_M4_MUX_TIMING       = 0x2C,  //!< DID for M4MuxTiming descriptor
        DID_MPEG_MPEG4_TEXT          = 0x2D,  //!< DID for MPEG-4 Text descriptor
        DID_MPEG_MPEG4_AUDIO_EXT     = 0x2E,  //!< DID for MPEG-4 Audio Extension descriptor
        DID_MPEG_AUX_VIDEO           = 0x2F,  //!< DID for Auxiliary Video Stream descriptor
        DID_MPEG_SVC_EXT             = 0x30,  //!< DID for SVC Extension descriptor
        DID_MPEG_MVC_EXT             = 0x31,  //!< DID for MVC Extension descriptor
        DID_MPEG_J2K_VIDEO           = 0x32,  //!< DID for J2K Video descriptor
        DID_MPEG_MVC_OPER_POINT      = 0x33,  //!< DID for MVC Operation Point descriptor
        DID_MPEG_STEREO_VIDEO_FORMAT = 0x34,  //!< DID for MPEG-2 Stereoscopic Video Format descriptor
        DID_MPEG_STEREO_PROG_INFO    = 0x35,  //!< DID for Stereoscopic Program Info descriptor
        DID_MPEG_STEREO_VIDEO_INFO   = 0x36,  //!< DID for Stereoscopic Video Info descriptor
        DID_MPEG_TRANSPORT_PROFILE   = 0x37,  //!< DID for Transport Profile descriptor
        DID_MPEG_HEVC_VIDEO          = 0x38,  //!< DID for HEVC Video descriptor
        DID_MPEG_VVC_VIDEO           = 0x39,  //!< DID for VVC Video descriptor
        DID_MPEG_EVC_VIDEO           = 0x3A,  //!< DID for EVC Video descriptor
        DID_MPEG_EXTENSION           = 0x3F,  //!< DID for MPEG-2 Extension descriptor

        // Valid in DVB context:

        DID_DVB_NETWORK_NAME        = 0x40,  //!< DID for DVB network_name_descriptor
        DID_DVB_SERVICE_LIST        = 0x41,  //!< DID for DVB service_list_descriptor
        DID_DVB_STUFFING            = 0x42,  //!< DID for DVB stuffing_descriptor
        DID_DVB_SAT_DELIVERY        = 0x43,  //!< DID for DVB satellite_delivery_system_desc
        DID_DVB_CABLE_DELIVERY      = 0x44,  //!< DID for DVB cable_delivery_system_descriptor
        DID_DVB_VBI_DATA            = 0x45,  //!< DID for DVB VBI_data_descriptor
        DID_DVB_VBI_TELETEXT        = 0x46,  //!< DID for DVB VBI_teletext_descriptor
        DID_DVB_BOUQUET_NAME        = 0x47,  //!< DID for DVB bouquet_name_descriptor
        DID_DVB_SERVICE             = 0x48,  //!< DID for DVB service_descriptor
        DID_DVB_COUNTRY_AVAIL       = 0x49,  //!< DID for DVB country_availability_descriptor
        DID_DVB_LINKAGE             = 0x4A,  //!< DID for DVB linkage_descriptor
        DID_DVB_NVOD_REFERENCE      = 0x4B,  //!< DID for DVB NVOD_reference_descriptor
        DID_DVB_TIME_SHIFT_SERVICE  = 0x4C,  //!< DID for DVB time_shifted_service_descriptor
        DID_DVB_SHORT_EVENT         = 0x4D,  //!< DID for DVB short_event_descriptor
        DID_DVB_EXTENDED_EVENT      = 0x4E,  //!< DID for DVB extended_event_descriptor
        DID_DVB_TIME_SHIFT_EVENT    = 0x4F,  //!< DID for DVB time_shifted_event_descriptor
        DID_DVB_COMPONENT           = 0x50,  //!< DID for DVB component_descriptor
        DID_DVB_MOSAIC              = 0x51,  //!< DID for DVB mosaic_descriptor
        DID_DVB_STREAM_ID           = 0x52,  //!< DID for DVB stream_identifier_descriptor
        DID_DVB_CA_ID               = 0x53,  //!< DID for DVB CA_identifier_descriptor
        DID_DVB_CONTENT             = 0x54,  //!< DID for DVB content_descriptor
        DID_DVB_PARENTAL_RATING     = 0x55,  //!< DID for DVB parental_rating_descriptor
        DID_DVB_TELETEXT            = 0x56,  //!< DID for DVB teletext_descriptor
        DID_DVB_TELEPHONE           = 0x57,  //!< DID for DVB telephone_descriptor
        DID_DVB_LOCAL_TIME_OFFSET   = 0x58,  //!< DID for DVB local_time_offset_descriptor
        DID_DVB_SUBTITLING          = 0x59,  //!< DID for DVB subtitling_descriptor
        DID_DVB_TERREST_DELIVERY    = 0x5A,  //!< DID for DVB terrestrial_delivery_system_desc
        DID_DVB_MLINGUAL_NETWORK    = 0x5B,  //!< DID for DVB multilingual_network_name_desc
        DID_DVB_MLINGUAL_BOUQUET    = 0x5C,  //!< DID for DVB multilingual_bouquet_name_desc
        DID_DVB_MLINGUAL_SERVICE    = 0x5D,  //!< DID for DVB multilingual_service_name_desc
        DID_DVB_MLINGUAL_COMPONENT  = 0x5E,  //!< DID for DVB multilingual_component_descriptor
        DID_DVB_PRIV_DATA_SPECIF    = 0x5F,  //!< DID for DVB private_data_specifier_descriptor
        DID_DVB_SERVICE_MOVE        = 0x60,  //!< DID for DVB service_move_descriptor
        DID_DVB_SHORT_SMOOTH_BUF    = 0x61,  //!< DID for DVB short_smoothing_buffer_descriptor
        DID_DVB_FREQUENCY_LIST      = 0x62,  //!< DID for DVB frequency_list_descriptor
        DID_DVB_PARTIAL_TS          = 0x63,  //!< DID for DVB partial_transport_stream_desc
        DID_DVB_DATA_BROADCAST      = 0x64,  //!< DID for DVB data_broadcast_descriptor
        DID_DVB_SCRAMBLING          = 0x65,  //!< DID for DVB scrambling_descriptor
        DID_DVB_DATA_BROADCAST_ID   = 0x66,  //!< DID for DVB data_broadcast_id_descriptor
        DID_DVB_TRANSPORT_STREAM    = 0x67,  //!< DID for DVB transport_stream_descriptor
        DID_DVB_DSNG                = 0x68,  //!< DID for DVB DSNG_descriptor
        DID_DVB_PDC                 = 0x69,  //!< DID for DVB PDC_descriptor
        DID_DVB_AC3                 = 0x6A,  //!< DID for DVB AC-3_descriptor
        DID_DVB_ANCILLARY_DATA      = 0x6B,  //!< DID for DVB ancillary_data_descriptor
        DID_DVB_CELL_LIST           = 0x6C,  //!< DID for DVB cell_list_descriptor
        DID_DVB_CELL_FREQ_LINK      = 0x6D,  //!< DID for DVB cell_frequency_link_descriptor
        DID_DVB_ANNOUNCE_SUPPORT    = 0x6E,  //!< DID for DVB announcement_support_descriptor
        DID_DVB_APPLI_SIGNALLING    = 0x6F,  //!< DID for DVB application_signalling_descriptor
        DID_DVB_ADAPTFIELD_DATA     = 0x70,  //!< DID for DVB adaptation_field_data_descriptor
        DID_DVB_SERVICE_ID          = 0x71,  //!< DID for DVB service_identifier_descriptor
        DID_DVB_SERVICE_AVAIL       = 0x72,  //!< DID for DVB service_availability_descriptor
        DID_DVB_DEFAULT_AUTHORITY   = 0x73,  //!< DID for DVB default_authority_descriptor
        DID_DVB_RELATED_CONTENT     = 0x74,  //!< DID for DVB related_content_descriptor
        DID_DVB_TVA_ID              = 0x75,  //!< DID for DVB TVA_id_descriptor
        DID_DVB_CONTENT_ID          = 0x76,  //!< DID for DVB content_identifier_descriptor
        DID_DVB_TIME_SLICE_FEC_ID   = 0x77,  //!< DID for DVB time_slice_fec_identifier_desc
        DID_DVB_ECM_REPETITION_RATE = 0x78,  //!< DID for DVB ECM_repetition_rate_descriptor
        DID_DVB_S2_SAT_DELIVERY     = 0x79,  //!< DID for DVB S2_satellite_delivery_system_descriptor
        DID_DVB_ENHANCED_AC3        = 0x7A,  //!< DID for DVB enhanced_AC-3_descriptor
        DID_DVB_DTS                 = 0x7B,  //!< DID for DVB DTS_descriptor
        DID_DVB_AAC                 = 0x7C,  //!< DID for DVB AAC_descriptor
        DID_DVB_XAIT_LOCATION       = 0x7D,  //!< DID for DVB XAIT_location_descriptor (DVB-MHP)
        DID_DVB_FTA_CONTENT_MGMT    = 0x7E,  //!< DID for DVB FTA_content_management_descriptor
        DID_DVB_EXTENSION           = 0x7F,  //!< DID for DVB extension_descriptor

        // Valid only in a DVB AIT (Application Information Table, ETSI TS 102 809):

        DID_AIT_APPLICATION     = 0x00,  //!< DID for AIT application_descriptor.
        DID_AIT_APP_NAME        = 0x01,  //!< DID for AIT application_name_descriptor.
        DID_AIT_TRANSPORT_PROTO = 0x02,  //!< DID for AIT transport_protocol_descriptor.
        DID_AIT_DVBJ_APP        = 0x03,  //!< DID for AIT dvb_j_application_descriptor.
        DID_AIT_DVBJ_APP_LOC    = 0x04,  //!< DID for AIT dvb_j_application_location_descriptor.
        DID_AIT_EXT_APP_AUTH    = 0x05,  //!< DID for AIT external_application_authorisation_descriptor.
        DID_AIT_APP_RECORDING   = 0x06,  //!< DID for AIT application_recording_descriptor.
        DID_AIT_HTML_APP        = 0x08,  //!< DID for AIT dvb_html_application_descriptor.
        DID_AIT_HTML_APP_LOC    = 0x09,  //!< DID for AIT dvb_html_application_location_descriptor.
        DID_AIT_HTML_APP_BOUND  = 0x0A,  //!< DID for AIT dvb_html_application_boundary_descriptor.
        DID_AIT_APP_ICONS       = 0x0B,  //!< DID for AIT application_icons_descriptor.
        DID_AIT_PREFETCH        = 0x0C,  //!< DID for AIT prefetch_descriptor.
        DID_AIT_DII_LOCATION    = 0x0D,  //!< DID for AIT DII_location_descriptor.
        DID_AIT_APP_STORAGE     = 0x10,  //!< DID for AIT application_storage_descriptor.
        DID_AIT_IP_SIGNALLING   = 0x11,  //!< DID for AIT IP_signalling_descriptor.
        DID_AIT_GRAPHICS_CONST  = 0x14,  //!< DID for AIT graphics_constraints_descriptor.
        DID_AIT_APP_LOCATION    = 0x15,  //!< DID for AIT simple_application_location_descriptor.
        DID_AIT_APP_USAGE       = 0x16,  //!< DID for AIT application_usage_descriptor.
        DID_AIT_APP_BOUNDARY    = 0x17,  //!< DID for AIT simple_application_boundary_descriptor.

        // Valid in DVB data carousel context (ISO/IEC 13818-6, ETSI EN 301 192, ETSI TR 101 202, ETSI TS 102 809):

        DID_DSMCC_TYPE                 = 0x01, //!< DID for DSM-CC U-N Message DSI/DII type_descriptor.
        DID_DSMCC_NAME                 = 0x02, //!< DID for DSM-CC U-N Message DSI/DII name_descriptor.
        DID_DSMCC_INFO                 = 0x03, //!< DID for DSM-CC U-N Message DSI/DII info_descriptor.
        DID_DSMCC_MODULE_LINK          = 0x04, //!< DID for DSM-CC U-N Message DII module_link_descriptor.
        DID_DSMCC_CRC32                = 0x05, //!< DID for DSM-CC U-N Message DII CRC32_descriptor.
        DID_DSMCC_LOCATION             = 0x06, //!< DID for DSM-CC U-N Message DSI/DII location_descriptor.
        DID_DSMCC_EST_DOWNLOAD_TIME    = 0x07, //!< DID for DSM-CC U-N Message DSI/DII est_download_time_descriptor.
        DID_DSMCC_GROUP_LINK           = 0x08, //!< DID for DSM-CC U-N Message DSI group_link_descriptor.
        DID_DSMCC_COMPRESSED_MODULE    = 0x09, //!< DID for DSM-CC U-N Message DII compressed_module_descriptor.
        DID_DSMCC_SSU_MODULE_TYPE      = 0x0A, //!< DID for DSM-CC U-N Message DII ssu_module_type_descriptor.
        DID_DSMCC_SUBGROUP_ASSOCIATION = 0x0B, //!< DID for DSM-CC U-N Message DSI subgroup_association_descriptor.

        // Valid in DVB MHP/HbbTV object carousel context (ETSI TS 102 727, ETSI TS 102 809):

        DID_DSMCC_LABEL            = 0x70, //!< DID for DSM-CC U-N Message DII label_descriptor.
        DID_DSMCC_CACHING_PRIORITY = 0x71, //!< DID for DSM-CC U-N Message DII caching_priority_descriptor.
        DID_DSMCC_CONTENT_TYPE     = 0x72, //!< DID for DSM-CC U-N Message DII content_type_descriptor.

        // Valid only in a DVB INT (IP/MAC Notification Table, ETSI EN 301 192):

        DID_INT_SMARTCARD      = 0x06,  //!< DID for INT target_smartcard_descriptor
        DID_INT_MAC_ADDR       = 0x07,  //!< DID for INT target_MAC_address_descriptor
        DID_INT_SERIAL_NUM     = 0x08,  //!< DID for INT target_serial_number_descriptor
        DID_INT_IP_ADDR        = 0x09,  //!< DID for INT target_IP_address_descriptor
        DID_INT_IPV6_ADDR      = 0x0A,  //!< DID for INT target_IPv6_address_descriptor
        DID_INT_PF_NAME        = 0x0C,  //!< DID for INT IP/MAC_platform_name_descriptor
        DID_INT_PF_PROVIDER    = 0x0D,  //!< DID for INT IP/MAC_platform_provider_name_descriptor
        DID_INT_MAC_ADDR_RANGE = 0x0E,  //!< DID for INT target_MAC_address_range_descriptor
        DID_INT_IP_SLASH       = 0x0F,  //!< DID for INT target_IP_slash_descriptor
        DID_INT_IP_SRC_SLASH   = 0x10,  //!< DID for INT target_IP_source_slash_descriptor
        DID_INT_IPV6_SLASH     = 0x11,  //!< DID for INT target_IPv6_slash_descriptor
        DID_INT_IPV6_SRC_SLASH = 0x12,  //!< DID for INT target_IPv6_source_slash_descriptor
        DID_INT_STREAM_LOC     = 0x13,  //!< DID for INT IP/MAC_stream_location_descriptor
        DID_INT_ISP_ACCESS     = 0x14,  //!< DID for INT ISP_access_mode_descriptor
        DID_INT_GEN_STREAM_LOC = 0x15,  //!< DID for INT IP/MAC_generic_stream_location_descriptor

        // Valid only in a DVB UNT (Update Notification Table, ETSI TS 102 006):

        DID_UNT_SCHEDULING     = 0x01,  //!< DID for UNT scheduling_descriptor
        DID_UNT_UPDATE         = 0x02,  //!< DID for UNT update_descriptor
        DID_UNT_SSU_LOCATION   = 0x03,  //!< DID for UNT ssu_location_descriptor
        DID_UNT_MESSAGE        = 0x04,  //!< DID for UNT message_descriptor
        DID_UNT_SSU_EVENT_NAME = 0x05,  //!< DID for UNT ssu_event_name_descriptor
        DID_UNT_SMARTCARD      = 0x06,  //!< DID for UNT target_smartcard_descriptor
        DID_UNT_MAC_ADDR       = 0x07,  //!< DID for UNT target_MAC_address_descriptor
        DID_UNT_SERIAL_NUM     = 0x08,  //!< DID for UNT target_serial_number_descriptor
        DID_UNT_IP_ADDR        = 0x09,  //!< DID for UNT target_IP_address_descriptor
        DID_UNT_IPV6_ADDR      = 0x0A,  //!< DID for UNT target_IPv6_address_descriptor
        DID_UNT_SUBGROUP_ASSOC = 0x0B,  //!< DID for UNT ssu_subgroup_association_descriptor
        DID_UNT_ENHANCED_MSG   = 0x0C,  //!< DID for UNT enhanced_message_descriptor
        DID_UNT_SSU_URI        = 0x0D,  //!< DID for UNT ssu_uri_descriptor

        // Valid only in a DVB RNT (RAR Notification Table, ETSI TS 102 323):

        DID_RNT_RAR_OVER_DVB = 0x40,  //!< DID for RNT RAR_over_DVB_stream_descriptor
        DID_RNT_RAR_OVER_IP  = 0x41,  //!< DID for RNT RAR_over_IP_descriptor
        DID_RNT_SCAN         = 0x42,  //!< DID for RNT RNT_scan_dscriptor

        // Valid in DVB context after PDS_LOGIWAYS private_data_specifier

        DID_LW_SUBSCRIPTION      = 0x81,  //!< DID for Logiways subscription_descriptor
        DID_LW_SCHEDULE          = 0xB0,  //!< DID for Logiways schedule_descriptor
        DID_LW_PRIV_COMPONENT    = 0xB1,  //!< DID for Logiways private_component_descriptor
        DID_LW_PRIV_LINKAGE      = 0xB2,  //!< DID for Logiways private_linkage_descriptor
        DID_LW_CHAPTER           = 0xB3,  //!< DID for Logiways chapter_descriptor
        DID_LW_DRM               = 0xB4,  //!< DID for Logiways DRM_descriptor
        DID_LW_VIDEO_SIZE        = 0xB5,  //!< DID for Logiways video_size_descriptor
        DID_LW_EPISODE           = 0xB6,  //!< DID for Logiways episode_descriptor
        DID_LW_PRICE             = 0xB7,  //!< DID for Logiways price_descriptor
        DID_LW_ASSET_REFERENCE   = 0xB8,  //!< DID for Logiways asset_reference_descriptor
        DID_LW_CONTENT_CODING    = 0xB9,  //!< DID for Logiways content_coding_descriptor
        DID_LW_VOD_COMMAND       = 0xBA,  //!< DID for Logiways vod_command_descriptor
        DID_LW_DELETION_DATE     = 0xBB,  //!< DID for Logiways deletion_date_descriptor
        DID_LW_PLAY_LIST         = 0xBC,  //!< DID for Logiways play_list_descriptor
        DID_LW_PLAY_LIST_ENTRY   = 0xBD,  //!< DID for Logiways play_list_entry_descriptor
        DID_LW_ORDER_CODE        = 0xBE,  //!< DID for Logiways order_code_descriptor
        DID_LW_BOUQUET_REFERENCE = 0xBF,  //!< DID for Logiways bouquet_reference_descriptor

        // Valid in DVB context after PDS_EUTELSAT private_data_specifier

        DID_EUTELSAT_CHAN_NUM = 0x83,  //!< DID for eutelsat_channel_number_descriptor

        // Valid in DVB context after PDS_NORDIG private_data_specifier

        DID_NORDIG_CHAN_NUM_V1 = 0x83,  //!< DID for nordig_logical_channel_descriptor_v1
        DID_NORDIG_CHAN_NUM_V2 = 0x87,  //!< DID for nordig_logical_channel_descriptor_v2

        // Valid in DVB context after PDS_EACEM/EICTA private_data_specifier

        DID_EACEM_LCN              = 0x83,  //!< DID for EACEM/EICTA logical_channel_number_descriptor
        DID_EACEM_PREF_NAME_LIST   = 0x84,  //!< DID for EACEM/EICTA preferred_name_list_descriptor
        DID_EACEM_PREF_NAME_ID     = 0x85,  //!< DID for EACEM/EICTA preferred_name_identifier_descriptor
        DID_EACEM_STREAM_ID        = 0x86,  //!< DID for EACEM/EICTA eacem_stream_identifier_descriptor
        DID_EACEM_HD_SIMULCAST_LCN = 0x88,  //!< DID for EACEM/EICTA HD_simulcast_logical_channel_number_descriptor

        // Valid in DVB context after PDS_OFCOM private_data_specifier

        DID_OFCOM_LOGICAL_CHAN  = 0x83,  //!< DID for OFCOM/DTG logical_channel_descriptor
        DID_OFCOM_PREF_NAME_LST = 0x84,  //!< DID for OFCOM/DTG preferred_name_list_descriptor
        DID_OFCOM_PREF_NAME_ID  = 0x85,  //!< DID for OFCOM/DTG preferred_name_identifier_descriptor
        DID_OFCOM_SERVICE_ATTR  = 0x86,  //!< DID for OFCOM/DTG service_attribute_descriptor
        DID_OFCOM_SHORT_SRV_NAM = 0x87,  //!< DID for OFCOM/DTG short_service_name_descriptor
        DID_OFCOM_HD_SIMULCAST  = 0x88,  //!< DID for OFCOM/DTG HD_simulcast_logical_channel_descriptor
        DID_OFCOM_GUIDANCE      = 0x89,  //!< DID for OFCOM/DTG guidance_descriptor

        // Valid in DVB context after PDS_AUSTRALIA private_data_specifier

        DID_AUSTRALIA_LOGICAL_CHAN = 0x83,  //!< DID for Free TV Australia logical_channel_descriptor

        // Valid in DVB context after PDS_CANALPLUS private_data_specifier

        DID_CPLUS_DTG_STREAM_IND      = 0x80,  //!< DID for Canal+ DTG_Stream_indicator_descriptor
        DID_CPLUS_PIO_OFFSET_TIME     = 0X80,  //!< DID for Canal+ pio_offset_time_descriptor
        DID_CPLUS_LOGICAL_CHANNEL_81  = 0x81,  //!< DID for Canal+ logical_channel_descriptor
        DID_CPLUS_PRIVATE2            = 0x82,  //!< DID for Canal+ private_descriptor2
        DID_CPLUS_LOGICAL_CHANNEL     = 0x83,  //!< DID for Canal+ logical_channel_descriptor
        DID_CPLUS_PIO_CONTENT         = 0x83,  //!< DID for Canal+ pio_content_descriptor
        DID_CPLUS_PIO_LOGO            = 0x84,  //!< DID for Canal+ pio_logo_descriptor
        DID_CPLUS_ADSL_DELIVERY       = 0x85,  //!< DID for Canal+ adsl_delivery_system_descriptor
        DID_CPLUS_PIO_FEE             = 0x86,  //!< DID for Canal+ pio_fee_descriptor
        DID_CPLUS_PIO_EVENT_RANGE     = 0x88,  //!< DID for Canal+ pio_event_range_descriptor
        DID_CPLUS_PIO_COPY_MANAGEMENT = 0x8B,  //!< DID for Canal+ pio_copy_management_descriptor
        DID_CPLUS_PIO_COPY_CONTROL    = 0x8C,  //!< DID for Canal+ pio_copy_control_descriptor
        DID_CPLUS_PIO_PPV             = 0x8E,  //!< DID for Canal+ pio_ppv_descriptor
        DID_CPLUS_PIO_STB_SERVICE_ID  = 0x90,  //!< DID for Canal+ pio_stb_service_id_descriptor
        DID_CPLUS_PIO_MASKING_SERV_ID = 0x91,  //!< DID for Canal+ pio_masking_service_id_descriptor
        DID_CPLUS_PIO_STB_SERVMAP_UPD = 0x92,  //!< DID for Canal+ pio_stb_service_map_update_desc
        DID_CPLUS_NEW_SERVICE_LIST    = 0x93,  //!< DID for Canal+ new_service_list_descriptor
        DID_CPLUS_MESSAGE_NAGRA       = 0x94,  //!< DID for Canal+ message_descriptor_Nagra
        DID_CPLUS_ITEM_EVENT          = 0xA1,  //!< DID for Canal+ item_event_descriptor
        DID_CPLUS_ITEM_ZAPPING        = 0xA2,  //!< DID for Canal+ item_zapping_descriptor
        DID_CPLUS_APPLI_MESSAGE       = 0xA3,  //!< DID for Canal+ appli_message_descriptor
        DID_CPLUS_LIST                = 0xA4,  //!< DID for Canal+ list_descriptor
        DID_CPLUS_KEY_LIST            = 0xB0,  //!< DID for Canal+ key_list_descriptor
        DID_CPLUS_PICTURE_SIGNALLING  = 0xB1,  //!< DID for Canal+ picture_signalling_descriptor
        DID_CPLUS_COUNTER_BB          = 0xBB,  //!< DID for Canal+ counter_descriptor
        DID_CPLUS_DATA_COMPONENT_BD   = 0xBD,  //!< DID for Canal+ data_component_descriptor
        DID_CPLUS_SYSTEM_MGMT_BE      = 0xBE,  //!< DID for Canal+ system_management_descriptor
        DID_CPLUS_VO_LANGUAGE         = 0xC0,  //!< DID for Canal+ vo_language_descriptor
        DID_CPLUS_DATA_LIST           = 0xC1,  //!< DID for Canal+ data_list_descriptor
        DID_CPLUS_APPLI_LIST          = 0xC2,  //!< DID for Canal+ appli_list_descriptor
        DID_CPLUS_MESSAGE             = 0xC3,  //!< DID for Canal+ message_descriptor
        DID_CPLUS_FILE                = 0xC4,  //!< DID for Canal+ file_descriptor
        DID_CPLUS_RADIO_FORMAT        = 0xC5,  //!< DID for Canal+ radio_format_descriptor
        DID_CPLUS_APPLI_STARTUP       = 0xC6,  //!< DID for Canal+ appli_startup_descriptor
        DID_CPLUS_PATCH               = 0xC7,  //!< DID for Canal+ patch_descriptor
        DID_CPLUS_LOADER              = 0xC8,  //!< DID for Canal+ loader_descriptor
        DID_CPLUS_CHANNEL_MAP_UPDATE  = 0xC9,  //!< DID for Canal+ channel_map_update_descriptor
        DID_CPLUS_PPV                 = 0xCA,  //!< DID for Canal+ ppv_descriptor
        DID_CPLUS_COUNTER_CB          = 0xCB,  //!< DID for Canal+ counter_descriptor
        DID_CPLUS_OPERATOR_INFO       = 0xCC,  //!< DID for Canal+ operator_info_descriptor
        DID_CPLUS_SERVICE_DEF_PARAMS  = 0xCD,  //!< DID for Canal+ service_default_parameters_desc
        DID_CPLUS_FINGER_PRINTING     = 0xCE,  //!< DID for Canal+ finger_printing_descriptor
        DID_CPLUS_FINGER_PRINTING_V2  = 0xCF,  //!< DID for Canal+ finger_printing_descriptor_v2
        DID_CPLUS_CONCEALED_GEO_ZONES = 0xD0,  //!< DID for Canal+ concealed_geo_zones_descriptor
        DID_CPLUS_COPY_PROTECTION     = 0xD1,  //!< DID for Canal+ copy_protection_descriptor
        DID_CPLUS_MG_SUBSCRIPTION     = 0xD3,  //!< DID for Canal+ subscription_descriptor
        DID_CPLUS_CABLE_BACKCH_DELIV  = 0xD4,  //!< DID for Canal+ cable_backchannel_delivery_system
        DID_CPLUS_INTERACT_SNAPSHOT   = 0xD5,  //!< DID for Canal+ Interactivity_snapshot_descriptor
        DID_CPLUS_ICON_POSITION       = 0xDC,  //!< DID for Canal+ icon_position_descriptor
        DID_CPLUS_ICON_PIXMAP         = 0xDD,  //!< DID for Canal+ icon_pixmap_descriptor
        DID_CPLUS_ZONE_COORDINATE     = 0xDE,  //!< DID for Canal+ Zone_coordinate_descriptor
        DID_CPLUS_HD_APP_CONTROL_CODE = 0xDF,  //!< DID for Canal+ HD_application_control_code_desc
        DID_CPLUS_EVENT_REPEAT        = 0xE0,  //!< DID for Canal+ Event_Repeat_descriptor
        DID_CPLUS_PPV_V2              = 0xE1,  //!< DID for Canal+ PPV_V2_descriptor
        DID_CPLUS_HYPERLINK_REF       = 0xE2,  //!< DID for Canal+ Hyperlink_ref_descriptor
        DID_CPLUS_SHORT_SERVICE       = 0xE4,  //!< DID for Canal+ Short_service_descriptor
        DID_CPLUS_OPERATOR_TELEPHONE  = 0xE5,  //!< DID for Canal+ Operator_telephone_descriptor
        DID_CPLUS_ITEM_REFERENCE      = 0xE6,  //!< DID for Canal+ Item_reference_descriptor
        DID_CPLUS_MH_PARAMETERS       = 0xE9,  //!< DID for Canal+ MH_Parameters_descriptor
        DID_CPLUS_LOGICAL_REFERENCE   = 0xED,  //!< DID for Canal+ Logical_reference_descriptor
        DID_CPLUS_DATA_VERSION        = 0xEE,  //!< DID for Canal+ Data_Version_descriptor
        DID_CPLUS_SERVICE_GROUP       = 0xEF,  //!< DID for Canal+ Service_group_descriptor
        DID_CPLUS_STREAM_LOC_TRANSP   = 0xF0,  //!< DID for Canal+ Stream_Locator_Transport_desc
        DID_CPLUS_DATA_LOCATOR        = 0xF1,  //!< DID for Canal+ Data_Locator_descriptor
        DID_CPLUS_RESIDENT_APP        = 0xF2,  //!< DID for Canal+ resident_application_descriptor
        DID_CPLUS_RESIDENT_APP_SIGNAL = 0xF3,  //!< DID for Canal+ Resident_Application_Signalling
        DID_CPLUS_MH_LOGICAL_REF      = 0xF8,  //!< DID for Canal+ MH_Logical_Reference_descriptor
        DID_CPLUS_RECORD_CONTROL      = 0xF9,  //!< DID for Canal+ record_control_descriptor
        DID_CPLUS_CMPS_RECORD_CONTROL = 0xFA,  //!< DID for Canal+ cmps_record_control_descriptor
        DID_CPLUS_EPISODE             = 0xFB,  //!< DID for Canal+ episode_descriptor
        DID_CPLUS_CMP_SELECTION       = 0xFC,  //!< DID for Canal+ CMP_Selection_descriptor
        DID_CPLUS_DATA_COMPONENT_FD   = 0xFD,  //!< DID for Canal+ data_component_descriptor
        DID_CPLUS_SYSTEM_MGMT_FE      = 0xFE,  //!< DID for Canal+ system_management_descriptor

        // Valid in DVB context after PDS_ASTRA private_data_specifier

        DID_ASTRA_SERVICE_LIST_NAME   = 0x88,  //!< DID for SES Astra service_list_name_descriptor
        DID_ASTRA_BOUQUET_LIST        = 0x93,  //!< DID for SES Astra bouquet_list_descriptor
        DID_ASTRA_VIRTUAL_SERVICE_ID  = 0xD1,  //!< DID for SES Astra virtual_service_id_descriptor

        // Valid in DVB context after PDS_BSKYB private_data_specifier

        DID_SKY_LCN     = 0xB1,  //!< DID for BskyB logical_channel_number_by_region_descriptor
        DID_SKY_SERVICE = 0xB2,  //!< DID for BskyB service_descriptor

        // Valid in DVB context after PDS_AVSVideo private_data_specifier

        DID_AVS3_VIDEO = 0xD1,  //!< DID for AVS3 video descriptor, as defined in T/AI 109.6

        // Valid in DVB context after PDS_AVSAudio private_data_specifier

        DID_AVS3_AUDIO = 0xD2,  //!< DID for AVS3 audio descriptor, as defined in T/AI 109.7
        DID_AVS2_AUDIO = 0xD3,  //!< DID for AVS2 audio descriptor, as defined in T/AI 109.7

        // Valid in DVB context after PDS_CUVV private_data_specifier

        DID_CUVV_HDR = 0xF3,  //!< DID for UWA HDR Vivid video descriptor, as defined in T/UWA 005.2-1

        // Valid in DVB context after PDS_AOM private_data_specifier

        DID_AOM_AV1_VIDEO = 0x80,  //!< DID for AV1 video descriptor, as defined in https://aomediacodec.github.io/av1-mpeg2-ts/

        // Valid in MPEG context after REGID_VANC registration id / format identifier

        DID_SMPTE_ANC_DATA = 0xC4,  //!< DID for SMPTE anc_data_descriptor

        // Valid in ATSC / SCTE context:

        DID_ATSC_STUFFING       = 0x80,  //!< DID for ATSC stuffing_descriptor
        DID_ATSC_AC3            = 0x81,  //!< DID for ATSC ac3_audio_stream_descriptor
        DID_ATSC_PID            = 0x85,  //!< DID for ATSC program_identifier_descriptor
        DID_ATSC_CAPTION        = 0x86,  //!< DID for ATSC caption_service_descriptor
        DID_ATSC_CONTENT_ADVIS  = 0x87,  //!< DID for ATSC content_advisory_descriptor
        DID_CUE_IDENTIFIER      = 0x8A,  //!< DID for SCTE 35 cue_identifier_descriptor
        DID_ATSC_PARAM_SERVICE  = 0x8D,  //!< DID for ATSC parameterized_service_descriptor (A/71)
        DID_ATSC_EXT_CHAN_NAME  = 0xA0,  //!< DID for ATSC extended_channel_name_descriptor
        DID_ATSC_SERVICE_LOC    = 0xA1,  //!< DID for ATSC service_location_descriptor
        DID_ATSC_TIME_SHIFT     = 0xA2,  //!< DID for ATSC time_shifted_event_descriptor
        DID_ATSC_COMPONENT_NAME = 0xA3,  //!< DID for ATSC component_name_descriptor
        DID_ATSC_DATA_SERVICE   = 0xA4,  //!< DID for ATSC data_service_descriptor
        DID_ATSC_PID_COUNT      = 0xA5,  //!< DID for ATSC pid_count_descriptor
        DID_ATSC_DOWNLOAD       = 0xA6,  //!< DID for ATSC download_descriptor
        DID_ATSC_MPROTO_ENCAPS  = 0xA7,  //!< DID for ATSC multiprotocol_encapsulation_descriptor
        DID_ATSC_DCC_DEPARTING  = 0xA8,  //!< DID for ATSC DCC_departing_request_descriptor
        DID_ATSC_DCC_ARRIVING   = 0xA9,  //!< DID for ATSC DCC_arriving_request_descriptor
        DID_ATSC_REDIST_CONTROL = 0xAA,  //!< DID for ATSC redistribution_control_descriptor
        DID_ATSC_GENRE          = 0xAB,  //!< DID for ATSC genre_descriptor
        DID_ATSC_PRIVATE_INFO   = 0xAD,  //!< DID for ATSC private_information_descriptor
        DID_ATSC_MODULE_LINK    = 0xB4,  //!< DID for ATSC module_link_descriptor
        DID_ATSC_CRC32          = 0xB5,  //!< DID for ATSC CRC32_descriptor
        DID_ATSC_GROUP_LINK     = 0xB8,  //!< DID for ATSC group_link_descriptor
        DID_ATSC_COMPONENT_LIST = 0xBB,  //!< DID for ATSC component_list_descriptor (A/71)
        DID_ATSC_ENHANCED_AC3   = 0xCC,  //!< DID for ATSC E-AC-3_audio_stream_descriptor

        // Valid in SCTE EAS (Emergency Alert System, SCTE 18).

        DID_EAS_INBAND_DETAILS = 0x00,  //!< DID for SCTE 18 In-Band Details Channel Descriptor
        DID_EAS_INBAND_EXCEPTS = 0x01,  //!< DID for SCTE 18 In-Band Exceptions Channel Descriptor
        DID_EAS_AUDIO_FILE     = 0x02,  //!< DID for SCTE 18 Audio File Descriptor
        DID_EAS_METADATA       = 0x03,  //!< DID for SCTE 18 / SCTE 164 Emergency Alert Metadata Descriptor

        // Valid only in a SCTE SIT (Splice Information Table, SCTE 35).

        DID_SPLICE_AVAIL   = 0x00,  //!< DID for SCTE 35 SIT avail_descriptor
        DID_SPLICE_DTMF    = 0x01,  //!< DID for SCTE 35 SIT DTMF_descriptor
        DID_SPLICE_SEGMENT = 0x02,  //!< DID for SCTE 35 SIT segmentation_descriptor
        DID_SPLICE_TIME    = 0x03,  //!< DID for SCTE 35 SIT time_descriptor
        DID_SPLICE_AUDIO   = 0x04,  //!< DID for SCTE 35 SIT audio_descriptor

        // Valid in ISDB context:

        DID_ISDB_MATERIAL_INFO  = 0x67,  //!< DID for ISDB Material information descriptor, in LIT only (WARNING: conflict with DVB)
        DID_ISDB_HYBRID_INFO    = 0x68,  //!< DID for ISDB Hybrid information descriptor (WARNING: conflict with DVB)
        DID_ISDB_HIERARCH_TRANS = 0xC0,  //!< DID for ISDB Hierarchical transmission descriptor
        DID_ISDB_COPY_CONTROL   = 0xC1,  //!< DID for ISDB Digital copy control descriptor
        DID_ISDB_NETWORK_ID     = 0xC2,  //!< DID for ISDB Network identifier descriptor
        DID_ISDB_PART_TS_TIME   = 0xC3,  //!< DID for ISDB Partial Transport Stream time descriptor
        DID_ISDB_AUDIO_COMP     = 0xC4,  //!< DID for ISDB Audio component descriptor
        DID_ISDB_HYPERLINK      = 0xC5,  //!< DID for ISDB Hyperlink descriptor
        DID_ISDB_TARGET_REGION  = 0xC6,  //!< DID for ISDB Target region descriptor
        DID_ISDB_DATA_CONTENT   = 0xC7,  //!< DID for ISDB Data content descriptor
        DID_ISDB_VIDEO_CONTROL  = 0xC8,  //!< DID for ISDB Video decode control descriptor
        DID_ISDB_DOWNLOAD_CONT  = 0xC9,  //!< DID for ISDB Download content descriptor
        DID_ISDB_CA_EMM_TS      = 0xCA,  //!< DID for ISDB CA_EMM_TS descriptor
        DID_ISDB_CA_CONTRACT    = 0xCB,  //!< DID for ISDB CA contract information descriptor
        DID_ISDB_CA_SERVICE     = 0xCC,  //!< DID for ISDB CA service descriptor
        DID_ISDB_TS_INFO        = 0xCD,  //!< DID for ISDB TS information descriptor
        DID_ISDB_EXT_BROADCAST  = 0xCE,  //!< DID for ISDB Extended broadcaster descriptor
        DID_ISDB_LOGO_TRANSM    = 0xCF,  //!< DID for ISDB Logo transmission descriptor
        DID_ISDB_BASIC_LOCAL_EV = 0xD0,  //!< DID for ISDB Basic local event descriptor
        DID_ISDB_REFERENCE      = 0xD1,  //!< DID for ISDB Reference descriptor
        DID_ISDB_NODE_RELATION  = 0xD2,  //!< DID for ISDB Node relation descriptor
        DID_ISDB_SHORT_NODE_INF = 0xD3,  //!< DID for ISDB Short node information descriptor
        DID_ISDB_STC_REF        = 0xD4,  //!< DID for ISDB STC reference descriptor
        DID_ISDB_SERIES         = 0xD5,  //!< DID for ISDB Series descriptor
        DID_ISDB_EVENT_GROUP    = 0xD6,  //!< DID for ISDB Event group descriptor
        DID_ISDB_SI_PARAMETER   = 0xD7,  //!< DID for ISDB SI parameter descriptor
        DID_ISDB_BROADCAST_NAME = 0xD8,  //!< DID for ISDB Broadcaster name descriptor
        DID_ISDB_COMP_GROUP     = 0xD9,  //!< DID for ISDB Component group descriptor
        DID_ISDB_SI_PRIME_TS    = 0xDA,  //!< DID for ISDB SI prime TS descriptor
        DID_ISDB_BOARD_INFO     = 0xDB,  //!< DID for ISDB Board information descriptor
        DID_ISDB_LDT_LINKAGE    = 0xDC,  //!< DID for ISDB LDT linkage descriptor
        DID_ISDB_CONNECT_TRANSM = 0xDD,  //!< DID for ISDB Connected transmission descriptor
        DID_ISDB_CONTENT_AVAIL  = 0xDE,  //!< DID for ISDB Content availability descriptor
        DID_ISDB_EXTENSION      = 0xDF,  //!< DID for ISDB extension descriptor
        DID_ISDB_SERVICE_GROUP  = 0xE0,  //!< DID for ISDB Service group descriptor
        DID_ISDB_AREA_BCAST_INF = 0xE1,  //!< DID for ISDB Area broadcast information descriptor
        DID_ISDB_NETW_DOWNLOAD  = 0xE2,  //!< DID for ISDB Network download content descriptor
        DID_ISDB_DOWNLOAD_PROT  = 0xE3,  //!< DID for ISDB Download protection descriptor
        DID_ISDB_CA_STARTUP     = 0xE4,  //!< DID for ISDB CA startup descriptor
        DID_ISDB_CHAR_CODE      = 0xE5,  //!< DID for ISDB character code descriptor
        DID_ISDB_WMCTDS         = 0xF3,  //!< DID for ISDB Wired multi-carrier transmission distribution system descriptor
        DID_ISDB_ADV_WDS        = 0xF4,  //!< DID for ISDB Advanced cable delivery system descriptor
        DID_ISDB_SCRAMBLER      = 0xF5,  //!< DID for ISDB Scrambler descriptor
        DID_ISDB_CA             = 0xF6,  //!< DID for ISDB Access control descriptor
        DID_ISDB_CAROUSEL_COMP  = 0xF7,  //!< DID for ISDB Carousel compatible composite descriptor
        DID_ISDB_COND_PLAYBACK  = 0xF8,  //!< DID for ISDB Conditional playback descriptor
        DID_ISDB_CABLE_TS_DIV   = 0xF9,  //!< DID for ISDB Cable TS division system descriptor
        DID_ISDB_TERRES_DELIV   = 0xFA,  //!< DID for ISDB Terrestrial delivery system descriptor
        DID_ISDB_PARTIAL_RECP   = 0xFB,  //!< DID for ISDB Partial reception descriptor
        DID_ISDB_EMERGENCY_INFO = 0xFC,  //!< DID for ISDB Emergency information descriptor
        DID_ISDB_DATA_COMP      = 0xFD,  //!< DID for ISDB Data component descriptor
        DID_ISDB_SYSTEM_MGMT    = 0xFE,  //!< DID for ISDB System management descriptor
    };

    //!
    //! Extension descriptor tag values (MPEG or DVB extension_descriptor)
    //!
    enum : DID {

        XDID_NULL                    = 0xFF,  //!< Invalid EDID value, can be used as placeholder.

        // In MPEG-defined extension_descriptor:

        XDID_MPEG_OBJ_DESC_UPD       = 0x02,  //!< Ext.DID for ObjectDescriptorUpdate.
        XDID_MPEG_HEVC_TIM_HRD       = 0x03,  //!< Ext.DID for HEVC_timing_and_HRD_descriptor.
        XDID_MPEG_AF_EXT             = 0x04,  //!< Ext.DID for AF_extensions_descriptor
        XDID_MPEG_HEVC_OP_POINT      = 0x05,  //!< Ext.DID for HEVC_operation_point_descriptor
        XDID_MPEG_HEVC_HIER_EXT      = 0x06,  //!< Ext.DID for HEVC_hierarchy_extension_descriptor
        XDID_MPEG_GREEN_EXT          = 0x07,  //!< Ext.DID for green_extension_descriptor
        XDID_MPEG_MPH3D_AUDIO        = 0x08,  //!< Ext.DID for MPEGH_3D_audio_descriptor
        XDID_MPEG_MPH3D_CONFIG       = 0x09,  //!< Ext.DID for MPEGH_3D_audio_config_descriptor
        XDID_MPEG_MPH3D_SCENE        = 0x0A,  //!< Ext.DID for MPEGH_3D_audio_scene_descriptor
        XDID_MPEG_MPH3D_TEXT         = 0x0B,  //!< Ext.DID for MPEGH_3D_audio_text_label_descriptor
        XDID_MPEG_MPH3D_MULTI        = 0x0C,  //!< Ext.DID for MPEGH_3D_audio_multi_stream_descriptor
        XDID_MPEG_MPH3D_DRCLOUD      = 0x0D,  //!< Ext.DID for MPEGH_3D_audio_DRC_loudness_descriptor
        XDID_MPEG_MPH3D_COMMAND      = 0x0E,  //!< Ext.DID for MPEGH_3D_audio_command_descriptor
        XDID_MPEG_QUALITY_EXT        = 0x0F,  //!< Ext.DID for quality_extension_descriptor
        XDID_MPEG_VIRT_SEGMENT       = 0x10,  //!< Ext.DID for virtual_segmentation_descriptor
        XDID_MPEG_TIMED_METADATA_EXT = 0x11,  //!< Ext.DID for timed_metadata_extension_descriptor
        XDID_MPEG_HEVC_TILE_SSTRM    = 0x12,  //!< Ext.DID for HEVC_tile_substream_descriptor
        XDID_MPEG_HEVC_SUBREGION     = 0x13,  //!< Ext.DID for HEVC_subregion_descriptor
        XDID_MPEG_JXS_VIDEO          = 0x14,  //!< Ext.DID for JXS_video_descriptor
        XDID_MPEG_VVC_TIM_HRD        = 0x15,  //!< Ext.DID for VVC_timing_and_HRD_descriptor.
        XDID_MPEG_EVC_TIM_HRD        = 0x16,  //!< Ext.DID for EVC_timing_and_HRD_descriptor.
        XDID_MPEG_LCEVC_VIDEO        = 0x17,  //!< Ext.DID for LCEVC_video_descriptor.
        XDID_MPEG_LCEVC_LINKAGE      = 0x18,  //!< Ext.DID for LCEVC_linkage_descriptor.
        XDID_MPEG_MEDIA_SVC_KIND     = 0x19,  //!< Ext.DID for Media_service_kind_descriptor

        // In DVB-defined extension_descriptor:

        XDID_DVB_IMAGE_ICON          = 0x00,  //!< Ext.DID for image_icon_descriptor
        XDID_DVB_CPCM_DELIVERY_SIG   = 0x01,  //!< Ext.DID for cpcm_delivery_signalling_descriptor
        XDID_DVB_CP                  = 0x02,  //!< Ext.DID for CP_descriptor
        XDID_DVB_CP_IDENTIFIER       = 0x03,  //!< Ext.DID for CP_identifier_descriptor
        XDID_DVB_T2_DELIVERY         = 0x04,  //!< Ext.DID for T2_delivery_system_descriptor
        XDID_DVB_SH_DELIVERY         = 0x05,  //!< Ext.DID for SH_delivery_system_descriptor
        XDID_DVB_SUPPL_AUDIO         = 0x06,  //!< Ext.DID for supplementary_audio_descriptor
        XDID_DVB_NETW_CHANGE_NOTIFY  = 0x07,  //!< Ext.DID for network_change_notify_descriptor
        XDID_DVB_MESSAGE             = 0x08,  //!< Ext.DID for message_descriptor
        XDID_DVB_TARGET_REGION       = 0x09,  //!< Ext.DID for target_region_descriptor
        XDID_DVB_TARGET_REGION_NAME  = 0x0A,  //!< Ext.DID for target_region_name_descriptor
        XDID_DVB_SERVICE_RELOCATED   = 0x0B,  //!< Ext.DID for service_relocated_descriptor
        XDID_DVB_XAIT_PID            = 0x0C,  //!< Ext.DID for XAIT_PID_descriptor
        XDID_DVB_C2_DELIVERY         = 0x0D,  //!< Ext.DID for C2_delivery_system_descriptor
        XDID_DVB_DTS_HD_AUDIO        = 0x0E,  //!< Ext.DID for DTS_HD_audio_stream_descriptor
        XDID_DVB_DTS_NEURAL          = 0x0F,  //!< Ext.DID for DTS_Neural_descriptor
        XDID_DVB_VIDEO_DEPTH_RANGE   = 0x10,  //!< Ext.DID for video_depth_range_descriptor
        XDID_DVB_T2MI                = 0x11,  //!< Ext.DID for T2MI_descriptor
        XDID_DVB_URI_LINKAGE         = 0x13,  //!< Ext.DID for URI_linkage_descriptor
        XDID_DVB_CI_ANCILLARY_DATA   = 0x14,  //!< Ext.DID for CI_ancillary_data_descriptor
        XDID_DVB_AC4                 = 0x15,  //!< Ext.DID for AC4_descriptor
        XDID_DVB_C2_BUNDLE_DELIVERY  = 0x16,  //!< Ext.DID for C2_bundle_system_delivery_descriptor
        XDID_DVB_S2X_DELIVERY        = 0x17,  //!< Ext.DID for S2X_satellite_delivery_system_descriptor
        XDID_DVB_PROTECTION_MSG      = 0x18,  //!< Ext.DID for protection_message_descriptor
        XDID_DVB_AUDIO_PRESELECT     = 0x19,  //!< Ext.DID for audio_preselection_descriptor
        XDID_DVB_TTML_SUBTITLING     = 0x20,  //!< Ext.DID for TTML_subtitling_descriptor
        XDID_DVB_DTS_UHD             = 0x21,  //!< Ext.DID for DTS-UHD_descriptor
        XDID_DVB_SERVICE_PROMINENCE  = 0x22,  //!< Ext.DID for service_prominence_descriptor
        XDID_DVB_VVC_SUBPICTURES     = 0x23,  //!< Ext.DID for vvc_subpictures_descriptor
        XDID_DVB_S2XV2_DELIVERY      = 0x24,  //!< Ext.DID for S2Xv2_satellite_delivery_system_descriptor
    };

    //!
    //! Name of a Descriptor ID.
    //! @param [in] did Descriptor id.
    //! @param [in] flags Presentation flags.
    //! @param [in,out] context Interpretation context of the descriptor.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString DIDName(DID did, DescriptorContext& context, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of an MPEG extension descriptor ID.
    //! @param [in] xdid MPEG extension descriptor ID.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString XDIDNameMPEG(DID xdid, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of a DVB extension descriptor ID.
    //! @param [in] xdid DVB extension descriptor ID.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString XDIDNameDVB(DID xdid, NamesFlags flags = NamesFlags::NAME);
}
