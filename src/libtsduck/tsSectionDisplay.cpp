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
//  This module contains the display routines for class ts::Section
//
//----------------------------------------------------------------------------

#include "tsSection.h"
#include "tsDescriptor.h"
#include "tsStringUtils.h"
#include "tsFormat.h"
#include "tsDecimal.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsCRC32.h"
#include "tsHexa.h"
#include "tsTime.h"


using namespace ts;


//----------------------------------------------------------------------------
// Profile of ancillary function to display sections.
//----------------------------------------------------------------------------

typedef void (*DisplaySectionHandler) (std::ostream& strm, const Section& section, int indent);


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
// Ancillary function to display table - unknown section
//----------------------------------------------------------------------------

namespace {
    void DSunknown (std::ostream& strm, const Section& section, int indent)
    {
        if (section.isLongSection()) {
            strm << std::string (indent, ' ')
                 << "TIDext: " << section.tableIdExtension()
                 << Format (" (0x%04X)", int (section.tableIdExtension()))
                 << std::endl;
        }
        strm << Hexa (section.payload(), section.payloadSize(), hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display table - Tables the content of which
// if only a list of descriptors and without TIDext significance
//----------------------------------------------------------------------------

namespace {
    void DSgeneric (std::ostream& strm, const Section& section, int indent)
    {
        Descriptor::Display (strm, section.payload(), section.payloadSize(), indent, section.tableId());
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display table - PAT
//----------------------------------------------------------------------------

namespace {
    void DSpat (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();
        uint16_t ts_id = section.tableIdExtension();

        strm << margin << Format ("TS id:   %5d (0x%04X)", int (ts_id), int (ts_id)) << std::endl;

        // Loop through all program / pid pairs

        while (size >= 4) {
            uint16_t program = GetUInt16 (data);
            uint16_t pid = GetUInt16 (data + 2) & 0x1FFF;
            data += 4; size -= 4;
            strm << margin
                 << Format ("%s %5d (0x%04X)  PID: %4d (0x%04X)",
                            program == 0 ? "NIT:    " : "Program:",
                            int (program), int (program),
                            int (pid), int (pid))
                 << std::endl;
        }

        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Ancillary function to display table - PMT
//----------------------------------------------------------------------------

namespace {
    void DSpmt (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        if (size >= 4) {
            // Fixed part
            ts::PID pid = GetUInt16 (data) & 0x1FFF;
            size_t info_length = GetUInt16 (data + 2) & 0x0FFF;
            data += 4; size -= 4;
            if (info_length > size) {
                info_length = size;
            }
            strm << margin << "Program: " << section.tableIdExtension() <<
                Format (" (0x%04X)", int (section.tableIdExtension())) <<
                ", PCR PID: ";
            if (pid == PID_NULL) {
                strm << "none";
            }
            else {
                strm << pid << Format (" (0x%04X)", int (pid));
            }
            strm << std::endl;

            // Process and display "program info"
            if (info_length > 0) {
                strm << margin << "Program information:" << std::endl;
                Descriptor::Display (strm, data, info_length, indent, section.tableId());
            }
            data += info_length; size -= info_length;

            // Process and display "elementary stream info"
            while (size >= 5) {
                uint8_t stream = *data;
                ts::PID es_pid = GetUInt16 (data + 1) & 0x1FFF;
                size_t es_info_length = GetUInt16 (data + 3) & 0x0FFF;
                data += 5; size -= 5;
                if (es_info_length > size) {
                    es_info_length = size;
                }
                strm << margin << "Elementary stream: type "
                     << Format("0x%02X", int(stream))
                     << " (" << names::StreamType(stream)
                     << "), PID: " << es_pid << Format(" (0x%04X)", int(es_pid)) << std::endl;
                Descriptor::Display(strm, data, es_info_length, indent, section.tableId());
                data += es_info_length; size -= es_info_length;
            }
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - DVB NIT
//----------------------------------------------------------------------------

namespace {
    void DSdvb_nit (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        strm << margin << "Network Id: " << section.tableIdExtension() <<
            Format (" (0x%04X)", int (section.tableIdExtension())) << std::endl;

        if (size >= 2) {
            // Display network information
            size_t loop_length = GetUInt16 (data) & 0x0FFF;
            data += 2; size -= 2;
            if (loop_length > size)
                loop_length = size;
            if (loop_length > 0) {
                strm << margin << "Network information:" << std::endl;
                Descriptor::Display (strm, data, loop_length, indent, section.tableId());
            }
            data += loop_length; size -= loop_length;

            // Display transport information
            if (size >= 2) {
                loop_length = GetUInt16 (data) & 0x0FFF;
                data += 2; size -= 2;
                if (loop_length > size)
                    loop_length = size;

                // Loop across all transports
                while (loop_length >= 6) {
                    uint16_t tsid = GetUInt16 (data);
                    uint16_t nwid = GetUInt16 (data + 2);
                    size_t length = GetUInt16 (data + 4) & 0x0FFF;
                    data += 6; size -= 6; loop_length -= 6;
                    if (length > loop_length)
                        length = loop_length;
                    strm << margin << "Transport Stream Id: " << tsid <<
                        Format (" (0x%04X)", int (tsid)) <<
                        ", Original Network Id: " << nwid <<
                        Format (" (0x%04X)", int (nwid)) << std::endl;
                    Descriptor::Display (strm, data, length, indent, section.tableId());
                    data += length; size -= length; loop_length -= length;
                }
            }
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - BAT
//----------------------------------------------------------------------------

namespace {
    void DSbat (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        strm << margin << "Bouquet Id: " << section.tableIdExtension() <<
            Format (" (0x%04X)", int (section.tableIdExtension())) << std::endl;

        if (size >= 2) {
            // Display bouquet information
            size_t loop_length = GetUInt16 (data) & 0x0FFF;
            data += 2; size -= 2;
            if (loop_length > size)
                loop_length = size;
            if (loop_length > 0) {
                strm << margin << "Bouquet information:" << std::endl;
                Descriptor::Display (strm, data, loop_length, indent, section.tableId());
            }
            data += loop_length; size -= loop_length;

            // Loop across all transports
            if (size >= 2) {
                loop_length = GetUInt16 (data) & 0x0FFF;
                data += 2; size -= 2;
                if (loop_length > size)
                    loop_length = size;

                while (loop_length >= 6) {
                    uint16_t tsid = GetUInt16 (data);
                    uint16_t nwid = GetUInt16 (data + 2);
                    size_t length = GetUInt16 (data + 4) & 0x0FFF;
                    data += 6; size -= 6; loop_length -= 6;
                    if (length > loop_length)
                        length = loop_length;
                    strm << margin << "Transport Stream Id: " << tsid <<
                        Format (" (0x%04X)", int (tsid)) <<
                        ", Original Network Id: " << nwid <<
                        Format (" (0x%04X)", int (nwid)) << std::endl;
                    Descriptor::Display (strm, data, length, indent, section.tableId());
                    data += length; size -= length; loop_length -= length;
                }
            }
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - SDT
//----------------------------------------------------------------------------

namespace {
    void DSsdt (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        strm << margin << "Transport Stream Id: " << section.tableIdExtension() <<
            Format (" (0x%04X)", int (section.tableIdExtension())) << std::endl;

        if (size >= 2) {
            uint16_t nwid = GetUInt16 (data);
            strm << margin << "Original Network Id: " << nwid <<
                Format (" (0x%04X)", int (nwid)) << std::endl;
            data += 2; size -= 2;
            if (size >= 1) {
                data += 1; size -= 1; // unused byte
            }

            // Loop across all services
            while (size >= 5) {
                uint16_t servid = GetUInt16 (data);
                bool eits = (data[2] >> 1) & 0x01;
                bool eitpf = data[2] & 0x01;
                uint16_t length_bytes = GetUInt16 (data + 3);
                uint8_t running_status = uint8_t (length_bytes >> 13);
                bool ca_mode = (length_bytes >> 12) & 0x01;
                size_t length = length_bytes & 0x0FFF;
                data += 5; size -= 5;
                if (length > size)
                    length = size;
                strm << margin << "Service Id: " << servid << 
                    Format (" (0x%04X)", int (servid)) <<
                    ", EITs: " << YesNo (eits) <<
                    ", EITp/f: " << YesNo (eitpf) <<
                    ", CA mode: " << (ca_mode ? "controlled" : "free") <<
                    std::endl << margin <<
                    "Running status: " << names::RunningStatus (running_status) <<
                    std::endl;
                Descriptor::Display (strm, data, length, indent, section.tableId());
                data += length; size -= length;
            }
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - TDT
//----------------------------------------------------------------------------

namespace {
    void DStdt (std::ostream& strm, const Section& section, int indent)
    {
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        if (size >= 5) {
            Time time;
            DecodeMJD (data, 5, time);
            data += 5; size -= 5;
            strm << std::string (indent, ' ') << "UTC time: "
                 << time.format (Time::DATE | Time::TIME) << std::endl;
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - TOT
//----------------------------------------------------------------------------

namespace {
    void DStot (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();

        if (size >= 5) {
            // Fixed part
            Time time;
            DecodeMJD (data, 5, time);
            data += 5; size -= 5;
            strm << margin << "UTC time: " << time.format (Time::DATE | Time::TIME) << std::endl;

            // Descriptor loop
            if (size >= 2) {
                size_t length = GetUInt16 (data) & 0x0FFF;
                data += 2; size -= 2;
                if (length > size) {
                    length = size;
                }
                Descriptor::Display (strm, data, length, indent, section.tableId());
                data += length; size -= length;
            }

            // There is a CRC32 at the end of a TOT, even though we are in a short section.
            if (size >= 4) {
                CRC32 comp_crc32 (section.content(), data - section.content());
                uint32_t sect_crc32 = GetUInt32 (data);
                data += 4; size -= 4;
                strm << margin << Format ("CRC32: 0x%08X ", sect_crc32);
                if (sect_crc32 == comp_crc32)
                    strm << "(OK)";
                else
                    strm << Format ("(WRONG, expected 0x%08X)", comp_crc32.value());
                strm << std::endl;
            }
        }

        ExtraData (strm, data, size, indent);
    }
}

//----------------------------------------------------------------------------
// Ancillary function to display table - EIT
//----------------------------------------------------------------------------

namespace {
    void DSeit (std::ostream& strm, const Section& section, int indent)
    {
        const std::string margin (indent, ' ');
        const uint8_t* data = section.payload();
        size_t size = section.payloadSize();
        const uint16_t sid = section.tableIdExtension();

        strm << margin << Format ("Service Id: %d (0x%04X)", int (sid), int (sid)) << std::endl;

        if (size >= 6) {
            uint16_t tsid = GetUInt16 (data);
            uint16_t onid = GetUInt16 (data + 2);
            uint8_t seg_last = data[4];
            uint8_t last_tid = data[5];
            data += 6; size -= 6;

            strm << margin << Format ("TS Id: %d (0x%04X)", int (tsid), int (tsid)) << std::endl
                 << margin << Format ("Original Network Id: %d (0x%04X)", int (onid), int (onid)) << std::endl
                 << margin << Format ("Segment last section: %d (0x%02X)", int (seg_last), int (seg_last)) << std::endl
                 << margin << Format ("Last Table Id: %d (0x%02X), ", int (last_tid), int (last_tid))
                 << names::TID (last_tid, CAS_OTHER) << std::endl;
        }

        while (size >= 12) {
            uint16_t evid = GetUInt16 (data);
            Time start;
            DecodeMJD (data + 2, 5, start);
            int hour = DecodeBCD (data[7]);
            int min = DecodeBCD (data[8]);
            int sec = DecodeBCD (data[9]);
            uint8_t run = (data[10] >> 5) & 0x07;
            uint8_t ca_mode = (data[10] >> 4) & 0x01;
            size_t loop_length = GetUInt16 (data + 10) & 0x0FFF;
            data += 12; size -= 12;
            if (loop_length > size) {
                loop_length = size;
            }
            strm << margin << Format ("Event Id: %d (0x%04X)", int (evid), int (evid)) << std::endl
                 << margin << "Start UTC: " << start.format (Time::DATE | Time::TIME) << std::endl
                 << margin << Format ("Duration: %02d:%02d:%02d", hour, min, sec) << std::endl
                 << margin << "Running status: " << names::RunningStatus (run) << std::endl
                 << margin << "CA mode: " << (ca_mode ? "controlled" : "free") << std::endl;
            Descriptor::Display (strm, data, loop_length, indent, section.tableId());
            data += loop_length; size -= loop_length;
        }

        ExtraData (strm, data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Display the section on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::Section::display (std::ostream& strm, int indent, CASFamily cas, bool no_header) const
{
    const std::string margin(indent, ' ');
    const TID tid(tableId());

    // Filter invalid section

    if (!_is_valid) {
        return strm;
    }

    // Display common header lines.
    // If PID is the null PID, this means "unknown PID"

    if (!no_header) {
        strm << margin << "* " << names::TID(tid, cas) << ", TID " << tid << Format(" (0x%02X)", tid);
        if (_source_pid != PID_NULL) {
            strm << ", PID " << _source_pid << Format(" (0x%04X)", int(_source_pid));
        }
        strm << std::endl
             << margin << "  Section: " << int(sectionNumber())
             << " (last: " << int(lastSectionNumber())
             << "), version: " << int(version())
             << ", size: " << size() << " bytes" << std::endl;
        indent += 2;
    }

    // Display section body

    DisplaySectionHandler handler;
    if (tid >= TID_EIT_MIN && tid <= TID_EIT_MAX) { // 34 values
        handler = DSeit;
    }
    else {
        switch (tid) {
            case TID_PAT:     handler = DSpat; break;
            case TID_CAT:     handler = DSgeneric; break;
            case TID_PMT:     handler = DSpmt; break;
            case TID_TSDT:    handler = DSgeneric; break;
            case TID_NIT_ACT: handler = DSdvb_nit; break;
            case TID_NIT_OTH: handler = DSdvb_nit; break;
            case TID_BAT:     handler = DSbat; break;
            case TID_SDT_ACT: handler = DSsdt; break;
            case TID_SDT_OTH: handler = DSsdt; break;
            case TID_TDT:     handler = DStdt; break;
            case TID_TOT:     handler = DStot; break;
            default:          handler = DSunknown; break;
        }
    }
    handler (strm, *this, indent);

    return strm;
}


//----------------------------------------------------------------------------
// Dump the section on an output stream
//----------------------------------------------------------------------------

std::ostream& ts::Section::dump (std::ostream& strm, int indent, CASFamily cas, bool no_header) const
{
    const std::string margin (indent, ' ');
    const TID tid (tableId());

    // Filter invalid section

    if (!_is_valid) {
        return strm;
    }

    // Display common header lines.
    // If PID is the null PID, this means "unknown PID"

    if (!no_header) {
        strm << margin << "* Section dump"
             << Format (", PID %d (0x%04X)", int (_source_pid), int (_source_pid))
             << Format (", TID %d (0x%02X)", tid, tid)
             << " (" << names::TID (tid, cas) << ")" << std::endl
             << margin << "  Section size: " << size()
             << " bytes, header: " << (isLongSection() ? "long" : "short") << std::endl;
        if (isLongSection()) {
            strm << margin
                 << Format ("  TIDext: %d (0x%04X)", int (tableIdExtension()), int (tableIdExtension()))
                 << ", version: " << int (version())
                 << ", index: " << int (sectionNumber())
                 << ", last: " << int (lastSectionNumber())
                 << ", " << (isNext() ? "next" : "current") << std::endl;
        }
    }

    // Display section body

    strm << Hexa(content(), size(), hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent + 2);

    return strm;
}
