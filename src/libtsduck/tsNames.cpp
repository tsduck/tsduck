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
//
//  Names of various MPEG entities (namespace ts::names)
//  (OUI names have moved to tsNamesOUI.cpp)
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsMPEG.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Table ID. Use CAS family for EMM/ECM table ids.
//----------------------------------------------------------------------------

std::string ts::names::TID(uint8_t tid, ts::CASFamily cas)
{
    std::string casName;
    if (cas != CAS_OTHER) {
        casName = names::CASFamily(cas) + " ";
    }

    switch (cas) {
        case CAS_MEDIAGUARD: {
            switch (tid) {
                case TID_MG_EMM_U:  return casName + "EMM-U";
                case TID_MG_EMM_A:  return casName + "EMM-A";
                case TID_MG_EMM_G:  return casName + "EMM-G";
                case TID_MG_EMM_I:  return casName + "EMM-I";
                case TID_MG_EMM_C:  return casName + "EMM-C";
                case TID_MG_EMM_CG: return casName + "EMM-GC";
                default: break;
            }
            break;
        }
        case CAS_VIACCESS:
        {
            switch (tid) {
                case TID_VIA_EMM_U:    return casName + "EMM-U";
                case TID_VIA_EMM_GA_E: return casName + "EMM-GA (even)";
                case TID_VIA_EMM_GA_O: return casName + "EMM-GA (odd)";
                case TID_VIA_EMM_GH_E: return casName + "EMM-GH (even)";
                case TID_VIA_EMM_GH_O: return casName + "EMM-GH (odd)";
                case TID_VIA_EMM_S:    return casName + "EMM-S";
                default: break;
            }
            break;
        }
        case CAS_SAFEACCESS: {
            switch (tid) {
                case TID_SA_CECM_82:   return casName + "CECM (even)";
                case TID_SA_CECM_83:   return casName + "CECM (odd)";
                case TID_SA_EMM_STB_U: return casName + "EMM-STB-U";
                case TID_SA_EMM_STB_G: return casName + "EMM-STB-G (all)";
                case TID_SA_EMM_A:     return casName + "EMM-A";
                case TID_SA_EMM_U:     return casName + "EMM-U";
                case TID_SA_EMM_S:     return casName + "EMM-S";
                case TID_SA_EMM_CAM_G: return casName + "EMM-CAM-G";
                case TID_SA_RECM_8A:   return casName + "RECM (even)";
                case TID_SA_RECM_8B:   return casName + "RECM (odd)";
                case TID_SA_EMM_T:     return casName + "EMM-T";
                case TID_LW_DMT:       return "LW/DMT";
                case TID_LW_BDT:       return "LW/BDT";
                case TID_LW_VIT:       return "LW/VIT";
                case TID_LW_VCT:       return "LW/VCT";
                default: break;
            }
            break;
        }
        default: {
            break;
        }
    }

    switch (tid) {
        case 0x00: return "PAT";
        case 0x01: return "CAT";
        case 0x02: return "PMT";
        case 0x03: return "TSDT";
        case 0x04: return "Scene DT";
        case 0x05: return "Object DT";
        case 0x06: return "MetaData";
        case 0x38: return "DSM-CC 0x38";
        case 0x39: return "DSM-CC 0x39";
        case 0x3A: return "DSM-CC MPE";
        case 0x3B: return "DSM-CC UNM";
        case 0x3C: return "DSM-CC DDM";
        case 0x3D: return "DSM-CC SD";
        case 0x3E: return "DSM-CC PD";
        case 0x3F: return "DSM-CC 0x3F";
        case 0x40: return "NIT Actual";
        case 0x41: return "NIT Other";
        case 0x42: return "SDT Actual";
        case 0x46: return "SDT Other";
        case 0x4A: return "BAT";
        case 0x4E: return "EIT p/f Actual";
        case 0x4F: return "EIT p/f Other";
        case 0x70: return "TDT";
        case 0x71: return "RST";
        case 0x72: return "ST";
        case 0x73: return "TOT";
        case 0x74: return "Resolution Notification";
        case 0x75: return "Container";
        case 0x76: return "Related Content";
        case 0x77: return "Content Identifier";
        case 0x78: return "MPE-FEC";
        case 0x7E: return "DIT";
        case 0x7F: return "SIT";
        case 0x80: return casName + "ECM (even)";
        case 0x81: return casName + "ECM (odd)";
        case 0xFF: return "Forbidden TID 0xFF";
        default: break;
    }

    if (tid >= 0x50 && tid <= 0x5F) {
        return Format("EIT schedule Actual (0x%02X)", int(tid));
    }
    else if (tid >= 0x60 && tid <= 0x6F) {
        return Format("EIT schedule Other (0x%02X)", int(tid));
    }
    else if (tid >= 0x82 && tid <= 0x8F) {
        return Format("%sEMM (0x%02X)", casName.c_str(), int(tid));
    }
    else if (tid < 0x40) {
        return Format("MPEG-Reserved (0x%02X)", int(tid));
    }
    else {
        return Format("DVB-Reserved (0x%02X)", int(tid));
    }
}

//----------------------------------------------------------------------------
// Descriptor ID. Use private data specified (pds) if did >= 0x80.
//----------------------------------------------------------------------------

