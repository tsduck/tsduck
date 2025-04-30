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
//!  MPEG PSI/SI table identifiers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCAS.h"
#include "tsTS.h"
#include "tsNames.h"

namespace ts {

    class DuckContext;

    //!
    //! Table identifier.
    //!
    using TID = uint8_t;

    //!
    //! Maximum number of TID values.
    //!
    constexpr size_t TID_MAX = 0x100;

    //!
    //! Table identification (TID) values
    //!
    enum : TID {

        // Valid in all MPEG contexts:

        TID_PAT           = 0x00, //!< Table id for Program Association Table PAT
        TID_CAT           = 0x01, //!< Table id for Conditional Access Table
        TID_PMT           = 0x02, //!< Table id for Program Map Table
        TID_TSDT          = 0x03, //!< Table id for Transport Stream Description Table
        TID_MP4SDT        = 0x04, //!< Table id for MPEG-4 Scene Description Table
        TID_MP4ODT        = 0x05, //!< Table id for MPEG-4 Object Descriptor Table
        TID_MDT           = 0x06, //!< Table id for MetaData Table
        TID_IPMP_CT       = 0x07, //!< Table id for IPMP Control Information Table (ISO/IEC 13818-11)
        TID_ISO_14496     = 0x08, //!< Table id for ISO/IEC-14496 Table
        TID_ISO_23001_11  = 0x09, //!< Table id for ISO/IEC 23001-11 Green Access Unit Table
        TID_ISO_23001_10  = 0x0A, //!< Table id for ISO/IEC 23001-10 Quality Access Unit Table
        TID_DSMCC_MPE     = 0x3A, //!< Table id for DSM-CC Multi-Protocol Encapsulated data
        TID_DSMCC_UNM     = 0x3B, //!< Table id for DSM-CC User-to-Network Messages
        TID_DSMCC_DDM     = 0x3C, //!< Table id for DSM-CC Download Data Messages
        TID_DSMCC_SD      = 0x3D, //!< Table id for DSM-CC Stream Descriptors
        TID_DSMCC_PD      = 0x3E, //!< Table id for DSM-CC Private Data
        TID_MPEG_LAST     = 0x3F, //!< Last MPEG-defined table id.
        TID_NULL          = 0xFF, //!< Reserved table id value, end of TS packet PSI payload

        // Valid in DVB context:

        TID_NIT_ACT       = 0x40, //!< Table id for Network Information Table - Actual network
        TID_NIT_OTH       = 0x41, //!< Table id for Network Information Table - Other network
        TID_SDT_ACT       = 0x42, //!< Table id for Service Description Table - Actual TS
        TID_SDT_OTH       = 0x46, //!< Table id for Service Description Table - Other TS
        TID_BAT           = 0x4A, //!< Table id for Bouquet Association Table
        TID_UNT           = 0x4B, //!< Table id for Update Notification Table (SSU, ETSI TS 102 006)
        TID_INT           = 0x4C, //!< Table id for IP/MAC Notification Table (MPE, ETSI EN 301 192)
        TID_SAT           = 0x4D, //!< Table id for Satellite Access Table
        TID_EIT_PF_ACT    = 0x4E, //!< Table id for EIT present/following - Actual network
        TID_EIT_PF_OTH    = 0x4F, //!< Table id for EIT present/following - Other network
        TID_EIT_S_ACT_MIN = 0x50, //!< Table id for EIT schedule - Actual network
        TID_EIT_S_ACT_MAX = 0x5F, //!< Table id for EIT schedule - Actual network
        TID_EIT_S_OTH_MIN = 0x60, //!< Table id for EIT schedule - Other network
        TID_EIT_S_OTH_MAX = 0x6F, //!< Table id for EIT schedule - Other network
        TID_TDT           = 0x70, //!< Table id for Time & Date Table
        TID_RST           = 0x71, //!< Table id for Running Status Table
        TID_ST            = 0x72, //!< Table id for Stuffing Table
        TID_TOT           = 0x73, //!< Table id for Time Offset Table
        TID_AIT           = 0x74, //!< Table id for Application Information Table (HbbTV, ETSI TS 102 809)
        TID_CT            = 0x75, //!< Table id for Container Table (TV-Anytime)
        TID_RCT           = 0x76, //!< Table id for Related Content Table (TV-Anytime)
        TID_CIT           = 0x77, //!< Table id for Content Identifier Table (TV-Anytime)
        TID_MPE_FEC       = 0x78, //!< Table id for MPE-FEC Table (Data Broadcasting)
        TID_RNT           = 0x79, //!< Table id for Resolution Notification Table (TV-Anytime)
        TID_MPE_IFEC      = 0x7A, //!< Table id for MPE-IFEC Table
        TID_PROT_MSG      = 0x7B, //!< Table id for Protection Message Table
        TID_DFIT          = 0x7C, //!< Table id for Downloadable Font Info Table
        TID_DIT           = 0x7E, //!< Table id for Discontinuity Information Table
        TID_SIT           = 0x7F, //!< Table id for Selection Information Table

