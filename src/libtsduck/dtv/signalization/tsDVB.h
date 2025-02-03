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
//!  Generic DVB definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"

namespace ts {

    class DuckContext;

    //!
    //! A placeholder for "invalid network id" value.
    //! In theory, all 16-bit values can be valid network id. However, this one is "usually" not used.
    //!
    constexpr uint16_t INVALID_NETWORK_ID = 0xFFFF;

    //!
    //! Name of Original Network Id.
    //! @param [in] id Original Network Id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString OriginalNetworkIdName(uint16_t id, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of Network Id.
    //! @param [in] id Network Id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString NetworkIdName(uint16_t id, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of Bouquet Id.
    //! @param [in] id Bouquet Id.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString BouquetIdName(uint16_t id, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of service type (in Service Descriptor).
    //! @param [in] st Service type (in Service Descriptor).
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString ServiceTypeName(uint8_t st, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Name of content name (in Content Descriptor).
    //! @param [in] duck TSDuck execution context (used to select from other standards).
    //! @param [in] c Content name.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString ContentIdName(const DuckContext& duck, uint8_t c, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Running status values (in RST, EIT, etc.)
    //!
    enum : uint8_t {
        RS_UNDEFINED   = 0x00, //!< Undefined
        RS_NOT_RUNNING = 0x01, //!< Not running
        RS_STARTING    = 0x02, //!< Starts in a few seconds (e.g. for video recording)
        RS_PAUSING     = 0x03, //!< Pausing
        RS_RUNNING     = 0x04, //!< Running
        RS_OFF_AIR     = 0x05, //!< Service off-air
    };

    //!
    //! Enumeration description of running status values.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Names& RunningStatusEnum();

    //!
    //! Name of Running Status (in SDT).
    //! @param [in] rs Running Status (in SDT).
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString RunningStatusName(uint8_t rs, NamesFlags flags = NamesFlags::NAME);

    //!
    //! Scrambling mode values (in scrambling_descriptor)
    //!
    enum : uint8_t {
        SCRAMBLING_DVB_CSA1      = 0x01, //!< DVB-CSA1
        SCRAMBLING_DVB_CSA2      = 0x02, //!< DVB-CSA2
        SCRAMBLING_DVB_CSA3      = 0x03, //!< DVB-CSA3
        SCRAMBLING_DVB_CSA3_MIN  = 0x04, //!< DVB-CSA3, minimally enhanced mode (obsolete)
        SCRAMBLING_DVB_CSA3_FULL = 0x05, //!< DVB-CSA3, fully enhanced mode (obsolete)
        SCRAMBLING_DVB_CISSA1    = 0x10, //!< DVB-CISSA v1
        SCRAMBLING_ATIS_IIF_IDSA = 0x70, //!< ATIS IIF IDSA for MPEG-2 TS
        SCRAMBLING_USER_MIN      = 0x80, //!< First user-defined value.
        SCRAMBLING_DUCK_AES_CBC  = 0xF0, //!< TSDuck-defined value, AES-128-CBC (with externally-defined IV).
        SCRAMBLING_DUCK_AES_CTR  = 0xF1, //!< TSDuck-defined value, AES-128-CTR (with externally-defined IV).
        SCRAMBLING_USER_MAX      = 0xFE, //!< Last user-defined value.
        SCRAMBLING_RESERVED      = 0xFF, //!< Reserved value.
    };

    //!
    //! Data broadcast id values (in data_broadcast[_id]_descriptor)
    //!
    enum : uint16_t {
        DBID_DATA_PIPE            = 0x0001, //!< Data pipe
        DBID_ASYNC_DATA_STREAM    = 0x0002, //!< Asynchronous data stream
        DBID_SYNC_DATA_STREAM     = 0x0003, //!< Synchronous data stream
        DBID_SYNCED_DATA_STREAM   = 0x0004, //!< Synchronised data stream
        DBID_MPE                  = 0x0005, //!< Multi protocol encapsulation
        DBID_DATA_CSL             = 0x0006, //!< Data Carousel
        DBID_OBJECT_CSL           = 0x0007, //!< Object Carousel
        DBID_ATM                  = 0x0008, //!< DVB ATM streams
        DBID_HP_ASYNC_DATA_STREAM = 0x0009, //!< Higher Protocols based on asynchronous data streams
        DBID_SSU                  = 0x000A, //!< System Software Update service [TS 102 006]
        DBID_IPMAC_NOTIFICATION   = 0x000B, //!< IP/MAC Notification service [EN 301 192]
        DBID_MHP_OBJECT_CSL       = 0x00F0, //!< MHP Object Carousel
        DBID_MHP_MPE              = 0x00F1, //!< Reserved for MHP Multi Protocol Encapsulation
        DBID_EUTELSAT_DATA_PIPE   = 0x0100, //!< Eutelsat Data Piping
        DBID_EUTELSAT_DATA_STREAM = 0x0101, //!< Eutelsat Data Streaming
        DBID_SAGEM_IP             = 0x0102, //!< SAGEM IP encapsulation in MPEG-2 PES packets
        DBID_BARCO_DATA_BRD       = 0x0103, //!< BARCO Data Broadcasting
        DBID_CIBERCITY_MPE        = 0x0104, //!< CyberCity Multiprotocol Encapsulation
        DBID_CYBERSAT_MPE         = 0x0105, //!< CyberSat Multiprotocol Encapsulation
        DBID_TDN                  = 0x0106, //!< The Digital Network
        DBID_OPENTV_DATA_CSL      = 0x0107, //!< OpenTV Data Carousel
        DBID_PANASONIC            = 0x0108, //!< Panasonic
        DBID_KABEL_DEUTSCHLAND    = 0x0109, //!< Kabel Deutschland
        DBID_TECHNOTREND          = 0x010A, //!< TechnoTrend Gorler GmbH
        DBID_MEDIAHIGHWAY_SSU     = 0x010B, //!< NDS France Technologies system software download
        DBID_GUIDE_PLUS           = 0x010C, //!< GUIDE Plus+ Rovi Corporation
        DBID_ACAP_OBJECT_CSL      = 0x010D, //!< ACAP Object Carousel
        DBID_MICRONAS             = 0x010E, //!< Micronas Download Stream
        DBID_POLSAT               = 0x0110, //!< Televizja Polsat
        DBID_DTG                  = 0x0111, //!< UK DTG
        DBID_SKYMEDIA             = 0x0112, //!< SkyMedia
        DBID_INTELLIBYTE          = 0x0113, //!< Intellibyte DataBroadcasting
        DBID_TELEWEB_DATA_CSL     = 0x0114, //!< TeleWeb Data Carousel
        DBID_TELEWEB_OBJECT_CSL   = 0x0115, //!< TeleWeb Object Carousel
        DBID_TELEWEB              = 0x0116, //!< TeleWeb
        DBID_BBC                  = 0x0117, //!< BBC
        DBID_ELECTRA              = 0x0118, //!< Electra Entertainment Ltd
        DBID_BBC_2_3              = 0x011A, //!< BBC 2 - 3
        DBID_TELETEXT             = 0x011B, //!< Teletext
        DBID_SKY_DOWNLOAD_1_5     = 0x0120, //!< Sky Download Streams 1-5
        DBID_ICO                  = 0x0121, //!< ICO mim
        DBID_CIPLUS_DATA_CSL      = 0x0122, //!< CI+ Data Carousel
        DBID_HBBTV                = 0x0123, //!< HBBTV Carousel
        DBID_ROVI_PREMIUM         = 0x0124, //!< Premium Content from Rovi Corporation
        DBID_MEDIA_GUIDE          = 0x0125, //!< Media Guide from Rovi Corporation
        DBID_INVIEW               = 0x0126, //!< InView Technology Ltd
        DBID_BOTECH               = 0x0130, //!< Botech Elektronik SAN. ve TIC. LTD.STI.
        DBID_SCILLA_PUSHVOD_CSL   = 0x0131, //!< Scilla Push-VOD Carousel
        DBID_CANAL_PLUS           = 0x0140, //!< Canal+
        DBID_OIPF_OBJECT_CSL      = 0x0150, //!< OIPF Object Carousel - Open IPTV Forum
        DBID_4TV                  = 0x4444, //!< 4TV Data Broadcast
        DBID_NOKIA_IP_SSU         = 0x4E4F, //!< Nokia IP based software delivery
        DBID_BBG_DATA_CSL         = 0xBBB1, //!< BBG Data Caroussel
        DBID_BBG_OBJECT_CSL       = 0xBBB2, //!< BBG Object Caroussel
        DBID_BBG                  = 0xBBBB, //!< Bertelsmann Broadband Group
    };

    //!
    //! Name of Data broadcast id (in Data Broadcast Id Descriptor).
    //! @param [in] id Data broadcast id (in Data Broadcast Id Descriptor).
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString DataBroadcastIdName(uint16_t id, NamesFlags flags = NamesFlags::NAME);

    //!
    //! DVB-MHP transport protocol ids.
    //!
    enum : uint16_t {
        MHP_PROTO_CAROUSEL = 0x0001,  //!< MHP Object Carousel
        MHP_PROTO_MPE      = 0x0002,  //!< IP via DVB-MPE
        MHP_PROTO_HTTP     = 0x0003,  //!< HTTP over interaction channel
    };
}