std::string ts::names::DID (uint8_t did, uint32_t pds)
{
    switch (did) {
        // MPEG-defined:
        case 0x01: return "NPT Reference";
        case 0x02: return "Video Stream";
        case 0x03: return "Audio Stream";
        case 0x04: return "Hierarchy";
        case 0x05: return "Registration";
        case 0x06: return "Data Stream Alignment";
        case 0x07: return "Target Background Grid";
        case 0x08: return "Video Window";
        case 0x09: return "CA";
        case 0x0A: return "ISO-639 Language";
        case 0x0B: return "System Clock";
        case 0x0C: return "Multiplex Buffer Utilization";
        case 0x0D: return "Copyright";
        case 0x0E: return "Maximum Bitrate";
        case 0x0F: return "Private Data Indicator";
        case 0x10: return "Smoothing Buffer";
        case 0x11: return "STD";
        case 0x12: return "IBP";
        case 0x13: return "Carousel Indentifier";
        case 0x14: return "Association Tag";
        case 0x15: return "Deferred Association Tags";
        case 0x1B: return "MPEG-4 Video Stream";
        case 0x1C: return "MPEG-4 Audio Stream";
        case 0x1D: return "IOD";
        case 0x1E: return "SL";
        case 0x1F: return "FMC";
        case 0x20: return "External ES id";
        case 0x21: return "MuxCode";
        case 0x22: return "FmxBufferSize";
        case 0x23: return "MultiplexBuffer";
        case 0x24: return "Content labeling";
        case 0x25: return "Metadata Association";
        case 0x26: return "Metadata";
        case 0x27: return "Metadata STD";
        case 0x28: return "AVC Video";
        case 0x29: return "MPEG-2 IPMP";
        case 0x2A: return "AVC Timing and HRD";
        // DVB-defined:
        case 0x40: return "Network Name";
        case 0x41: return "Service List";
        case 0x42: return "Stuffing";
        case 0x43: return "Satellite Delivery System";
        case 0x44: return "Cable Delivery System";
        case 0x45: return "VBI Data";
        case 0x46: return "VBI Teletext";
        case 0x47: return "Bouquet Name";
        case 0x48: return "Service";
        case 0x49: return "Country Availability";
        case 0x4A: return "Linkage";
        case 0x4B: return "NVOD Reference";
        case 0x4C: return "Time Shifted Service";
        case 0x4D: return "Short Event";
        case 0x4E: return "Extended Event";
        case 0x4F: return "Time Shifted Event";
        case 0x50: return "Component";
        case 0x51: return "Mosaic";
        case 0x52: return "Stream Identifier";
        case 0x53: return "CA Identifier";
        case 0x54: return "Content";
        case 0x55: return "Parental Rating";
        case 0x56: return "Teletext";
        case 0x57: return "Telephone";
        case 0x58: return "Local Time Offset";
        case 0x59: return "Subtitling";
        case 0x5A: return "Terrestrial Delivery System";
        case 0x5B: return "Multilingual Network Name";
        case 0x5C: return "Multilingual Bouquet Name";
        case 0x5D: return "Multilingual Service Name";
        case 0x5E: return "Multilingual Component";
        case 0x5F: return "Private Data Specifier";
        case 0x60: return "Service Move";
        case 0x61: return "Short Smoothing Buffer";
        case 0x62: return "Frequency List";
        case 0x63: return "Partial Transport Stream";
        case 0x64: return "Data Broadcast";
        case 0x65: return "Scrambling";
        case 0x66: return "Data Broadcast Id";
        case 0x67: return "Transport Stream";
        case 0x68: return "DSNG";
        case 0x69: return "PDC";
        case 0x6A: return "AC-3";
        case 0x6B: return "Ancillary Data";
        case 0x6C: return "Cell List";
        case 0x6D: return "Cell Frequency Link";
        case 0x6E: return "Announcement Support";
        case 0x6F: return "Application Signalling";
        case 0x70: return "Adaptation Field Data";
        case 0x71: return "Service Identifier";
        case 0x72: return "Service Availability";
        case 0x73: return "Default Authority";
        case 0x74: return "Related Content";
        case 0x75: return "TVA Id";
        case 0x76: return "Content Identifier";
        case 0x77: return "Time Slice FEC Identifier";
        case 0x78: return "ECM Repetition Rate";
        case 0x79: return "S2 Satellite Delivery System";
        case 0x7A: return "Enhanced AC-3";
        case 0x7B: return "DTS";
        case 0x7C: return "AAC";
        case 0x7D: return "XAIT Location";
        case 0x7E: return "FTA Content Management";
        case 0x7F: return "Extension Descriptor";
        case 0xFF: return "Forbidden Descriptor Id 0xFF";
        default: break;
    }

    switch (pds) {
        case PDS_CANALPLUS:
            switch (did) {
                case 0x80: return "DTG Stream Indicator/PIO Offset Time (Canal+)";
                case 0x81: return "Logical Channel/AC-3 Audio Stream (Canal+)";
                case 0x82: return "Private Descriptor 2 (Canal+)";
                case 0x83: return "Logical Channel (Canal+)";
                case 0x84: return "PIO Logo (Canal+)";
                case 0x85: return "ADSL Delivery System (Canal+)";
                case 0x86: return "PIO Fee (Canal+)";
                case 0x88: return "PIO Event Range (Canal+)";
                case 0x8B: return "PIO Copy Management (Canal+)";
                case 0x8C: return "PIO Copy Control (Canal+)";
                case 0x8E: return "PIO PPV (Canal+)";
                case 0x90: return "PIO STB Service Id (Canal+)";
                case 0x91: return "PIO Masking Service Id (Canal+)";
                case 0x92: return "PIO STB Service Map Update (Canal+)";
                case 0x93: return "New Service List (Canal+)";
                case 0x94: return "Message Descriptor Nagra (Canal+)";
                case 0xA1: return "Item Event (Canal+)";
                case 0xA2: return "Item Zapping (Canal+)";
                case 0xA3: return "Appli Message (Canal+)";
                case 0xA4: return "List (Canal+)";
                case 0xB0: return "Key List (Canal+)";
                case 0xB1: return "Picture Signalling (Canal+)";
                case 0xBB: return "Counter (Canal+)";
                case 0xBD: return "Data Component (Canal+)";
                case 0xBE: return "System Management (Canal+)";
                case 0xC0: return "VO Language (Canal+)";
                case 0xC1: return "Data List (Canal+)";
                case 0xC2: return "Appli List (Canal+)";
                case 0xC3: return "Message (Canal+)";
                case 0xC4: return "File (Canal+)";
                case 0xC5: return "Radio Format (Canal+)";
                case 0xC6: return "Appli Startup (Canal+)";
                case 0xC7: return "Patch (Canal+)";
                case 0xC8: return "Loader (Canal+)";
                case 0xC9: return "Channel Map Update (Canal+)";
                case 0xCA: return "PPV (Canal+)";
                case 0xCB: return "Counter (Canal+)";
                case 0xCC: return "Operator Info (Canal+)";
                case 0xCD: return "Service Default Parameters (Canal+)";
                case 0xCE: return "Finger Printing (Canal+)";
                case 0xCF: return "Finger Printing Descriptor V2 (Canal+)";
                case 0xD0: return "Concealed Geo Zones (Canal+)";
                case 0xD1: return "Copy Protection (Canal+)";
                case 0xD3: return "Subscription (Canal+)";
                case 0xD4: return "Cable Backchannel Delivery System (Canal+)";
                case 0xD5: return "Interactivity Snapshot (Canal+)";
                case 0xDC: return "Icon Position (Canal+)";
                case 0xDD: return "Icon Pixmap (Canal+)";
                case 0xDE: return "Zone Coordinate (Canal+)";
                case 0xDF: return "HD Application Control Code (Canal+)";
                case 0xE0: return "Event Repeat (Canal+)";
                case 0xE1: return "PPV V2 (Canal+)";
                case 0xE2: return "Hyperlink Ref (Canal+)";
                case 0xE4: return "Short Service (Canal+)";
                case 0xE5: return "Operator Telephone (Canal+)";
                case 0xE6: return "Item Reference (Canal+)";
                case 0xE9: return "MH Parameters (Canal+)";
                case 0xED: return "Logical Reference (Canal+)";
                case 0xEE: return "Data Version (Canal+)";
                case 0xEF: return "Service Group (Canal+)";
                case 0xF0: return "Stream Locator Transport (Canal+)";
                case 0xF1: return "Data Locator (Canal+)";
                case 0xF2: return "Resident Application (Canal+)";
                case 0xF3: return "Resident Application Signalling (Canal+)";
                case 0xF8: return "MH Logical Reference (Canal+)";
                case 0xF9: return "Record Control (Canal+)";
                case 0xFA: return "CMPS Record Control (Canal+)";
                case 0xFB: return "Episode (Canal+)";
                case 0xFC: return "CMP Selection (Canal+)";
                case 0xFD: return "Data Component (Canal+)";
                case 0xFE: return "System Management (Canal+)";
                default: break;
            }
            break;

        case PDS_LOGIWAYS:
            switch (did) {
                case DID_LW_SUBSCRIPTION: return "Subscription (Logiways)";
                case DID_LW_SCHEDULE: return "Schedule (Logiways)";
                case DID_LW_PRIV_COMPONENT: return "Private Component (Logiways)";
                case DID_LW_PRIV_LINKAGE: return "Private Linkage (Logiways)";
                case DID_LW_CHAPTER: return "Chapter (Logiways)";
                case DID_LW_DRM: return "DRM (Logiways)";
                case DID_LW_VIDEO_SIZE: return "Video Size (Logiways)";
                case DID_LW_EPISODE: return "Episode (Logiways)";
                case DID_LW_PRICE: return "Price (Logiways)";
                case DID_LW_ASSET_REFERENCE: return "Asset Reference (Logiways)";
                case DID_LW_CONTENT_CODING: return "Content Coding (Logiways)";
                case DID_LW_VOD_COMMAND: return "VoD Command (Logiways)";
                case DID_LW_DELETION_DATE: return "Deletion Date (Logiways)";
                case DID_LW_PLAY_LIST: return "Play-List (Logiways)";
                case DID_LW_PLAY_LIST_ENTRY: return "Play-List Entry (Logiways)";
                case DID_LW_ORDER_CODE: return "Order code (Logiways)";
                case DID_LW_BOUQUET_REFERENCE: return "Bouquet reference (Logiways)";
                default: break;
            }
            break;

        case PDS_TPS:
            // Incorrect use of TPS private data, TPS broadcasters should
            // use EACEM/EICTA PDS instead.
        case PDS_EICTA:
            // European Association of Consumer Electronics Manufacturers.
            // Descriptors are in defined in IEC/CENELEC 62216-1
            // "Baseline terrestrial receiver specification"
            switch (did) {
                case DID_LOGICAL_CHANNEL_NUM: return "Logical Channel Number";
                case DID_PREF_NAME_LIST: return "Preferred Name List";
                case DID_PREF_NAME_ID: return "Preferred Name Identifier";
                case DID_EACEM_STREAM_ID: return "EICTA Stream Identifier";
                case DID_HD_SIMULCAST_LCN: return "HD Simulcast Logical Channel Number";
                default: break;
            }
            break;

        case PDS_EUTELSAT:
            // Eutelsat operator, including Fransat
            switch (did) {
                case DID_EUTELSAT_CHAN_NUM: return "Eutelsat Channel Number";
                default: break;
            }
            break;

        default:
            break;
    }

    if (did < 0x40) {
        return Format("MPEG-Reserved (0x%02X)", int(did));
    }
    else if (did < 0x80) {
        return Format("DVB-Reserved (0x%02X)", int(did));
    }
    else {
        return Format("Unknown (0x%02X)", int(did));
    }
}