        TID_ECM_80        = 0x80, //!< Table id for ECM 80
        TID_ECM_81        = 0x81, //!< Table id for ECM 81
        TID_EMM_FIRST     = 0x82, //!< Start of Table id range for EMM's
        TID_EMM_82        = 0x82, //!< Table id for EMM 82
        TID_EMM_83        = 0x83, //!< Table id for EMM 83
        TID_EMM_84        = 0x84, //!< Table id for EMM 84
        TID_EMM_85        = 0x85, //!< Table id for EMM 85
        TID_EMM_86        = 0x86, //!< Table id for EMM 86
        TID_EMM_87        = 0x87, //!< Table id for EMM 87
        TID_EMM_88        = 0x88, //!< Table id for EMM 88
        TID_EMM_89        = 0x89, //!< Table id for EMM 89
        TID_EMM_8A        = 0x8A, //!< Table id for EMM 8A
        TID_EMM_8B        = 0x8B, //!< Table id for EMM 8B
        TID_EMM_8C        = 0x8C, //!< Table id for EMM 8C
        TID_EMM_8D        = 0x8D, //!< Table id for EMM 8D
        TID_EMM_8E        = 0x8E, //!< Table id for EMM 8E
        TID_EMM_8F        = 0x8F, //!< Table id for EMM 8F
        TID_EMM_LAST      = 0x8F, //!< End of Table id range for EMM's

        // Ranges by type

        TID_EIT_MIN       = 0x4E, //!< Table id for EIT, first TID
        TID_EIT_MAX       = 0x6F, //!< Table id for EIT, last TID
        TID_CAS_FIRST     = 0x80, //!< Start of Table id range for CAS
        TID_CAS_LAST      = 0x8F, //!< End of Table id range for CAS

        // Valid in SES Astra context:

        TID_ASTRA_SGT     = 0x91, //!< Table id for SES Astra Service Guide Table

        // Valid in SafeAccess CAS context:

        TID_SA_CECM_82    = 0x82, //!< Table id for SafeAccess Complementary ECM
        TID_SA_CECM_83    = 0x83, //!< Table id for SafeAccess Complementary ECM
        TID_SA_EMM_STB_U  = 0x84, //!< Table id for SafeAccess STB or CI-CAM unique EMM
        TID_SA_EMM_STB_G  = 0x85, //!< Table id for SafeAccess STB global EMM
        TID_SA_EMM_A      = 0x86, //!< Table id for SafeAccess Global EMM ("all")
        TID_SA_EMM_U      = 0x87, //!< Table id for SafeAccess Unique EMM
        TID_SA_EMM_S      = 0x88, //!< Table id for SafeAccess Group EMM ("shared")
        TID_SA_EMM_CAM_G  = 0x89, //!< Table id for SafeAccess CI-CAM global EMM
        TID_SA_RECM_8A    = 0x8A, //!< Table id for SafeAccess Record ECM
        TID_SA_RECM_8B    = 0x8B, //!< Table id for SafeAccess Record ECM
        TID_SA_EMM_T      = 0x8F, //!< Table id for SafeAccess Technical EMM

        // Valid in Logiways context:

        TID_LW_DMT        = 0x90, //!< Table id for Logiways Download Marker Table
        TID_LW_BDT        = 0x91, //!< Table id for Logiways Binary Data Table
        TID_LW_VIT        = 0x92, //!< Table id for Logiways VoD Information Table
        TID_LW_VCT        = 0x93, //!< Table id for Logiways VoD Command Table

        // Valid in Viaccess CAS context:

        TID_VIA_EMM_FT_E  = 0x86, //!< Table id for Viaccess EMM-FT (even)
        TID_VIA_EMM_FT_O  = 0x87, //!< Table id for Viaccess EMM-FT (odd)
        TID_VIA_EMM_U     = 0x88, //!< Table id for Viaccess EMM-U and EMM-D-U
        TID_VIA_EMM_GA_E  = 0x8A, //!< Table id for Viaccess EMM-GA and EMM-D-GA (even)
        TID_VIA_EMM_GA_O  = 0x8B, //!< Table id for Viaccess EMM-GA and EMM-D-GA (odd)
        TID_VIA_EMM_GH_E  = 0x8C, //!< Table id for Viaccess EMM-GH (even)
        TID_VIA_EMM_GH_O  = 0x8D, //!< Table id for Viaccess EMM-GH (odd)
        TID_VIA_EMM_S     = 0x8E, //!< Table id for Viaccess EMM-S

