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
//  This module contains the display routines for class ts::Descriptor
//
//----------------------------------------------------------------------------

#include "tsDescriptor.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsCRC32.h"
#include "tsStringUtils.h"
#include "tsHexa.h"
#include "tsTime.h"
#include "tsCASFamily.h"

using namespace ts;


//----------------------------------------------------------------------------
// Profile of ancillary function to display descriptors.
// The data points at the descriptor payload (after tag/length).
//----------------------------------------------------------------------------

typedef void (*DisplayDescriptorHandler) (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid);


//----------------------------------------------------------------------------
// Dump extraneous bytes after the expected data.
//----------------------------------------------------------------------------

namespace {
    void ExtraData (std::ostream& strm, const void *data, size_t size, int indent)
    {
        if (size > 0) {
            strm << std::string (indent, ' ') << "Extraneous " << size << " bytes:" << std::endl
                 << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
        }
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - unknown descriptor
//----------------------------------------------------------------------------

namespace {
    void DDunknown (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        strm << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors:
// Descriptors containing one name only
// (bouquet_name_descriptor, network_name_descriptor)
//----------------------------------------------------------------------------

namespace {
    void DDname (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        strm << std::string (indent, ' ') << "Name: \"" << Printable (data, size) << "\"" << std::endl;
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - appli_list_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDappli_list (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 8) {
            // Each application name is at most 8 characters long (zero-padded)
            strm << margin << "Appli: \"" << Printable (data, 8) << "\"" << std::endl;
            data += 8; size -= 8;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - application_signalling_descr
//----------------------------------------------------------------------------

namespace {
    void DDappli_signalling (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 3) {
            uint16_t app_type = GetUInt16 (data);
            uint8_t ait_version = data [2];
            data += 3; size -= 3;
            strm << margin << "Application type: " << app_type <<
                Format (" (0x%04X)", int (app_type)) <<
                ", AIT Version: " << int (ait_version) <<
                Format (" (0x%02X)", int (ait_version)) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - appli_startup_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDappli_startup (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 5) {
            const std::string margin (indent, ' ');
            uint8_t req_version = data [0];
            uint16_t startup_id = GetUInt16 (data + 1);
            uint8_t startup_version = data [3];
            uint8_t max_version = data [4];
            data += 5; size -= 5;

            strm << margin << "Startup Id: " << Format ("0x%04X", int (startup_id)) <<
                ", Version: " << int (startup_version) <<
                Format (" (0x%02X)", int (startup_version)) << std::endl <<
                margin << "Required version, min: " << int (req_version) <<
                Format (" (0x%02X)", int (req_version)) <<
                ", max: " << int (max_version) << 
                Format (" (0x%02X)", int (max_version)) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - ca_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDca (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 4) {
            const std::string margin (indent, ' ');

            // Extract common part

            uint16_t sysid = GetUInt16 (data);
            uint16_t pid = GetUInt16 (data + 2) & 0x1FFF;
            CASFamily cas = CASFamilyOf (sysid);
            const char *dtype = tid == TID_CAT ? "EMM" : (tid == TID_PMT ? "ECM" : "CA");
            data += 4; size -= 4;
            strm << margin << "CA System Id: " << Format ("0x%04X", int (sysid)) <<
                " (" << names::CASId (sysid) << ")";

            // Display private data

            if (cas == CAS_VIACCESS) {
                // Viaccsss CA descriptor
                strm << ", " << dtype << " PID: " << pid <<
                    Format (" (0x%04X)", int (pid)) << std::endl;
                // Loop of pseudo-descriptors
                while (size >= 2) {
                    uint8_t tag = *data++;
                    size_t length = *data++;
                    size -= 2;
                    if (length > size)
                        length = size;
                    // Content of pseudo-descriptor
                    if (tag == 0x10 && length == 1) {
                        strm << margin << "ECM exchange id: " << int (*data) <<
                            Format (" (0x%02X)", int (*data)) << std::endl;
                    }
                    else if (tag == 0x13 && length == 1) {
                        strm << margin << "Crypto algorithm id: " << int (*data) << std::endl;
                    }
                    else if (tag == 0x14 && length == 3) {
                        int soid = (int (*data) << 16) | GetUInt16 (data+1);
                        strm << margin << "Service Operator Id: " << soid <<
                            Format (" (0x%06X)", soid) << std::endl;
                    }
                    else {
                        strm << margin << "Subdescriptor tag: " << int (tag) <<
                            Format (" (0x%02X)", int (tag)) <<
                            ", length: " << length << " bytes:" << std::endl <<
                            Hexa (data, length, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent + 2);
                    }
                    // Next pseudo-descriptor
                    data += length; size -= length;
                }
            }

            else {
                // Unknown CA descriptor
                strm << ", " << dtype << " PID: " << pid <<
                    Format (" (0x%04X)", int (pid)) << std::endl;
                if (size > 0) {
                    strm << margin << "Private CA data:" << std::endl <<
                        Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                    data += size; size = 0;
                }
            }
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - CA_identifier_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDca_id (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        while (size >= 2) {
            const std::string margin (indent, ' ');
            uint16_t cas_id = GetUInt16 (data);
            data += 2; size -= 2;
            strm << margin << "CA System Id: " << Format ("0x%04X", int (cas_id))
                 << " (" << names::CASId (cas_id) << ")" << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - cable_delivery_system
//----------------------------------------------------------------------------

namespace {
    void DDcable_delivery (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 11) {
            const std::string margin (indent, ' ');
            uint8_t fec_outer = data[5] & 0x0F;
            uint8_t modulation = data[6];
            uint8_t fec_inner = data[10] & 0x0F;
            std::string freq, srate;
            BCDToString (freq, data, 8, 4);
            BCDToString (srate, data + 7, 7, 3);
            data += 11; size -= 11;

            strm << margin << "Frequency: " << freq << " MHz" << std::endl <<
                margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl <<
                margin << "Modulation: ";
            switch (modulation) {
                case 0:  strm << "not defined"; break;
                case 1:  strm << "16-QAM"; break;
                case 2:  strm << "32-QAM"; break;
                case 3:  strm << "64-QAM"; break;
                case 4:  strm << "128-QAM"; break;
                case 5:  strm << "256-QAM"; break;
                default: strm << "code " << int (modulation) << " (reserved)"; break;
            }
            strm << std::endl << margin << "Outer FEC: ";
            switch (fec_outer) {
                case 0:  strm << "not defined"; break;
                case 1:  strm << "none"; break;
                case 2:  strm << "RS(204/188)"; break;
                default: strm << "code " << int (fec_outer) << " (reserved)"; break;
            }
            strm << ", Inner FEC: ";
            switch (fec_inner) {
                case 0:  strm << "not defined"; break;
                case 1:  strm << "1/2 conv. code rate"; break;
                case 2:  strm << "2/3 conv. code rate"; break;
                case 3:  strm << "3/4 conv. code rate"; break;
                case 4:  strm << "5/6 conv. code rate"; break;
                case 5:  strm << "7/8 conv. code rate"; break;
                case 6:  strm << "8/9 conv. code rate"; break;
                case 15: strm << "none"; break;
                default: strm << "code " << int (fec_inner) << " (reserved)"; break;
            }
            strm << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - channel_map_update_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDchannel_map_upd (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t map = data[0];
            data += 1; size -= 1;
            strm << margin << Format ("Update map: 0x%02X", int (map));
            if (map & 0x80)
                strm << ", complete remapping";
            if (map & 0x40)
                strm << ", channel adding";
            if (map & 0x20)
                strm << ", channel number remapping";
            if (map & 0x01)
                strm << ", no action";
            if ((map & 0xE1) != 0)
                strm << Format (", reserved bits: 0x%02X", int (map & 0xE1));
            strm << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - cmps_record_control_descript
//----------------------------------------------------------------------------

namespace {
    void DDcmps_record_contr (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            uint8_t flags = data[0];
            data += 1; size -= 1;
            strm << std::string (indent, ' ') <<
                "Record allowed: " << YesNo ((flags & 0x80) != 0) <<
                ", Record mode: " << ((flags & 0x40) ? "scrambled" : "clear") <<
                std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - data_version_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDdata_version (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            size_t length = data[0];
            data += 1; size -= 1;
            if (length > size)
                length = size;
            strm << margin << "Data name: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
            while (size >= 3) {
                uint16_t id = GetUInt16 (data);
                uint8_t version = data [2] & 0x1F;
                data += 3; size -= 3;
                strm << margin << "Subtable Id: " << int (id) <<
                    Format (" (0x%04X)", int (id)) <<
                    ", version: " << int (version) <<
                    Format (" (0x%02X)", int (version)) << std::endl;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - ISO_639_language_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDlanguage (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 4) {
            uint8_t type = data[3];
            strm << margin << "Language: " << Printable (data, 3) <<
                ", Type: " << int (type) <<
                " (" << names::AudioType (type) << ")" << std::endl;
            data += 4; size -= 4;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - linkage_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDlinkage (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 7) {
            const std::string margin (indent, ' ');

            // Fixed part
            uint16_t tsid = GetUInt16 (data);
            uint16_t onid = GetUInt16 (data + 2);
            uint16_t servid = GetUInt16 (data + 4);
            uint8_t ltype = data[6];
            data += 7; size -= 7;
            strm << margin << "Transport stream id: " << tsid << Format (" (0x%04X)", int (tsid)) << std::endl
                 << margin << "Original network Id: " << onid << Format (" (0x%04X)", int (onid)) << std::endl
                 << margin << "Service id: " << servid << Format (" (0x%04X)", int (servid)) << std::endl
                 << margin << "Linkage type: " << Format ("0x%02X", int (ltype))
                 << ", " << names::LinkageType (ltype) << std::endl;

            // Variable part
            if (ltype == 0x08 && size >= 1) {
                // Mobile hand-over
                uint8_t hand_over = *data >> 4;
                uint8_t origin = *data & 0x01;
                data += 1; size -= 1;
                const char *name;
                switch (hand_over) {
                    case 0x01: name = "identical service in neighbour country"; break;
                    case 0x02: name = "local variation of same service"; break;
                    case 0x03: name = "associated service"; break;
                    default:   name = "unknown"; break;
                }
                strm << margin << "Hand-over type: " << Format ("0x%02X", int (hand_over)) <<
                    ", " << name << ", Origin: " << (origin ? "SDT" : "NIT") << std::endl;
                if ((hand_over == 0x01 || hand_over == 0x02 || hand_over == 0x03) && size >= 2) {
                    uint16_t nwid = GetUInt16 (data);
                    data += 2; size -= 2;
                    strm << margin << "Network id: " << nwid << Format (" (0x%04X)", int (nwid)) << std::endl;
                }
                if (origin == 0x00 && size >= 2) {
                    uint16_t org_servid = GetUInt16 (data);
                    data += 2; size -= 2;
                    strm << margin << "Original service id: " << org_servid << Format (" (0x%04X)", int (org_servid)) << std::endl;
                }
            }
            else if (ltype == 0x09 && size >= 1) {
                // System Software Update (ETSI TS 102 006)
                uint8_t dlength = data[0];
                data += 1; size -= 1;
                if (dlength > size) {
                    dlength = uint8_t(size);
                }
                while (dlength >= 4) {
                    uint32_t oui = GetUInt32 (data - 1) & 0x00FFFFFF; // 24 bits
                    uint8_t slength = data[3];
                    data += 4; size -= 4; dlength -= 4;
                    const uint8_t* sdata = data;
                    if (slength > dlength) {
                        slength = dlength;
                    }
                    data += slength; size -= slength; dlength -= slength;
                    strm << margin << Format ("OUI: 0x%06X (", int (oui)) << names::OUI (oui) << ")" << std::endl;
                    if (slength > 0) {
                        strm << margin << "Selector data:" << std::endl
                             << Hexa (sdata, slength, hexa::HEXA | hexa::ASCII, indent);
                    }
                }
            }
            else if (ltype == 0x0A && size >= 1) {
                // TS with System Software Update BAT or NIT (ETSI TS 102 006)
                uint8_t ttype = data[0];
                data += 1; size -= 1;
                strm << margin << "SSU table type: ";
                switch (ttype) {
                    case 0x01: strm << "NIT"; break;
                    case 0x02: strm << "BAT"; break;
                    default:   strm << Format ("0x%02x", int (ttype)); break;
                }
                strm << std::endl;
            }

            // Remaining private data
            if (size > 0) {
                strm << margin << "Private data:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                data += size; size = 0;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - loader_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDloader (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 2) {
            const std::string margin (indent, ' ');
            uint8_t op_id = data[0];
            uint8_t manuf = data[1];
            data += 2; size -= 2;
            strm << margin << "Operator Id: " << int (op_id) <<
                Format (" (0x%02X)", int (op_id)) <<
                ", Manufacturer Id: " << int (manuf) <<
                Format (" (0x%02X)", int (manuf)) << std::endl;
            while (size >= 2) {
                uint8_t version = data[0];
                data += 2; size -= 2;
                strm << margin << "Target software version: " << int (version) <<
                    Format (" (0x%02X)", int (version)) << std::endl;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - local_time_offset_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDlocal_time_offset (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 3) {
            const std::string margin (indent, ' ');
            // Country code is a 3-byte string
            strm << margin << "Country code: " << Printable (data, 3) << std::endl;
            data += 3; size -= 3;
            if (size >= 1) {
                uint8_t region_id = *data >> 2;
                uint8_t polarity = *data & 0x01;
                data += 1; size -= 1;
                strm << margin << "Region id: " << int (region_id) <<
                    Format (" (0x%02X)", int (region_id)) <<
                    ", polarity: " << (polarity ? "west" : "east") <<
                    " of Greenwich" << std::endl;
                if (size >= 2) {
                    strm << margin << "Local time offset: " << (polarity ? "-" : "") <<
                        Format ("%02d:%02d", DecodeBCD (data[0]), DecodeBCD (data[1])) << std::endl;
                    data += 2; size -= 2;
                    if (size >= 5) {
                        Time next_change;
                        DecodeMJD (data, 5, next_change);
                        data += 5; size -= 5;
                        strm << margin << "Next change: " <<
                            next_change.format (Time::DATE | Time::TIME) << std::endl;
                        if (size >= 2) {
                            strm << margin << "Next time offset: " << (polarity ? "-" : "") <<
                                Format ("%02d:%02d", DecodeBCD (data[0]), DecodeBCD (data[1])) <<
                                std::endl;
                            data += 2; size -= 2;
                        }
                    }
                }
            }
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - logical_channel_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDlogical_channel (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 4) {
            uint16_t service = GetUInt16 (data);
            uint16_t channel = GetUInt16 (data + 2) & 0x03FF;
            data += 4; size -= 4;
            strm << margin << "Service Id: " << service <<
                Format (" (0x%04X)", int (service)) <<
                ", Channel number: " << channel << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - logical_reference_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDlogical_reference (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        size_t length;
        if (size < 1)
            length = 0;
        else {
            length = data[0];
            data += 1; size -= 1;
            if (length > size)
                length = size;
        }
        strm << margin << "Reference: \"" << Printable (data, length) << "\"" << std::endl;
        data += length; size -= length;
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - MH_logical_reference_descrip
//----------------------------------------------------------------------------

namespace {
    void DDmh_logical_ref (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            // Reference type
            uint8_t reftype = data[0];
            data += 1; size -= 1;
            strm << margin << "Reference type: ";
            switch (reftype) {
                case 0:  strm << "reserved"; break;
                case 1:  strm << "channel logo"; break;
                case 2:  strm << "visu picture"; break;
                default: strm << int (reftype) << " (user-defined)"; break;
            }
            strm << std::endl;
            // Reference name
            size_t length;
            if (size < 1)
                length = 0;
            else {
                length = *data;
                data += 1; size -= 1;
                if (length > size)
                    length = size;
            }
            strm << margin << "Reference: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - private_data_specifier_descr
//----------------------------------------------------------------------------

namespace {
    void DDpriv_data_specif (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 4) {
            uint32_t pds = GetUInt32 (data);
            data += 4; size -= 4;
            strm << std::string (indent, ' ') <<
                Format ("Specifier: 0x%08X", pds) <<
                " (" << names::PrivateDataSpecifier (pds) << ")" << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - record_control_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDrecord_control (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t flags = data[0];
            data += 1; size -= 1;
            strm <<
                margin << "Digital record allowed: " << YesNo ((flags & 0x80) != 0)  << std::endl <<
                margin << "Analog record allowed: " << YesNo ((flags & 0x40) != 0) << std::endl <<
                margin << "Time shifting allowed: " << YesNo ((flags & 0x20) != 0) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - satellite_delivery_system
//----------------------------------------------------------------------------

namespace {
    void DDsat_delivery (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 11) {
            const std::string margin (indent, ' ');
            const uint8_t east = data[6] >> 7;
            const uint8_t polar = (data[6] >> 5) & 0x03;
            const uint8_t roll_off = (data[6] >> 3) & 0x03;
            const uint8_t mod_system = (data[6] >> 2) & 0x01;
            const uint8_t mod_type = data[6] & 0x03;
            const uint8_t fec_inner = data[10] & 0x0F;
            std::string freq, srate, orbital;
            BCDToString (freq, data, 8, 3);
            BCDToString (orbital, data + 4, 4, 3);
            BCDToString (srate, data + 7, 7, 3);
            data += 11; size -= 11;

            strm << margin << "Orbital position: " << orbital <<
                " degree, " << (east ? "east" : "west") << std::endl <<
                margin << "Frequency: " << freq << " GHz" << std::endl <<
                margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl <<
                margin << "Polarization: ";
            switch (polar) {
                case 0:  strm << "linear - horizontal"; break;
                case 1:  strm << "linear - vertical"; break;
                case 2:  strm << "circular - left"; break;
                case 3:  strm << "circular - right"; break;
                default: assert(false);
            }
            strm << std::endl << margin << "Modulation: " << (mod_system == 0 ? "DVB-S" : "DVB-S2") << ", ";
            switch (mod_type) {
                case 0:  strm << "Auto"; break;
                case 1:  strm << "QPSK"; break;
                case 2:  strm << "8PSK"; break;
                case 3:  strm << "16-QAM"; break;
                default: assert(false);
            }
            if (mod_system == 1) {
                switch (roll_off) {
                    case 0:  strm << ", alpha=0.35"; break;
                    case 1:  strm << ", alpha=0.25"; break;
                    case 2:  strm << ", alpha=0.20"; break;
                    case 3:  strm << ", undefined roll-off (3)"; break;
                    default: assert(false);
                }
            }
            strm << std::endl << margin << "Inner FEC: ";
            switch (fec_inner) {
                case 0:  strm << "not defined"; break;
                case 1:  strm << "1/2"; break;
                case 2:  strm << "2/3"; break;
                case 3:  strm << "3/4"; break;
                case 4:  strm << "5/6"; break;
                case 5:  strm << "7/8"; break;
                case 6:  strm << "8/9"; break;
                case 7:  strm << "3/5"; break;
                case 8:  strm << "4/5"; break;
                case 9:  strm << "9/10"; break;
                case 15: strm << "none"; break;
                default: strm << "code " << int (fec_inner) << " (reserved)"; break;
            }
            strm << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - service_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDservice (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 2) {
            const std::string margin (indent, ' ');
            // Service type
            uint8_t stype = *data;
            data += 1; size -= 1;
            strm << margin << Format ("Service type: 0x%02X, ", int (stype)) <<
                names::ServiceType (stype) << std::endl;
            // Provider name
            size_t plength;
            if (size < 1)
                plength = 0;
            else {
                plength = *data;
                data += 1; size -= 1;
                if (plength > size)
                    plength = size;
            }
            const uint8_t* provider = data;
            data += plength; size -= plength;
            // Service name
            size_t slength;
            if (size < 1)
                slength = 0;
            else {
                slength = *data;
                data += 1; size -= 1;
                if (slength > size)
                    slength = size;
            }
            const uint8_t* service = data;
            data += slength; size -= slength;
            // Display names
            strm << margin << "Service: \"" << Printable (service, slength) <<
                "\", Provider: \"" << Printable (provider, plength) << "\"" <<
                std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - service_list_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDservice_list (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 3) {
            uint16_t sid = GetUInt16 (data);
            uint8_t stype = data[2];
            data += 3; size -= 3;
            strm << margin << Format ("Service id: %d (0x%04X), Type: 0x%02X, ", int (sid), int (sid), int (stype))
                 << names::ServiceType (stype) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - short_service_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDshort_service (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 3) {
            const std::string margin (indent, ' ');
            // Language
            strm << margin << "Language: " << Printable (data, 3) << std::endl;
            data += 3; size -= 3;
            // Description
            size_t length;
            if (size < 1) {
                length = 0;
            }
            else {
                length = data[0];
                data += 1; size -= 1;
                if (length > size) {
                    length = size;
                }
            }
            strm << margin << "Description: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - STD_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDstd (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            uint8_t leak = data[0] & 0x01;
            data += 1; size -= 1;
            strm << std::string (indent, ' ') << "Link valid flag: " << int (leak)
                 << (leak != 0 ? " (leak)" : " (vbv_delay)") << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - stream_identifier_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDstream_id (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            uint8_t id = data[0];
            data += 1; size -= 1;
            strm << Format ("%*sComponent tag: %d (0x%02X)", indent, "", int (id), int (id)) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - subtitling_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDsubtitling (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 8) {
            uint8_t type = data[3];
            uint16_t comp_page = GetUInt16 (data + 4);
            uint16_t ancil_page = GetUInt16 (data + 6);
            strm << margin << "Language: " << Printable (data, 3) <<
                ", Type: " << int (type) <<
                Format (" (0x%02X)", int (type)) << std::endl <<
                margin << "Type: " << names::SubtitlingType (type) << std::endl <<
                margin << "Composition page: " << comp_page <<
                Format (" (0x%04X)", int (comp_page)) <<
                ", Ancillary page: " << ancil_page <<
                Format (" (0x%04X)", int (ancil_page)) << std::endl;
            data += 8; size -= 8;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - teletext_descriptor
// (also valid for VBI_teletext_descriptor which uses the same syntax)
//----------------------------------------------------------------------------

namespace {
    void DDteletext (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 5) {
            uint8_t type = data[3] >> 3;
            uint8_t mag = data[3] & 0x07;
            uint8_t page = data[4];
            strm << margin << "Language: " << Printable (data, 3) <<
                ", Type: " << int (type) <<
                Format (" (0x%02X)", int (type)) << std::endl <<
                margin << "Type: " << names::TeletextType (type) << std::endl <<
                margin << "Magazine number: " << int (mag) <<
                ", Page number: " << int (page) << std::endl;
            data += 5; size -= 5;
        }
        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display descriptors - terrestrial_delivery_system
//----------------------------------------------------------------------------

namespace {
    void DDterrest_delivery (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 11) {
            const std::string margin(indent, ' ');
            uint32_t cfreq = GetUInt32(data);
            uint8_t bwidth = data[4] >> 5;
            uint8_t prio = (data[4] >> 4) & 0x01;
            uint8_t tslice = (data[4] >> 3) & 0x01;
            uint8_t mpe_fec = (data[4] >> 2) & 0x01;
            uint8_t constel = data[5] >> 6;
            uint8_t hierarchy = (data[5] >> 3) & 0x07;
            uint8_t rate_hp = data[5] & 0x07;
            uint8_t rate_lp = data[6] >> 5;
            uint8_t guard = (data[6] >> 3) & 0x03;
            uint8_t transm = (data[6] >> 1) & 0x03;
            bool other_freq = (data[6] & 0x01) != 0;
            data += 11; size -= 11;

            strm << margin << "Centre frequency: " << 
                Decimal (10 * uint64_t (cfreq)) << " Hz, Bandwidth: ";
            switch (bwidth) {
                case 0:  strm << "8 MHz"; break;
                case 1:  strm << "7 MHz"; break;
                case 2:  strm << "6 MHz"; break;
                case 3:  strm << "5 MHz"; break;
                default: strm << "code " << int (bwidth) << " (reserved)"; break;
            }
            strm << std::endl <<
                margin << "Priority: " << (prio ? "high" : "low") <<
                ", Time slicing: " << (tslice ? "unused" : "used") <<
                ", MPE-FEC: " << (mpe_fec ? "unused" : "used") << std::endl <<
                margin << "Constellation pattern: ";
            switch (constel) {
                case 0:  strm << "QPSK"; break;
                case 1:  strm << "16-QAM"; break;
                case 2:  strm << "64-QAM"; break;
                case 3:  strm << "reserved"; break;
                default: assert(false);
            }
            strm << std::endl << margin << "Hierarchy: ";
            assert(hierarchy < 8);
            switch (hierarchy & 0x03) {
                case 0:  strm << "non-hierarchical"; break;
                case 1:  strm << "alpha = 1"; break;
                case 2:  strm << "alpha = 2"; break;
                case 3:  strm << "alpha = 4"; break;
                default: assert(false);
            }
            strm << ", " << ((hierarchy & 0x04) ? "in-depth" : "native")
                 << " interleaver" << std::endl
                 << margin << "Code rate: high prio: ";
            switch (rate_hp) {
                case 0:  strm << "1/2"; break;
                case 1:  strm << "2/3"; break;
                case 2:  strm << "3/4"; break;
                case 3:  strm << "5/6"; break;
                case 4:  strm << "7/8"; break;
                default: strm << "code " << int (rate_hp) << " (reserved)"; break;
            }
            strm << ", low prio: ";
            switch (rate_lp) {
                case 0:  strm << "1/2"; break;
                case 1:  strm << "2/3"; break;
                case 2:  strm << "3/4"; break;
                case 3:  strm << "5/6"; break;
                case 4:  strm << "7/8"; break;
                default: strm << "code " << int (rate_lp) << " (reserved)"; break;
            }
            strm << std::endl << margin << "Guard interval: ";
            switch (guard) {
                case 0:  strm << "1/32"; break;
                case 1:  strm << "1/16"; break;
                case 2:  strm << "1/8"; break;
                case 3:  strm << "1/4"; break;
                default: assert(false);
            }
            strm << std::endl << margin << "OFDM transmission mode: ";
            switch (transm) {
                case 0:  strm << "2k"; break;
                case 1:  strm << "8k"; break;
                case 2:  strm << "4k"; break;
                case 3:  strm << "reserved"; break;
                default: assert(false);
            }
            strm << ", other frequencies: " << YesNo (other_freq) << std::endl;
        }
        ExtraData(strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - aac_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDaac (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t prof_lev = data[0];
            data++; size--;
            strm << margin << Format ("Profile and level: 0x%02X", int (prof_lev)) << std::endl;
            if (size >= 1) {
                uint8_t flags = data [0];
                data++; size--;
                if ((flags & 0x80) && size >= 1) { // AAC_type
                    uint8_t type = data [0];
                    data++; size--;
                    strm << margin << Format ("AAC type: 0x%02X", int (type)) << std::endl;
                }
                if (size > 0) {
                    strm << margin << "Additional information:" << std::endl
                         << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                    data += size; size = 0;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - component_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDcomponent (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 6) {
            const std::string margin (indent, ' ');
            uint16_t type = GetUInt16 (data) & 0x0FFF;
            uint8_t tag = data[2];
            strm << margin << Format ("Content/type: 0x%04X", int (type))
                 << " (" << names::ComponentType (type) << ")" << std::endl
                 << margin << Format ("Component tag: %d (0x%02X)", int (tag), int (tag)) << std::endl
                 << margin << "Language: " << Printable (data + 3, 3) << std::endl;
            data += 6; size -= 6;
            if (size > 0) {
                strm << margin << "Description: \"" << Printable (data, size) << "\"" << std::endl;
            }
            data += size; size = 0;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - content_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDcontent (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 2) {
            uint8_t content = data[0];
            uint8_t user = data[1];
            data += 2; size -= 2;
            strm << margin << Format ("Content: 0x%02X", int (content))
                 << ", " << names::Content (content)
                 << Format (" / User: 0x%02X", int (user)) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - country_availability_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDcountry_avail (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            bool available = (data[0] & 0x80) != 0;
            data += 1; size -= 1;
            strm << margin << "Available: " << YesNo (available) << std::endl;
            while (size >= 3) {
                strm << margin << "Country code: \"" << Printable (data, 3) << "\"" << std::endl;
                data += 3; size -= 3;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Display the content of data broadcast selector bytes
//----------------------------------------------------------------------------

namespace {
    void data_broadcast_selectors (std::ostream& strm, const uint8_t* data, size_t size, int indent, uint16_t dbid)
    {
        const std::string margin (indent, ' ');
        // Interpretation depends in the data broadcast id.
        if (dbid == 0x000A && size >= 1) {
            // System Software Update (ETSI TS 102 006)
            // Id selector is a system_software_update_info structure.
            // OUI_data_length:
            uint8_t dlength = data[0];
            data += 1; size -= 1;
            if (dlength > size) {
                dlength = uint8_t(size);
            }
            // OUI loop:
            while (dlength >= 6) {
                // Get fixed part (6 bytes)
                uint32_t oui = GetUInt32 (data - 1) & 0x00FFFFFF; // 24 bits
                uint8_t upd_type = data[3] & 0x0F;
                uint8_t upd_flag = (data[4] >> 5) & 0x01;
                uint8_t upd_version = data[4] & 0x1F;
                uint8_t slength = data[5];
                data += 6; size -= 6; dlength -= 6;
                // Get variable-length selector
                const uint8_t* sdata = data;
                if (slength > dlength) {
                    slength = dlength;
                }
                data += slength; size -= slength; dlength -= slength;
                // Display
                strm << margin << Format ("OUI: 0x%06X (", int (oui)) << names::OUI (oui) << ")" << std::endl
                     << margin << Format ("  Update type: 0x%02X (", int (upd_type));
                switch (upd_type) {
                    case 0x00: strm << "proprietary update solution"; break;
                    case 0x01: strm << "standard update carousel (no notification) via broadcast"; break;
                    case 0x02: strm << "system software update with UNT via broadcast"; break;
                    case 0x03: strm << "system software update using return channel with UNT"; break;
                    default:   strm << "reserved"; break;
                }
                strm << ")" << std::endl << margin << "  Update version: ";
                if (upd_flag == 0) {
                    strm << "none";
                }
                else {
                    strm << Format ("%d (0x%02X)", int (upd_version), int (upd_version));
                }
                strm << std::endl;
                if (slength > 0) {
                    strm << margin << "  Selector data:" << std::endl
                         << Hexa (sdata, slength, hexa::HEXA | hexa::ASCII, indent + 2);
                }
            }
            // Extraneous data in OUI_loop:
            if (dlength > 0) {
                strm << margin << "Extraneous data in OUI loop:" << std::endl
                     << Hexa (data, dlength, hexa::HEXA | hexa::ASCII, indent);
                data += dlength; size -= dlength;
            }
            // Private data
            if (size > 0) {
                strm << margin << "Private data:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII, indent);
                data += size; size = 0;
            }
        }
        else if (size > 0) {
            // Generic "id selector".
            strm << margin << "Data Broadcast Id selector:" << std::endl
                 << Hexa (data, size, hexa::HEXA | hexa::ASCII, indent);
            data += size; size = 0;
        }
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - data_broadcast_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDdata_broadcast (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 4) {
            const std::string margin (indent, ' ');
            const uint16_t dbid = GetUInt16 (data);
            const uint8_t ctag = data[2];
            size_t slength = data[3];
            data += 4; size -= 4;
            if (slength > size) {
                slength = size;
            }
            strm << margin << Format ("Data broadcast id: %d (0x%04X), ", int (dbid), int (dbid))
                 << names::DataBroadcastId (dbid) << std::endl
                 << margin << Format ("Component tag: %d (0x%02X), ", int (ctag), int (ctag))
                 << std::endl;
            data_broadcast_selectors (strm, data, slength, indent, dbid);
            data += slength; size -= slength;
            if (size >= 3) {
                strm << margin << "Language: " << Printable (data, 3) << std::endl;
                data += 3; size -= 3;
                size_t length = 0;
                if (size >= 1) {
                    length = data[0];
                    data += 1; size -= 1;
                    if (length > size) {
                        length = size;
                    }
                }
                strm << margin << "Description: \"" << Printable (data, length) << "\"" << std::endl;
                data += length; size -= length;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - data_broadcast_id_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDdata_broadcast_id (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 2) {
            const std::string margin (indent, ' ');
            uint16_t id = GetUInt16 (data);
            data += 2; size -= 2;
            strm << margin << Format ("Data broadcast id: %d (0x%04X), ", int (id), int (id))
                 << names::DataBroadcastId (id) << std::endl;
            // The rest of the descriptor is the "id selector".
            data_broadcast_selectors (strm, data, size, indent, id);
            data += size; size = 0;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - dts_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDdts (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 5) {
            const std::string margin (indent, ' ');
            uint8_t sample_rate_code = (data[0] >> 4) & 0x0F;
            uint8_t bit_rate_code = (GetUInt16 (data) >> 6) & 0x3F;
            uint8_t nblks = (GetUInt16 (data + 1) >> 7) & 0x7F;
            uint16_t fsize = (GetUInt16 (data + 2) >> 1) & 0x3FFF;
            uint8_t surround_mode = (GetUInt16 (data + 3) >> 3) & 0x3F;
            bool lfe_flag = ((data[4] >> 2) & 0x01) != 0;
            uint8_t extended_surround_flag = data[4] & 0x03;
            data += 5; size -= 5;
 
            strm << margin << "Sample rate code: " << names::DTSSampleRateCode (sample_rate_code) << std::endl
                 << margin << "Bit rate code: " << names::DTSBitRateCode (bit_rate_code) << std::endl
                 << margin << "NBLKS: " << int (nblks) << std::endl
                 << margin << "FSIZE: " << int (fsize) << std::endl
                 << margin << "Surround mode: " << names::DTSSurroundMode (surround_mode) << std::endl
                 << margin << "LFE (Low Frequency Effect) audio channel: " << OnOff (lfe_flag) << std::endl
                 << margin << "Extended surround flag: " << names::DTSExtendedSurroundMode (extended_surround_flag) << std::endl;

            if (size > 0) {
                strm << margin << "Additional information:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                data += size; size = 0;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - eacem_stream_identifier_desc
//----------------------------------------------------------------------------

namespace {
    void DDeacem_stream_id (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t version = data[0];
            data += 1; size -= 1;
            strm << margin << "Version: " << int (version) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - ac3_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDac3 (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t flags = data[0];
            data++; size--;
            if ((flags & 0x80) && size >= 1) { // component_type
                uint8_t type = data[0];
                data++; size--;
                strm << margin << Format ("Component type: 0x%02X", int (type))
                     << " (" << names::AC3ComponentType (type) << ")" << std::endl;
            }
            if ((flags & 0x40) && size >= 1) { // bsid
                uint8_t bsid = data[0];
                data++; size--;
                strm << margin << Format ("AC-3 coding version: %d (0x%02X)", int (bsid), int (bsid)) << std::endl;
            }
            if ((flags & 0x20) && size >= 1) { // mainid
                uint8_t mainid = data[0];
                data++; size--;
                strm << margin << Format ("Main audio service id: %d (0x%02X)", int (mainid), int (mainid)) << std::endl;
            }
            if ((flags & 0x10) && size >= 1) { // asvc
                uint8_t asvc = data[0];
                data++; size--;
                strm << margin << Format ("Associated to: 0x%02X", int (asvc)) << std::endl;
            }
            if (size > 0) {
                strm << margin << "Additional information:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                data += size; size = 0;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - enhanced_ac3_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDenhanced_ac3 (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t flags = data[0];
            data++; size--;

            if ((flags & 0x80) && size >= 1) { // component_type
                uint8_t type = data [0];
                data++; size--;
                strm << margin << Format ("Component type: 0x%02X ", int (type))
                     << " (" << names::AC3ComponentType (type) << ")" << std::endl;
            }
            if ((flags & 0x40) && size >= 1) { // bsid
                uint8_t bsid = data[0];
                data++; size--;
                strm << margin << Format ("AC-3 coding version: %d (0x%02X)", int (bsid), int (bsid)) << std::endl;
            }
            if ((flags & 0x20) && size >= 1) { // mainid
                uint8_t mainid = data[0];
                data++; size--;
                strm << margin << Format ("Main audio service id: %d (0x%02X)", int (mainid), int (mainid)) << std::endl;
            }
            if ((flags & 0x10) && size >= 1) { // asvc
                uint8_t asvc = data[0];
                data++; size--;
                strm << margin << Format ("Associated to: 0x%02X", int (asvc)) << std::endl;
            }
            if (flags & 0x08) {
                strm << margin << "Substream 0: Mixing control metadata" << std::endl;
            }
            if ((flags & 0x04) && size >= 1) { // substream1
                uint8_t type = data [0];
                data++; size--;
                strm << margin << Format ("Substream 1: 0x%02X", int (type))
                     << " (" << names::AC3ComponentType (type) << ")" << std::endl;
            }
            if ((flags & 0x02) && size >= 1) { // substream2
                uint8_t type = data [0];
                data++; size--;
                strm << margin << Format ("Substream 2: 0x%02X", int (type))
                     << " (" << names::AC3ComponentType (type) << ")" << std::endl;
            }
            if ((flags & 0x01) && size >= 1) { // substream3
                uint8_t type = data [0];
                data++; size--;
                strm << margin << Format ("Substream 3: 0x%02X", int (type))
                     << " (" << names::AC3ComponentType (type) << ")" << std::endl;
            }
            if (size > 0) {
                strm << margin << "Additional information:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                data += size; size = 0;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - extended_event_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDextended_event (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 5) {
            const std::string margin (indent, ' ');
            uint8_t desc_num = data[0];
            std::string lang (Printable (data + 1, 3));
            size_t length = data[4];
            data += 5; size -= 5;
            if (length > size) {
                length = size;
            }
            strm << margin << "Descriptor number: " << int ((desc_num >> 4) & 0x0F)
                 << ", last: " << int (desc_num & 0x0F) << std::endl
                 << margin << "Language: " << lang << std::endl;
            while (length > 0) {
                size_t len = data[0];
                data++; size--; length--;
                if (len > length) {
                    len = length;
                }
                strm << margin << "\"" << Printable (data, len) << "\" : \"";
                data += len; size -= len; length -= len;
                if (length == 0) {
                    len = 0;
                }
                else {
                    len = data[0];
                    data++; size--; length--;
                    if (len > length) {
                        len = length;
                    }
                }
                strm << Printable (data, len) << "\"" << std::endl;
                data += len; size -= len; length -= len;
            }
            if (size < 1) {
                length = 0;
            }
            else {
                length = data[0];
                data++; size--;
                if (length > size) {
                    length = size;
                }
            }
            strm << margin << "Description: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - logical_channel_number_descript
//----------------------------------------------------------------------------

namespace {
    void DDlogical_chan_num (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        uint16_t service, channel;
        uint8_t visible;
 
        while (size >= 4) {
            service = GetUInt16 (data);
            visible = (data[2] >> 7) & 0x01;
            channel = GetUInt16 (data + 2) & 0x03FF;
            data += 4; size -= 4;
            strm << margin
                 << Format ("Service Id: %5d (0x%04X), Visible: %1d, Channel number: %3d",
                            int (service), int (service), int (visible), int (channel))
                 << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - eutelsat_channel_number_descript
//----------------------------------------------------------------------------

namespace {
    void DDeutelsat_chan_num (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
 
        while (size >= 8) {
            const uint16_t onid = GetUInt16 (data);
            const uint16_t tsid = GetUInt16 (data + 2);
            const uint16_t svid = GetUInt16 (data + 4);
            const uint16_t chan = (GetUInt16 (data + 6) >> 4) & 0x0FFF;
            data += 8; size -= 8;
            strm << margin
                 << Format ("Orig Net Id: %5d (0x%04X), TS Id: %5d (0x%04X), Service Id: %5d (0x%04X), Channel number: %3d",
                            int (onid), int (onid),
                            int (tsid), int (tsid),
                            int (svid), int (svid),
                            int (chan))
                 << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - message_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDmessage (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 4) {
            const std::string margin (indent, ' ');
            const uint8_t message_id = data[0];
            const std::string lang (Printable (data + 1, 3));
            data += 4; size -= 4;
            strm << margin << "Message id: " << int (message_id)
                 << ", language: " << lang << std::endl
                 << margin << "Message: \"" << Printable (data, size) << "\"" << std::endl;
            data += size; size -= size;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - preferred_name_identifier_desc
//----------------------------------------------------------------------------

namespace {
    void DDpreferred_name_id (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            uint8_t id = data[0];
            data += 1; size -= 1;
            strm << margin << "Name identifier: " << int (id) << std::endl;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - parental_rating_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDparental_rating (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 4) {
            uint8_t rating = data[3];
            strm << margin << "Country code: " << Printable (data, 3)
                 << Format (", rating: 0x%02d ", int (rating));
            if (rating == 0) {
                strm << "(undefined)";
            }
            else if (rating <= 0x0F) {
                strm << "(min. " << int (rating + 3) << " years)";
            }
            else {
                strm << "(broadcaster-defined)";
            }
            strm << std::endl;
            data += 4; size -= 4;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - preferred_name_list_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDpreferred_name_list (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 4) {
            std::string lang (Printable (data, 3));
            uint8_t count = data[3];
            data += 4; size -= 4;

            strm << margin << "Language: " << lang << ", name count: " << int (count) << std::endl;
            while (count-- > 0 && size >= 2) {
                uint8_t id = data[0];
                size_t length = data[1];
                data += 2; size -= 2;
                if (length > size) {
                    length = size;
                }
                strm << margin << "Id: " << int (id) << ", Name: \"" << Printable (data, length) << "\"" << std::endl;
                data += length; size -= length;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - short_event_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDshort_event (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 4) {
            const std::string margin (indent, ' ');
            const std::string lang (Printable (data, 3));
            size_t length = data[3];
            data += 4; size -= 4;
            if (length > size) {
                length = size;
            }
            strm << margin << "Language: " << lang << std::endl
                 << margin << "Event name: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
            if (size < 1) {
                length = 0;
            }
            else {
                length = *data;
                data += 1; size -= 1;
                if (length > size) {
                    length = size;
                }
            }
            strm << margin << "Description: \"" << Printable (data, length) << "\"" << std::endl;
            data += length; size -= length;
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - supplementary_audio_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDsuppl_audio (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            const std::string margin (indent, ' ');
            const uint8_t mix_type = (data[0] >> 7) & 0x01;
            const uint8_t editorial = (data[0] >> 2) & 0x1F;
            const uint8_t lang_present = data[0] & 0x01;
            data++; size--;
            strm << margin << "Mix type: ";
            switch (mix_type) {
                case 0:  strm << "supplementary stream"; break;
                case 1:  strm << "complete and independent stream"; break;
                default: assert(false);
            }
            strm << std::endl << margin << "Editorial classification: ";
            switch (editorial) {
                case 0x00: strm << "main audio"; break;
                case 0x01: strm << "audio description for the visually impaired"; break;
                case 0x02: strm << "clean audio for the hearing impaired"; break;
                case 0x03: strm << "spoken subtitles for the visually impaired"; break;
                default:   strm << Format ("reserved value 0x%02X", editorial); break;
            }
            strm << std::endl;
            if (lang_present && size >= 3) {
                strm << margin << "Language: " << Printable (data, 3) << std::endl;
                data += 3; size -= 3;
            }
            if (size > 0) {
                strm << margin << "Private data:" << std::endl
                     << Hexa (data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
                data += size; size = 0;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - VBI_data_descriptor
//----------------------------------------------------------------------------

namespace {
    void DDvbi_data (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        const std::string margin (indent, ' ');
        while (size >= 2) {
            const uint8_t data_id = data[0];
            size_t length = data[1];
            data += 2; size -= 2;
            if (length > size) {
                length = size;
            }
            strm << margin << Format("Data service id: %d (0x%02X)", int (data_id), int (data_id));
            switch (data_id) {
                case 1:  strm << ", EBU teletext"; break;
                case 2:  strm << ", Inverted teletext"; break;
                case 4:  strm << ", VPS, Video Programming System"; break;
                case 5:  strm << ", WSS, Wide Screen Signaling"; break;
                case 6:  strm << ", Closed captioning"; break;
                case 7:  strm << ", Monochrone 4:2:2 samples"; break;
                default: strm << ", data id " << int(data_id) << " (reserved)"; break;
            }
            strm << std::endl;
            if (data_id == 1 || data_id == 2 || (data_id >= 4 && data_id <= 7)) {
                while (length > 0) {
                    const uint8_t field_parity = (data[0] >> 5) & 0x01;
                    const uint8_t line_offset = data[0] & 0x1F;
                    data++; size--; length--;
                    strm << margin << "Field parity: " << int (field_parity) << ", line offset: " << int (line_offset) << std::endl;
                }
            }
            else if (length > 0) {
                strm << margin << "Associated data:" << std::endl
                     << Hexa (data, length, hexa::HEXA | hexa::ASCII, indent);
                data += length; size -= length;
            }
        }
        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display descriptors - extension descriptor
//----------------------------------------------------------------------------

namespace {
    void DDextension (std::ostream& strm, const uint8_t* data, size_t size, int indent, ts::TID tid)
    {
        if (size >= 1) {
            // Get extended descriptor tag
            const std::string margin (indent, ' ');
            const uint8_t edid = *data++;
            size--;

            // Display extended descriptor header
            strm << margin << "Extended descriptor: " << names::EDID (edid, 0)
                 << Format (", Tag %d (0x%02X)", int (edid), int (edid)) << std::endl;

            // Determine display handler for the descriptor.
            DisplayDescriptorHandler handler = DDunknown;
            switch (edid) {
                case EDID_MESSAGE:     handler = DDmessage; break;
                case EDID_SUPPL_AUDIO: handler = DDsuppl_audio; break;
                default: break;
            }

            // Display content of extended descriptor
            handler (strm, data, size, indent, tid);
        }
    }
}


//----------------------------------------------------------------------------
// This static routine displays a list of descriptors from a memory area
//----------------------------------------------------------------------------

std::ostream& ts::Descriptor::Display(std::ostream& strm,
                                      const void* data,
                                      size_t size,
                                      int indent,
                                      TID tid,
                                      PDS pds)
{
    const std::string margin(indent, ' ');
    const uint8_t* desc_start = reinterpret_cast<const uint8_t*>(data);
    size_t desc_index = 0;

    // Loop across all descriptors

    while (size >= 2) {  // descriptor header size

        // Get descriptor header

        uint8_t desc_tag = *desc_start++;
        size_t desc_length = *desc_start++;
        size -= 2;

        if (desc_length > size) {
            strm << margin << "- Invalid descriptor length: " << desc_length
                 << " (" << size << " bytes allocated)" << std::endl;
            break;
        }

        // Display descriptor header

        strm << margin << "- Descriptor " << desc_index++
             << ": " << names::DID (desc_tag, pds) << ", Tag " << int (desc_tag)
             << Format (" (0x%02X), ", int (desc_tag)) << desc_length << " bytes" << std::endl;

        // If the descriptor contains a private_data_specifier, keep it
        // to establish a private context.

        if (desc_tag == DID_PRIV_DATA_SPECIF && desc_length >= 4) {
            pds = GetUInt32 (desc_start);
        }

        // Move to next descriptor for next iteration

        const uint8_t* desc (desc_start);
        desc_start += desc_length;
        size -= desc_length;

        // Determine display handler for the descriptor.
        // Private descriptors depend on the private_data_specifier.

        DisplayDescriptorHandler handler = DDunknown;

        switch (desc_tag) {
            case DID_AAC:               handler = DDaac; break;
            case DID_AC3:               handler = DDac3; break;
            case DID_APPLI_SIGNALLING:  handler = DDappli_signalling; break;
            case DID_BOUQUET_NAME:      handler = DDname; break;
            case DID_CA:                handler = DDca; break;
            case DID_CA_ID:             handler = DDca_id; break;
            case DID_CABLE_DELIVERY:    handler = DDcable_delivery; break;
            case DID_COMPONENT:         handler = DDcomponent; break;
            case DID_CONTENT:           handler = DDcontent; break;
            case DID_COUNTRY_AVAIL:     handler = DDcountry_avail; break;
            case DID_DATA_BROADCAST:    handler = DDdata_broadcast; break;
            case DID_DATA_BROADCAST_ID: handler = DDdata_broadcast_id; break;
            case DID_DTS:               handler = DDdts; break;
            case DID_ENHANCED_AC3:      handler = DDenhanced_ac3; break;
            case DID_EXTENDED_EVENT:    handler = DDextended_event; break;
            case DID_EXTENSION:         handler = DDextension; break;
            case DID_LANGUAGE:          handler = DDlanguage; break;
            case DID_LINKAGE:           handler = DDlinkage; break;
            case DID_LOCAL_TIME_OFFSET: handler = DDlocal_time_offset; break;
            case DID_NETWORK_NAME:      handler = DDname; break;
            case DID_PARENTAL_RATING:   handler = DDparental_rating; break;
            case DID_PRIV_DATA_SPECIF:  handler = DDpriv_data_specif; break;
            case DID_SAT_DELIVERY:      handler = DDsat_delivery; break;
            case DID_SERVICE:           handler = DDservice; break;
            case DID_SERVICE_LIST:      handler = DDservice_list; break;
            case DID_SHORT_EVENT:       handler = DDshort_event; break;
            case DID_STD:               handler = DDstd; break;
            case DID_STREAM_ID:         handler = DDstream_id; break;
            case DID_SUBTITLING:        handler = DDsubtitling; break;
            case DID_TELETEXT:          handler = DDteletext; break;
            case DID_TERREST_DELIVERY:  handler = DDterrest_delivery; break;
            case DID_VBI_DATA:          handler = DDvbi_data; break;
            case DID_VBI_TELETEXT:      handler = DDteletext; break;
            default:                    handler = DDunknown; break;
        }

        switch (pds) {
            case PDS_CANALPLUS:
            case 0:
                // These descriptor tags are private. They should occur only after
                // a "private data specifier". However, due to a bug in the DBC
                // Supervisor, some tables (essentially PMT) do not have a Canal+
                // PDS before private descriptors. To handle this, if a private
                // descriptor is found without PDS (pds == 0), we assume the
                // Canal+ PDS.
                switch (desc_tag) {
                    case DID_APPLI_LIST:          handler = DDappli_list; break;
                    case DID_APPLI_STARTUP:       handler = DDappli_startup; break;
                    case DID_CMPS_RECORD_CONTROL: handler = DDcmps_record_contr; break;
                    case DID_DATA_VERSION:        handler = DDdata_version; break;
                    case DID_CHANNEL_MAP_UPDATE:  handler = DDchannel_map_upd; break;
                    case DID_LOADER:              handler = DDloader; break;
                    case DID_LOGICAL_CHANNEL:     handler = DDlogical_channel; break;
                    case DID_LOGICAL_REFERENCE:   handler = DDlogical_reference; break;
                    case DID_MH_LOGICAL_REF:      handler = DDmh_logical_ref; break;
                    case DID_RECORD_CONTROL:      handler = DDrecord_control; break;
                    case DID_SHORT_SERVICE:       handler = DDshort_service; break;
                    default: break;
                }
                break;
    
            case PDS_TPS:
                // Incorrect use of TPS private data, TPS broadcasters should
                // use EACEM/EICTA PDS instead.
            case PDS_EICTA:
                // European Association of Consumer Electronics Manufacturers.
                // Descriptors are defined in IEC/CENELEC 62216-1
                // "Baseline terrestrial receiver specification"
                switch (desc_tag) {
                    case DID_LOGICAL_CHANNEL_NUM: handler = DDlogical_chan_num; break;
                    case DID_PREF_NAME_LIST:      handler = DDpreferred_name_list; break;
                    case DID_PREF_NAME_ID:        handler = DDpreferred_name_id; break;
                    case DID_EACEM_STREAM_ID:     handler = DDeacem_stream_id; break;
                    case DID_HD_SIMULCAST_LCN:    handler = DDlogical_chan_num; break;
                    default: break;
                }
                break;

            case PDS_EUTELSAT:
                // Eutelsat operator, including Fransat
                switch (desc_tag) {
                    case DID_EUTELSAT_CHAN_NUM: handler = DDeutelsat_chan_num; break;
                    default: break;
                }
                break;

            default:
                break;
        }

        // Display content of descriptor
        handler (strm, desc, desc_length, indent + 2, tid);
    }

    // Report extraneous bytes

    ExtraData (strm, desc_start, size, indent);

    return strm;
}


//----------------------------------------------------------------------------
// Display the descriptor on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::Descriptor::display (std::ostream& strm, int indent, ts::TID tid, PDS pds) const
{
    if (isValid()) {
        Display (strm, content(), size(), indent, tid, pds);
    }
    return strm;
}