//----------------------------------------------------------------------------
// Extended descriptor ID. Use private data specified (pds) if edid >= 0x80.
//----------------------------------------------------------------------------

std::string ts::names::EDID(uint8_t edid, uint32_t pds)
{
    switch (edid) {
        case EDID_IMAGE_ICON: return "Image Icon";
        case EDID_CPCM_DELIVERY_SIG: return "CPCM Delivery Signalling";
        case EDID_CP: return "CP";
        case EDID_CP_IDENTIFIER: return "CP Identifier";
        case EDID_T2_DELIVERY: return "T2 Delivery System";
        case EDID_SH_DELIVERY: return "SH Delivery System";
        case EDID_SUPPL_AUDIO: return "Supplementary Audio";
        case EDID_NETW_CHANGE_NOTIFY: return "Network Change Notify";
        case EDID_MESSAGE: return "Message";
        case EDID_TARGET_REGION: return "Target Region";
        case EDID_TARGET_REGION_NAME: return "Target Region Name";
        case EDID_SERVICE_RELOCATED: return "Service Relocated";
        case EDID_XAIT_PID: return "XAIT PID";
        case EDID_C2_DELIVERY: return "C2 Delivery System";
        case EDID_DTS_HD_AUDIO: return "DTS-HD Audio Stream";
        case EDID_DTS_NEURAL: return "DTS Neural";
        case EDID_VIDEO_DEPTH_RANGE: return "Video Depth Range";
        case EDID_T2MI: return "T2MI";
        case EDID_URI_LINKAGE: return "URI Linkage";
        case EDID_CI_ANCILLARY_DATA: return "CI Ancillary Data";
        case EDID_AC4: return "AC-4";
        case EDID_C2_BUNDLE_DELIVERY: return "C2 Bundle Delivery System";
        default:
            if (edid < 0x80) {
                return Format ("DVB-Reserved (0x%02X)", int (edid));
            }
            else {
                return Format ("Unknown (0x%02X)", int (edid));
            }
    }
}

//----------------------------------------------------------------------------
// Private Data Specified
//----------------------------------------------------------------------------

std::string ts::names::PrivateDataSpecifier (uint32_t pds)
{
    switch (pds) {
        case 0x00000000: return "Reserved";
        case 0x00000001: return "SES";
        case 0x00000002: return "BskyB 1";
        case 0x00000003: return "BskyB 2";
        case 0x00000004: return "BskyB 3";
        case 0x00000005: return "ARD, ZDF, ORF";
        case 0x00000006: return "Nokia";
        case 0x00000007: return "AT Entertainment";
        case 0x00000008: return "TV Cabo";
        case 0x00000009: return "Nagravision 1";
        case 0x0000000A: return "Nagravision 2";
        case 0x0000000B: return "Nagravision 3";
        case 0x0000000C: return "Nagravision 4";
        case 0x0000000D: return "Nagravision 5";
        case 0x0000000E: return "Valvision";
        case 0x0000000F: return "Quiero Television";
        case 0x00000010: return "TPS";
        case 0x00000011: return "EchoStar";
        case 0x00000012: return "Telia";
        case 0x00000013: return "Viasat";
        case 0x00000014: return "Senda (Swedish TTV)";
        case 0x00000015: return "MediaKabel";
        case 0x00000016: return "Casema";
        case 0x00000017: return "Humax";
        case 0x00000018: return "@Sky";
        case 0x00000019: return "Singapore DTT";
        case 0x00000020: return "Lyonnaise Cable 1";
        case 0x00000021: return "Lyonnaise Cable 2";
        case 0x00000022: return "Lyonnaise Cable 3";
        case 0x00000023: return "Lyonnaise Cable 4";
        case 0x00000025: return "MTV Europe";
        case 0x00000026: return "Pansonic";
        case 0x00000027: return "Mentor";
        case 0x00000028: return "EACEM/EICTA";
        case 0x00000029: return "NorDig";
        case 0x0000002A: return "Intelsus";
        case 0x00000030: return "Telenor";
        case 0x00000031: return "TeleDenmark";
        case 0x00000035: return "Europe Online Networks";
        case 0x00000038: return "OTE";
        case 0x00000039: return "Telewizja Polsat";
        case 0x000000A0: return "Sentech";
        case 0x000000A1: return "TechniSat";
        case 0x000000A2: return "Logiways";
        case 0x000000BE: return "BetaTechnik";
        case 0x000000C0: return "Canal+";
        case 0x000000D0: return "Dolby Laboratories";
        case 0x000000E0: return "ExpressVu";
        case 0x000000F0: return "France Telecom, CNES, DGA (STENTOR)";
        case 0x00000100: return "OpenTV";
        case 0x00000150: return "Loewe Opta";
        case 0x0000055F: return "Eutelsat";
        case 0x00000600: return "UPC 1";
        case 0x00000601: return "UPC 2";
        case 0x00001000: return "TPS";
        case 0x000022D4: return "Spanish Broadcast Regulator";
        case 0x000022F1: return "Swedish Broadcast Regulator";
        case 0x0000233A: return "Independent television Commission";
        case 0x00006000: return "News Datacom";
        case 0x00006001: return "NDC 1";
        case 0x00006002: return "NDC 2";
        case 0x00006003: return "NDC 3";
        case 0x00006004: return "NDC 4";
        case 0x00006005: return "NDC 5";
        case 0x00006006: return "NDC 6";
        case 0x00362275: return "Irdeto";
        case 0x004E544C: return "NTL";
        case 0x00532D41: return "Scientific Atlanta";
        case 0x5347444E: return "StarGuide Digial Networks";
        case 0x00600000: return "Rhone Vision Cable";
        case 0x44414E59: return "News Datacom (IL) 1";
        case 0x46524549: return "News Datacom (IL) 1";
        case 0x4A4F4A4F: return "MSG MediaServices";
        case 0x53415053: return "Scientific Atlanta";
        case 0xBBBBBBBB: return "Bertelsmann";
        case 0xECCA0001: return "ECCA";
        case 0xFCFCFCFC: return "France Telecom";
        default:
            if (pds >= 0x46545600 && pds <= 0x46545620) {
                return Format ("FreeTV %d", int (pds - 0x46545600));
            }
            else if (pds >= 0x4F545600 && pds <= 0x4F5456FF) {
                return Format ("OpenTV %d", int (pds - 0x4F545600));
            }
            else if (pds >= 0x50484900 && pds <= 0x504849FF) {
                return Format ("Philips DVS %d", int (pds - 0x50484900));
            }
            else {
                return Format ("Undefined 0x%08X", pds);
            }
    }
}