        // Valid in MediaGuard CAS context:

        TID_MG_EMM_U      = 0x82, //!< Table id for MediaGuard EMM-U
        TID_MG_EMM_A      = 0x83, //!< Table id for MediaGuard EMM-A
        TID_MG_EMM_G      = 0x84, //!< Table id for MediaGuard EMM-G
        TID_MG_EMM_I      = 0x85, //!< Table id for MediaGuard EMM-I
        TID_MG_EMM_C      = 0x86, //!< Table id for MediaGuard EMM-C
        TID_MG_EMM_CG     = 0x89, //!< Table id for MediaGuard EMM-CG

        // Valid in ATSC context:

        TID_MGT           = 0xC7, //!< Table id for Master Guide Table (ATSC)
        TID_TVCT          = 0xC8, //!< Table id for Terrestrial Virtual Channel Table (ATSC)
        TID_CVCT          = 0xC9, //!< Table id for Cable Virtual Channel Table (ATSC)
        TID_RRT           = 0xCA, //!< Table id for Rating Region Table (ATSC)
        TID_ATSC_EIT      = 0xCB, //!< Table id for Event Information Table (ATSC version)
        TID_ETT           = 0xCC, //!< Table id for Extended Text Table (ATSC)
        TID_STT           = 0xCD, //!< Table id for System Time Table (ATSC)
        TID_DET           = 0xCE, //!< Table id for Data Event Table (ATSC A/90)
        TID_DST           = 0xCF, //!< Table id for Data Service Table (ATSC A/90)
        TID_NRT           = 0xD1, //!< Table id for Network Resources Table (ATSC A/90)
        TID_LTST          = 0xD2, //!< Table id for Long Term Service Table (ATSC A/90)
        TID_DCCT          = 0xD3, //!< Table id for Directed Channel Change Table (ATSC)
        TID_DCCSCT        = 0xD4, //!< Table id for Directed Channel Change Selection Code Table (ATSC)
        TID_AEIT          = 0xD6, //!< Table id for Aggregate Event Information Table (ATSC A/81)
        TID_AETT          = 0xD7, //!< Table id for Aggregate Extended Text Table (ATSC A/81)
        TID_SVCT          = 0xDA, //!< Table id for Satellite Virtual Channel Table (ATSC A/81)

        // Valid in SCTE context:

        TID_SCTE18_EAS    = 0xD8, //!< Table id for SCTE 18 Emergency Alert System
        TID_SCTE35_SIT    = 0xFC, //!< Table id for SCTE 35 Splice Information Table

        // Valid in ISDB context:

        TID_DCT           = 0xC0, //!< Table id for Download Control Table (ISDB)
        TID_DLT           = 0xC1, //!< Table id for DownLoad Table (ISDB)
        TID_PCAT          = 0xC2, //!< Table id for Partial Content Announcement Table (ISDB)
        TID_SDTT          = 0xC3, //!< Table id for Software Download Trigger Table (ISDB)
        TID_BIT           = 0xC4, //!< Table id for Broadcaster Information Table (ISDB)
        TID_NBIT_BODY     = 0xC5, //!< Table id for Network Board Information Table (information body) (ISDB)
        TID_NBIT_REF      = 0xC6, //!< Table id for Network Board Information Table (reference to information) (ISDB)
        TID_LDT           = 0xC7, //!< Table id for Linked Description Table (ISDB)
        TID_CDT           = 0xC8, //!< Table id for Common Data Table (ISDB)
        TID_LIT           = 0xD0, //!< Table id for Local Event Information Table (ISDB)
        TID_ERT           = 0xD1, //!< Table id for Event Relation Table (ISDB)
        TID_ITT           = 0xD2, //!< Table id for Index Transmission Table (ISDB)
        TID_AMT           = 0xFE, //!< Table id for Address Map Table (ISDB)
    };

    //!
    //! Get the name of a Table ID.
    //! @param [in] duck TSDuck execution context (used to select from conflicting standards).
    //! @param [in] tid Table id.
    //! @param [in] pid PID of the table, if known.
    //! @param [in] cas CAS id for EMM/ECM table ids.
    //! @param [in] flags Presentation flags.
    //! @return The corresponding name.
    //!
    TSDUCKDLL UString TIDName(const DuckContext& duck, TID tid, PID pid = PID_NULL, CASID cas = CASID_NULL, NamesFlags flags = NamesFlags::NAME);
}