//----------------------------------------------------------------------------
// Stream type (in PMT)
//----------------------------------------------------------------------------

std::string ts::names::StreamType (uint8_t type)
{
    switch (type) {
        // MPEG values
        case 0x01: return "MPEG-1 Video";
        case 0x02: return "MPEG-2 Video";
        case 0x03: return "MPEG-1 Audio";
        case 0x04: return "MPEG-2 Audio";
        case 0x05: return "MPEG-2 Private sections";
        case 0x06: return "MPEG-2 PES private data";
        case 0x07: return "MHEG";
        case 0x08: return "DSM-CC";
        case 0x09: return "MPEG-2 over ATM";
        case 0x0A: return "DSM-CC MPE";
        case 0x0B: return "DSM-CC U-N";
        case 0x0C: return "DSM-CC Stream Descriptors";
        case 0x0D: return "DSM-CC Sections";
        case 0x0E: return "MPEG-2 Auxiliary";
        case 0x0F: return "MPEG-2 AAC Audio";
        case 0x10: return "MPEG-4 Video";
        case 0x11: return "MPEG-4 AAC Audio";
        case 0x12: return "MPEG-4 SL or FlexMux in PES packets";
        case 0x13: return "MPEG-4 SL or FlexMux in MPEG-4 sections";
        case 0x14: return "DSM-CC Synchronized Download Protocol";
        case 0x15: return "MetaData in PES packets";
        case 0x16: return "MetaData in sections";
        case 0x17: return "MetaData in DSM-CC Data Carousel";
        case 0x18: return "MetaData in DSM-CC Object Carousel";
        case 0x19: return "MetaData in DSM-CC Sync. Download Protocol";
        case 0x1A: return "MPEG-2 IPMP";
        case 0x1B: return "AVC video";
        case 0x7F: return "IPMP";
        // ATSC values
        case 0x81: return "ATSC AC-3 Audio";
        case 0x87: return "ATSC Enhanced-AC-3 Audio";
        // Canal+ values
        case 0xBF: return "C+ Dummy stream";
        case 0xC0: return "C+ Module";
        case 0xC1: return "C+ Data Appli";
        case 0xC2: return "C+ Table index";
        case 0xC3: return "C+ File";
        case 0xC4: return "C+ Diapos";
        case 0xC5: return "C+ Synchro";
        case 0xC6: return "C+ Boot table";
        case 0xC7: return "C+ Patch";
        case 0xC8: return "C+ Loader";
        case 0xC9: return "C+ CDNT / Update table";
        case 0xCA: return "C+ Data system table";
        case 0xCB: return "C+ ACT";
        case 0xCE: return "C+ CMPS ECM";
        case 0xCF: return "C+ CMPT (CMPS scenario)";
        case 0xD0: return "C+ Appli loader";
        default:
            if (type < 0x80) {
                return Format ("Reserved stream type 0x%02X", int (type));
            }
            else {
                return Format ("User-defined stream type 0x%02X", int (type));
            }
    }
}

//----------------------------------------------------------------------------
// Stream ID (in PES header)
//----------------------------------------------------------------------------

std::string ts::names::StreamId (uint8_t sid)
{
    switch (sid) {
        case SID_PSMAP:      return "Program stream map";
        case SID_PRIV1:      return "Private stream 1";
        case SID_PAD:        return "Padding stream";
        case SID_PRIV2:      return "Private stream 2";
        case SID_ECM:        return "ECM";
        case SID_EMM:        return "EMM";
        case SID_DSMCC:      return "DSM-CC data";
        case SID_ISO13522:   return "ISO-13522 Hypermedia";
        case SID_H222_1_A:   return "H.222.1 type A";
        case SID_H222_1_B:   return "H.222.1 type B";
        case SID_H222_1_C:   return "H.222.1 type C";
        case SID_H222_1_D:   return "H.222.1 type D";
        case SID_H222_1_E:   return "H.222.1 type E";
        case SID_ANCILLARY:  return "Ancillary stream";
        case SID_PSDIR:      return "Program stream directory";
        case SID_MP4_SLPACK: return "MPEG-4 SL-packetized stream";
        case SID_MP4_FLEXM:  return "MPEG-4 FlexMux";
        case SID_METADATA:   return "MPEG-7 MetaData";
        case SID_EXTENDED:   return "Extended stream id";
        case SID_RESERVED:   return "Reserved";
        default:
            if (IsAudioSID (sid)) {
                return Format ("Audio %d", int (sid & SID_AUDIO_MASK));
            }
            else if (IsVideoSID (sid)) {
                return Format ("Video %d", int (sid & SID_VIDEO_MASK));
            }
            else {
                return Format ("Invalid 0x%02X", int (sid));
            }
    }
}

//----------------------------------------------------------------------------
// PES start code value
//----------------------------------------------------------------------------

std::string ts::names::PESStartCode (uint8_t code)
{
    switch (code) {
        case PST_PICTURE:         return "Picture";
        case PST_USER_DATA:       return "User data";
        case PST_SEQUENCE_HEADER: return "Sequence header";
        case PST_SEQUENCE_ERROR:  return "Sequence error";
        case PST_EXTENSION:       return "Extension";
        case PST_SEQUENCE_END:    return "Sequence end";
        case PST_GROUP:           return "Group";
        default:
            if (code >= PST_SYSTEM_MIN) {
                return StreamId (code);
            }
            else if (code >= PST_SLICE_MIN && code <= PST_SLICE_MAX) {
                return Format ("Slice 0x%02X", int (code));
            }
            else {
                return Format ("Reserved 0x%02X", int (code));
            }
    }
}

//----------------------------------------------------------------------------
// Aspect ratio values (in MPEG-1/2 video sequence header)
//----------------------------------------------------------------------------

std::string ts::names::AspectRatio (uint8_t ar)
{
    switch (ar) {
        case AR_SQUARE: return "1/1";
        case AR_4_3:    return "4/3";
        case AR_16_9:   return "16/9";
        case AR_221:    return "2.21/1";
        default:        return Format("Reserved %d", int (ar));
    }
}

//----------------------------------------------------------------------------
// Chroma format values (in MPEG-1/2 video sequence header)
//----------------------------------------------------------------------------

std::string ts::names::ChromaFormat (uint8_t cf)
{
    switch (cf) {
        case CHROMA_MONO: return "monochrome";
        case CHROMA_420:  return "4:2:0";
        case CHROMA_422:  return "4:2:2";
        case CHROMA_444:  return "4:4:4";
        default:          return Format("Reserved %d", int (cf));
    }
}

//----------------------------------------------------------------------------
// AVC (ISO 14496-10, ITU H.264) access unit (aka "NALunit") type
//----------------------------------------------------------------------------

std::string ts::names::AVCUnitType (uint8_t type)
{
    switch (type) {
        case AVC_AUT_NON_IDR:      return "Coded slice of a non-IDR picture";
        case AVC_AUT_SLICE_A:      return "Coded slice data partition A";
        case AVC_AUT_SLICE_B:      return "Coded slice data partition B";
        case AVC_AUT_SLICE_C:      return "Coded slice data partition C";
        case AVC_AUT_IDR:          return "Coded slice of an IDR picture";
        case AVC_AUT_SEI:          return "Supplemental enhancement information (SEI)";
        case AVC_AUT_SEQPARAMS:    return "Sequence parameter set";
        case AVC_AUT_PICPARAMS:    return "Picture parameter set";
        case AVC_AUT_DELIMITER:    return "Access unit delimiter";
        case AVC_AUT_END_SEQUENCE: return "End of sequence";
        case AVC_AUT_END_STREAM:   return "End of stream";
        case AVC_AUT_FILLER:       return "Filler data";
        case AVC_AUT_SEQPARAMSEXT: return "Sequence parameter set extension";
        case AVC_AUT_PREFIX:       return "Prefix NAL unit in scalable extension";
        case AVC_AUT_SUBSETPARAMS: return "Subset sequence parameter set";
        case AVC_AUT_SLICE_NOPART: return "Coded slice without partitioning";
        case AVC_AUT_SLICE_SCALE:  return "Coded slice in scalable extension";
        default:                   return Format("Reserved %d", int (type));
    }
}

//----------------------------------------------------------------------------
// AVC (ISO 14496-10, ITU H.264) profile
//----------------------------------------------------------------------------

std::string ts::names::AVCProfile (int profile)
{
    switch (profile) {
        case 44:  return "CAVLC 4:4:4 intra profile";
        case 66:  return "baseline profile";
        case 77:  return "main profile";
        case 83:  return "scalable baseline profile";
        case 86:  return "scalable high profile";
        case 88:  return "extended profile";
        case 100: return "high profile";
        case 110: return "high 10 profile";
        case 122: return "high 4:2:2 profile";
        case 244: return "high 4:4:4 predictive profile";
        default:  return Format ("AVC profile %d", profile);
    }
}

//----------------------------------------------------------------------------
// Service type (in Service Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::ServiceType (uint8_t type)
{
    switch (type) {
        case 0x01: return "Digital television service";
        case 0x02: return "Digital radio sound service";
        case 0x03: return "Teletext service";
        case 0x04: return "NVOD reference service";
        case 0x05: return "NVOD time-shifted service";
        case 0x06: return "Mosaic service";
        case 0x07: return "PAL-coded signal";
        case 0x08: return "SECAM-coded signal";
        case 0x09: return "D/D2-MAC";
        case 0x0A: return "FM radio";
        case 0x0B: return "NTSC-coded signal";
        case 0x0C: return "Data broadcast service";
        case 0x0D: return "Common Interface usage";
        case 0x0E: return "RCS map";
        case 0x0F: return "RCS FLS";
        case 0x10: return "DVB-MHP service";
        case 0x11: return "MPEG-2 HD digital television service";
        case 0x16: return "Advanced codec SD digital television service";
        case 0x17: return "Advanced codec SD NVOD time-shifted service";
        case 0x18: return "Advanced codec SD NVOD reference service";
        case 0x19: return "Advanced codec HD digital television service";
        case 0x1A: return "Advanced codec HD NVOD time-shifted service";
        case 0x1B: return "Advanced codec HD NVOD reference service";
        default:
            if (type < 0x80 || type == 0xFF) {
                return Format ("Reserved service type 0x%02X", int (type));
            }
            else {
                return Format ("User-defined service type 0x%02X", int (type));
            }
    }
}

//----------------------------------------------------------------------------
// Linkage type (in Linkage Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::LinkageType (uint8_t type)
{
    switch (type) {
        case 0x01: return "information service";
        case 0x02: return "EPG service";
        case 0x03: return "CA replacement service";
        case 0x04: return "TS containing complete Network/Bouquet SI";
        case 0x05: return "service replacement service";
        case 0x06: return "data broadcast service";
        case 0x07: return "RCS map";
        case 0x08: return "mobile hand-over";
        case 0x09: return "system software update service";
        case 0x0A: return "TS containing SSU BAT or NIT";
        case 0x0B: return "IP/MAC notification service";
        case 0x0C: return "TS containing INT BAT or NIT";
        case 0x0D: return "event linkage";
        default:
            if (type < 0x80 || type == 0xFF) {
                return Format ("Reserved linkage type 0x%02X", int (type));
            }
            else {
                return Format ("User-defined linkage type 0x%02X", int (type));
            }
    }
}

//----------------------------------------------------------------------------
// Teletext type (in Teletext Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::TeletextType (uint8_t type)
{
    switch (type) {
        case 0x01: return "Initial Teletext page";
        case 0x02: return "Teletext subtitles";
        case 0x03: return "Additional information page";
        case 0x04: return "Programme schedule page";
        case 0x05: return "Teletext subtitles for hearing impaired";
        default:   return Format("Reserved teletext type 0x%02X", int (type));
    }
}


//----------------------------------------------------------------------------
// Name of Conditional Access Families.
//----------------------------------------------------------------------------

std::string ts::names::CASFamily(ts::CASFamily cas)
{
    switch (cas) {
        case CAS_OTHER:       return "Other";
        case CAS_MEDIAGUARD:  return "MediaGuard";
        case CAS_NAGRA:       return "Nagravision";
        case CAS_VIACCESS:    return "Viaccess";
        case CAS_THALESCRYPT: return "ThalesCrypt";
        case CAS_SAFEACCESS:  return "SafeAccess";
        default:              return Format("CAS Family %d", int(cas));
    }
}


//----------------------------------------------------------------------------
// Conditional Access System Id (in CA Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::CASId(uint16_t ca_sysid)
{
    struct Name {
        uint16_t first;
        const char* name;
    };

    static const Name names [] = {
        {0x0000, "Reserved"},
        {0x0001, "Standardized systems"},
        {0x0100, "MediaGuard"},
        {0x0200, "CCETT"},
        {0x0300, "MSG"},
        {0x0400, "Eurodec"},
        {0x0500, "Viaccess"},
        {0x0600, "Irdeto"},
        {0x0700, "Motorola"},
        {0x0800, "Matra"},
        {0x0900, "NDS"},
        {0x0A00, "Nokia"},
        {0x0B00, "Conax"},
        {0x0C00, "NTL"},
        {0x0D00, "CryptoWorks"},
        {0x0E00, "Scientific Atlanta"},
        {0x0F00, "Sony"},
        {0x1000, "Tandberg"},
        {0x1100, "Thomson"},
        {0x1200, "TV/Com"},
        {0x1300, "HPT"},
        {0x1400, "HRT"},
        {0x1500, "IBM"},
        {0x1600, "Nera"},
        {0x1700, "BetaCrypt"},
        {0x1800, "Nagravision"},
        {0x1900, "Titan"},
        {0x1A00, "Unknown"},
        {0x2000, "Telefonica"},
        {0x2100, "Stentor"},
        {0x2200, "Tadiran Scopus"},
        {0x2300, "Barco"},
        {0x2400, "StarGuide"},
        {0x2500, "Mentor"},
        {0x2600, "European Broadcasting Union"},
        {0x2700, "Unknown"},
        {0x4700, "General Instruments"},
        {0x4800, "Telemann"},
        {0x4900, "DTV Alliance China"},
        {0x4A00, "Tsinghua TongFang"},
        {0x4A10, "Easycas"},
        {0x4A20, "AlphaCrypt"},
        {0x4A30, "DVN"},
        {0x4A40, "ADT"},
        {0x4A50, "Shenzhen Kingsky"},
        {0x4A60, "@Sky"},
        {0x4A70, "DreamCrypt"},
        {0x4A80, "ThalesCrypt"},
        {0x4A90, "Unknown"},
        {0x4ADC, "SafeAccess"},
        {0x4ADD, 0} // end of list
    };

    for (const Name* n = names; n->name != 0; n++) {
        if (ca_sysid < n[1].first) {
            return n->name;
        }
    }

    return Format("Unknown CAS 0x%04X", int(ca_sysid));
}

//----------------------------------------------------------------------------
// Running Status (in SDT)
//----------------------------------------------------------------------------

std::string ts::names::RunningStatus (uint8_t status)
{
    switch (status) {
        case  0: return "undefined";
        case  1: return "not running";
        case  2: return "starting";
        case  3: return "pausing";
        case  4: return "running";
        case  5: return "off-air";
        case  6:
        case  7: return Format ("reserved 0x%02X", int (status));
        default: return Format ("invalid 0x%02X", int (status));
    }
}

//----------------------------------------------------------------------------
// Audio type (in ISO639 Language Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::AudioType (uint8_t status)
{
    switch (status) {
        case 0x00: return "undefined";
        case 0x01: return "clean effects";
        case 0x02: return "hearing impaired";
        case 0x03: return "visual impaired commentary";
        default:   return Format ("reserved 0x%02X", int (status));
    }
}

//----------------------------------------------------------------------------
// Subtitling type (in Subtitling Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::SubtitlingType (uint8_t type)
{
    switch (type) {
        case 0x01: return "EBU Teletext subtitles";
        case 0x02: return "Associated EBU Teletext";
        case 0x03: return "VBI data";
        case 0x10: return "DVB subtitles, no aspect ratio";
        case 0x11: return "DVB subtitles, 4:3 aspect ratio";
        case 0x12: return "DVB subtitles, 16:9 aspect ratio";
        case 0x13: return "DVB subtitles, 2.21:1 aspect ratio";
        case 0x14: return "DVB subtitles, high definition";
        case 0x20: return "DVB subtitles for hard of hearing, no aspect ratio";
        case 0x21: return "DVB subtitles for hard of hearing, 4:3 aspect ratio";
        case 0x22: return "DVB subtitles for hard of hearing, 16:9 aspect ratio";
        case 0x23: return "DVB subtitles for hard of hearing, 2.21:1 aspect ratio";
        case 0x24: return "DVB subtitles for hard of hearing, high definition";
        case 0x30: return "Open (in-vision) sign language interpretation for the deaf";
        case 0x31: return "Closed sign language interpretation for the deaf";
        case 0x40: return "Video up-sampled from standard definition source";
        default:
            if (type < 0xB0 || type == 0xFF) {
                return Format ("Reserved subtitling type 0x%02X", int (type));
            }
            else {
                return Format ("User-defined subtitling type 0x%02X", int (type));
            }
    }
}

//----------------------------------------------------------------------------
// Component Type (in Component Descriptor)
// Combination of stream_content (4 bits) and component_type (8 bits)
//----------------------------------------------------------------------------

std::string ts::names::ComponentType (uint16_t type)
{
    // Specific types

    switch (type) {
        case 0x0101: return "MPEG-2 video, 4:3 aspect ratio, 25 Hz";
        case 0x0102: return "MPEG-2 video, 16:9 aspect ratio with pan vectors, 25 Hz";
        case 0x0103: return "MPEG-2 video, 16:9 aspect ratio without pan vectors, 25 Hz";
        case 0x0104: return "MPEG-2 video, > 16:9 aspect ratio, 25 Hz";
        case 0x0105: return "MPEG-2 video, 4:3 aspect ratio, 30 Hz";
        case 0x0106: return "MPEG-2 video, 16:9 aspect ratio with pan vectors, 30 Hz";
        case 0x0107: return "MPEG-2 video, 16:9 aspect ratio without pan vectors, 30 Hz";
        case 0x0108: return "MPEG-2 video, > 16:9 aspect ratio, 30 Hz";
        case 0x0109: return "MPEG-2 high definition video, 4:3 aspect ratio, 25 Hz";
        case 0x010A: return "MPEG-2 high definition video, 16:9 aspect ratio with pan vectors, 25 Hz";
        case 0x010B: return "MPEG-2 high definition video, 16:9 aspect ratio without pan vectors, 25 Hz";
        case 0x010C: return "MPEG-2 high definition video, > 16:9 aspect ratio, 25 Hz";
        case 0x010D: return "MPEG-2 high definition video, 4:3 aspect ratio, 30 Hz";
        case 0x010E: return "MPEG-2 high definition video, 16:9 aspect ratio with pan vectors, 30 Hz";
        case 0x010F: return "MPEG-2 high definition video, 16:9 aspect ratio without pan vectors, 30 Hz";
        case 0x0110: return "MPEG-2 high definition video, > 16:9 aspect ratio, 30 Hz";
        case 0x0201: return "MPEG-1 Layer 2 audio, single mono channel";
        case 0x0202: return "MPEG-1 Layer 2 audio, dual mono channel";
        case 0x0203: return "MPEG-1 Layer 2 audio, stereo (2 channel)";
        case 0x0204: return "MPEG-1 Layer 2 audio, multi-lingual, multi-channel";
        case 0x0205: return "MPEG-1 Layer 2 audio, surround sound";
        case 0x0240: return "MPEG-1 Layer 2 audio description for the visually impaired";
        case 0x0241: return "MPEG-1 Layer 2 audio for the hard of hearing";
        case 0x0242: return "Receiver-mix supplementary audio";
        case 0x0247: return "MPEG-1 Layer 2 audio, receiver-mix audio description";
        case 0x0248: return "MPEG-1 Layer 2 audio, broadcaster-mix audio description";
        case 0x0501: return "H.264/AVC standard definition video, 4:3 aspect ratio, 25 Hz";
        case 0x0503: return "H.264/AVC standard definition video, 16:9 aspect ratio, 25 Hz";
        case 0x0504: return "H.264/AVC standard definition video, > 16:9 aspect ratio, 25 Hz";
        case 0x0505: return "H.264/AVC standard definition video, 4:3 aspect ratio, 30 Hz";
        case 0x0507: return "H.264/AVC standard definition video, 16:9 aspect ratio, 30 Hz";
        case 0x0508: return "H.264/AVC standard definition video, > 16:9 aspect ratio, 30 Hz";
        case 0x050B: return "H.264/AVC high definition video, 16:9 aspect ratio, 25 Hz";
        case 0x050C: return "H.264/AVC high definition video, > 16:9 aspect ratio, 25 Hz";
        case 0x050F: return "H.264/AVC high definition video, 16:9 aspect ratio, 30 Hz";
        case 0x0510: return "H.264/AVC high definition video, > 16:9 aspect ratio, 30 Hz";
        case 0x0601: return "HE-AAC audio, single mono channel";
        case 0x0603: return "HE-AAC audio, stereo";
        case 0x0605: return "HE-AAC audio, surround sound";
        case 0x0640: return "HE-AAC audio description for the visually impaired";
        case 0x0641: return "HE-AAC audio for the hard of hearing";
        case 0x0642: return "HE-AAC receiver-mixed supplementary audio";
        case 0x0643: return "HE-AAC v2 audio, stereo";
        case 0x0644: return "HE-AAC v2 audio description for the visually impaired";
        case 0x0645: return "HE-AAC v2 audio for the hard of hearing";
        case 0x0646: return "HE-AAC v2 receiver-mix supplementary audio";
        case 0x0647: return "HE-AAC receiver-mix audio description for the visually impaired";
        case 0x0648: return "HE-AAC broadcaster-mix audio description for the visually impaired";
        case 0x0649: return "HE-AAC v2 receiver-mix audio description for the visually impaired";
        case 0x064A: return "HE-AAC v2 broadcaster-mix audio description for the visually impaired";
        case 0x0801: return "DVB SRM data";
        default:     break;
    }

    // Dedicated types

    if ((type & 0xFF00) == 0x0300) {
        return SubtitlingType (type & 0x00FF);
    }
    if ((type & 0xFF00) == 0x0400) {
        return AC3ComponentType (type & 0x00FF);
    }

    const char* s;
    switch (type & 0xFF00) {
        case 0x0100: s = "MPEG-2 video, "; break;
        case 0x0200: s = "MPEG-1 audio, "; break;
        case 0x0500: s = "H.264/AVC, "; break;
        case 0x0600: s = "HE-AAC, "; break;
        case 0x0700: s = "DTS, "; break;
        case 0x0800: s = "DVB CPCM, "; break;
        default:     s = ""; break;
    }
    if ((type & 0x00FF) >= 0x00B0 && (type & 0x00FF) <= 0x00FE) {
        return Format ("%sUser defined (0x%04X)", s, int (type));
    }
    else {
        return Format ("%sReserved (0x%04X)", s, int (type));
    }
}

//----------------------------------------------------------------------------
// AC-3 Component Type
//----------------------------------------------------------------------------

std::string ts::names::AC3ComponentType (uint8_t type)
{
    std::string s ((type & 0x80) ? "Enhanced AC-3" : "AC-3");

    s += (type & 0x40) ? ", full" : ", combined";

    switch (type & 0x38) {
        case 0x00: s += ", complete main"; break;
        case 0x08: s += ", music and effects"; break;
        case 0x10: s += ", visually impaired"; break;
        case 0x18: s += ", hearing impaired"; break;
        case 0x20: s += ", dialogue"; break;
        case 0x28: s += ", commentary"; break;
        case 0x30: s += ", emergency"; break;
        case 0x38: s += (type & 0x40) ? ", karaoke" : ", voiceover"; break;
        default: assert (false); // unreachable
    }

    switch (type & 0x07) {
        case 0:  s += ", mono"; break;
        case 1:  s += ", 1+1 channel"; break;
        case 2:  s += ", 2 channels"; break;
        case 3:  s += ", 2 channels dolby surround"; break;
        case 4:  s += ", multichannel > 2"; break;
        case 5:  s += ", multichannel > 5.1"; break;
        case 6:  s += ", multiple substreams"; break;
        case 7:  s += ", reserved"; break;
        default: assert (false); // unreachable
    }

    return s;
}

//----------------------------------------------------------------------------
// DTS Audio Sample Rate code
//----------------------------------------------------------------------------

std::string ts::names::DTSSampleRateCode (uint8_t x)
{
    switch (x) {
        case  0: return "invalid";
        case  1: return "8 kHz";
        case  2: return "16 kHz";
        case  3: return "32 kHz";
        case  4: return "64 kHz";
        case  5: return "128 kHz";
        case  6: return "11.025 kHz";
        case  7: return "22.05 kHz";
        case  8: return "44.1 kHz";
        case  9: return "88.2 kHz";
        case 10: return "176.4 kHz";
        case 11: return "12 kHz";
        case 12: return "24 kHz";
        case 13: return "48 kHz";
        case 14: return "96 kHz";
        case 15: return "192 kHz";
        default: return Format ("illegal value %d", int (x));
    }
}

//----------------------------------------------------------------------------
// DTS Audio Bit Rate Code
//----------------------------------------------------------------------------

std::string ts::names::DTSBitRateCode (uint8_t x)
{
    switch (x & 0x1F) {
        case  5: return "128 kb/s";
        case  6: return "192 kb/s";
        case  7: return "224 kb/s";
        case  8: return "256 kb/s";
        case  9: return "320 kb/s";
        case 10: return "384 kb/s";
        case 11: return "448 kb/s";
        case 12: return "512 kb/s";
        case 13: return "576 kb/s";
        case 14: return "640 kb/s";
        case 15: return "768 kb/s";
        case 16: return "920 kb/s";
        case 17: return "1024 kb/s";
        case 18: return "1152 kb/s";
        case 19: return "1280 kb/s";
        case 20: return "1344 kb/s";
        case 21: return "1408 kb/s";
        case 22: return "1411.2 kb/s";
        case 23: return "1472 kb/s";
        case 24: return "1536 kb/s";
        case 25: return "1920 kb/s";
        case 26: return "2048 kb/s";
        case 27: return "3072 kb/s";
        case 28: return "3840 kb/s";
        case 29: return "open";
        case 30: return "variable";
        case 31: return "lossless";
        default: return Format ("illegal value %d", int (x));
    }
}

//----------------------------------------------------------------------------
// DTS Audio Surround Mode
//----------------------------------------------------------------------------

std::string ts::names::DTSSurroundMode (uint8_t x)
{
    switch (x) {
        case  0: return "1 / mono";
        case  1: return "illegal";
        case  2: return "2 / L+R (stereo)";
        case  3: return "2 / (L+R) + (L-R) (sum-difference)";
        case  4: return "2 / LT+RT (left & right total)";
        case  5: return "3 / C+L+R";
        case  6: return "3 / L+R+S";
        case  7: return "4 / C+L+R+S";
        case  8: return "4 / L+R+SL+SR";
        case  9: return "5 / C+L+R+SL+SR";
        default: return Format ("user defined %d", int (x));
    }
}

//----------------------------------------------------------------------------
// DTS Audio Extended Surround Mode
//----------------------------------------------------------------------------

std::string ts::names::DTSExtendedSurroundMode (uint8_t x)
{
    switch (x) {
        case  0: return "none";
        case  1: return "matrixed";
        case  2: return "discrete";
        default: return Format ("undefined %d", int (x));
    }
}

//----------------------------------------------------------------------------
// Content name (in Content Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::Content (uint8_t x)
{
    switch (x) {
        case 0x10: return "movie/drama (general)";
        case 0x11: return "detective/thriller";
        case 0x12: return "adventure/western/war";
        case 0x13: return "science fiction/fantasy/horror";
        case 0x14: return "comedy";
        case 0x15: return "soap/melodrama/folkloric";
        case 0x16: return "romance";
        case 0x17: return "serious/classical/religious/historical movie/drama";
        case 0x18: return "adult movie/drama";
        case 0x20: return "news/current affairs (general)";
        case 0x21: return "news/weather report";
        case 0x22: return "news magazine";
        case 0x23: return "documentary";
        case 0x24: return "discussion/interview/debate";
        case 0x30: return "show/game show (general)";
        case 0x31: return "game show/quiz/contest";
        case 0x32: return "variety show";
        case 0x33: return "talk show";
        case 0x40: return "sports (general)";
        case 0x41: return "special events (Olympic Games, World Cup, etc.)";
        case 0x42: return "sports magazines";
        case 0x43: return "football/soccer";
        case 0x44: return "tennis/squash";
        case 0x45: return "team sports (excluding football)";
        case 0x46: return "athletics";
        case 0x47: return "motor sport";
        case 0x48: return "water sport";
        case 0x49: return "winter sports";
        case 0x4A: return "equestrian";
        case 0x4B: return "martial sports";
        case 0x50: return "children's/youth programmes (general)";
        case 0x51: return "pre-school children's programmes";
        case 0x52: return "entertainment programmes for 6 to 14";
        case 0x53: return "entertainment programmes for 10 to 16";
        case 0x54: return "informational/educational/school programmes";
        case 0x55: return "cartoons/puppets";
        case 0x60: return "music/ballet/dance (general)";
        case 0x61: return "rock/pop";
        case 0x62: return "serious music/classical music";
        case 0x63: return "folk/traditional music";
        case 0x64: return "jazz";
        case 0x65: return "musical/opera";
        case 0x66: return "ballet";
        case 0x70: return "arts/culture (without music, general)";
        case 0x71: return "performing arts";
        case 0x72: return "fine arts";
        case 0x73: return "religion";
        case 0x74: return "popular culture/traditional arts";
        case 0x75: return "literature";
        case 0x76: return "film/cinema";
        case 0x77: return "experimental film/video";
        case 0x78: return "broadcasting/press";
        case 0x79: return "new media";
        case 0x7A: return "arts/culture magazines";
        case 0x7B: return "fashion";
        case 0x80: return "social/political issues/economics (general)";
        case 0x81: return "magazines/reports/documentary";
        case 0x82: return "economics/social advisory";
        case 0x83: return "remarkable people";
        case 0x90: return "education/science/factual topics (general)";
        case 0x91: return "nature/animals/environment";
        case 0x92: return "technology/natural sciences";
        case 0x93: return "medicine/physiology/psychology";
        case 0x94: return "foreign countries/expeditions";
        case 0x95: return "social/spiritual sciences";
        case 0x96: return "further education";
        case 0x97: return "languages";
        case 0xA0: return "leisure hobbies (general)";
        case 0xA1: return "tourism/travel";
        case 0xA2: return "handicraft";
        case 0xA3: return "motoring";
        case 0xA4: return "fitness and health";
        case 0xA5: return "cooking";
        case 0xA6: return "advertisement/shopping";
        case 0xA7: return "gardening";
        case 0xB0: return "original language";
        case 0xB1: return "black and white";
        case 0xB2: return "unpublished";
        case 0xB3: return "live broadcast";
        default:   return Format ("(%s, 0x%02X)", (x & 0x0F) == 0x0F ? "user-defined" : "reserved", int (x));
    }
}

//----------------------------------------------------------------------------
// Scrambling control value in TS header
//----------------------------------------------------------------------------

std::string ts::names::ScramblingControl (uint8_t scv)
{
    switch (scv) {
        case  0: return "clear";
        case  1: return "DVB-reserved (1)";
        case  2: return "even";
        case  3: return "odd";
        default: return Format ("invalid (%d)", int (scv));
    }
}

//----------------------------------------------------------------------------
// Bouquet Id
//----------------------------------------------------------------------------

std::string ts::names::BouquetId (uint16_t id)
{
    switch (id) {
        case 0x0086: return "Tv Numeric";
        case 0xC002: return "Canal+";
        case 0xC003: return "Canal+ TNT";
        case 0xFF00: return "DVB System Software Update";
        default: break;
    }

    if (id >= 0x1000 && id <= 0x101F) {
        return Format ("BskyB %d", int (id - 0x1000 + 1));
    }
    if (id >= 0x1020 && id <= 0x103F) {
        return Format ("Dish Network %d", int (id - 0x1020 + 1));
    }
    if (id >= 0x3000 && id <= 0x300F) {
        return Format ("TPS %d", int (id - 0x3000 + 1));
    }
    if (id >= 0x4040 && id <= 0x407F) {
        return Format ("OpenTV %d", int (id - 0x4040 + 1));
    }
    if (id >= 0xC000 && id <= 0xC01F) {
        return Format ("Canal+ %d", int (id - 0xC000 + 1));
    }
    if (id >= 0xFC00 && id <= 0xFCFF) {
        return Format ("France Telecom %d", int (id - 0xFC00 + 1));
    }

    return Format ("0x%04X", int (id));
}

//----------------------------------------------------------------------------
// Data broadcast id (in Data Broadcast Id Descriptor)
//----------------------------------------------------------------------------

std::string ts::names::DataBroadcastId (uint16_t id)
{
    switch (id) {
        case DBID_DATA_PIPE:            return "Data pipe";
        case DBID_ASYNC_DATA_STREAM:    return "Asynchronous data stream";
        case DBID_SYNC_DATA_STREAM:     return "Synchronous data stream";
        case DBID_SYNCED_DATA_STREAM:   return "Synchronised data stream";
        case DBID_MPE:                  return "Multi protocol encapsulation";
        case DBID_DATA_CSL:             return "Data Carousel";
        case DBID_OBJECT_CSL:           return "Object Carousel";
        case DBID_ATM:                  return "DVB ATM streams";
        case DBID_HP_ASYNC_DATA_STREAM: return "Higher Protocols based on asynchronous data streams";
        case DBID_SSU:                  return "System Software Update service [TS 102 006]";
        case DBID_IPMAC_NOTIFICATION:   return "IP/MAC Notification service [EN 301 192]";
        case DBID_MHP_OBJECT_CSL:       return "MHP Object Carousel";
        case DBID_MHP_MPE:              return "Reserved for MHP Multi Protocol Encapsulation";
        case DBID_EUTELSAT_DATA_PIPE:   return "Eutelsat Data Piping";
        case DBID_EUTELSAT_DATA_STREAM: return "Eutelsat Data Streaming";
        case DBID_SAGEM_IP:             return "SAGEM IP encapsulation in MPEG-2 PES packets";
        case DBID_BARCO_DATA_BRD:       return "BARCO Data Broadcasting";
        case DBID_CIBERCITY_MPE:        return "CyberCity Multiprotocol Encapsulation";
        case DBID_CYBERSAT_MPE:         return "CyberSat Multiprotocol Encapsulation";
        case DBID_TDN:                  return "The Digital Network";
        case DBID_OPENTV_DATA_CSL:      return "OpenTV Data Carousel";
        case DBID_PANASONIC:            return "Panasonic";
        case DBID_KABEL_DEUTSCHLAND:    return "Kabel Deutschland";
        case DBID_TECHNOTREND:          return "TechnoTrend Gorler GmbH";
        case DBID_MEDIAHIGHWAY_SSU:     return "NDS France Technologies system software download";
        case DBID_GUIDE_PLUS:           return "GUIDE Plus+ Rovi Corporation";
        case DBID_ACAP_OBJECT_CSL:      return "ACAP Object Carousel";
        case DBID_MICRONAS:             return "Micronas Download Stream";
        case DBID_POLSAT:               return "Televizja Polsat";
        case DBID_DTG:                  return "UK DTG";
        case DBID_SKYMEDIA:             return "SkyMedia";
        case DBID_INTELLIBYTE:          return "Intellibyte DataBroadcasting";
        case DBID_TELEWEB_DATA_CSL:     return "TeleWeb Data Carousel";
        case DBID_TELEWEB_OBJECT_CSL:   return "TeleWeb Object Carousel";
        case DBID_TELEWEB:              return "TeleWeb";
        case DBID_BBC:                  return "BBC";
        case DBID_ELECTRA:              return "Electra Entertainment Ltd";
        case DBID_BBC_2_3:              return "BBC 2 - 3";
        case DBID_TELETEXT:             return "Teletext";
        case DBID_SKY_DOWNLOAD_1_5:     return "Sky Download Streams 1-5";
        case DBID_ICO:                  return "ICO mim";
        case DBID_CIPLUS_DATA_CSL:      return "CI+ Data Carousel";
        case DBID_HBBTV:                return "HBBTV Carousel";
        case DBID_ROVI_PREMIUM:         return "Premium Content from Rovi Corporation";
        case DBID_MEDIA_GUIDE:          return "Media Guide from Rovi Corporation";
        case DBID_INVIEW:               return "InView Technology Ltd";
        case DBID_BOTECH:               return "Botech Elektronik SAN. ve TIC. LTD.STI.";
        case DBID_SCILLA_PUSHVOD_CSL:   return "Scilla Push-VOD Carousel";
        case DBID_CANAL_PLUS:           return "Canal+";
        case DBID_OIPF_OBJECT_CSL:      return "OIPF Object Carousel - Open IPTV Forum";
        case DBID_4TV:                  return "4TV Data Broadcast";
        case DBID_NOKIA_IP_SSU:         return "Nokia IP based software delivery";
        case DBID_BBG_DATA_CSL:         return "BBG Data Caroussel";
        case DBID_BBG_OBJECT_CSL:       return "BBG Object Caroussel";
        case DBID_BBG:                  return "Bertelsmann Broadband Group";
        default:                        return Format ("0x%04X", int (id));
    }
}
