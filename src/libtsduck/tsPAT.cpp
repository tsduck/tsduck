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
//  Representation of a Program Association Table (PAT)
//
//----------------------------------------------------------------------------

#include "tsPAT.h"
#include "tsFormat.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::PAT::PAT (uint8_t  version_,
                bool   is_current_,
                uint16_t ts_id_,
                PID    nit_pid_) :

    AbstractLongTable (TID_PAT, version_, is_current_),
    ts_id (ts_id_),
    nit_pid (nit_pid_),
    pmts ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::PAT::PAT (const BinaryTable& table) :
    AbstractLongTable (TID_PAT),
    ts_id (0),
    nit_pid (PID_NULL),
    pmts ()
{
    deserialize (table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PAT::deserialize (const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;
    nit_pid = PID_NULL;
    pmts.clear ();

    if (!table.isValid() || table.tableId() != _table_id) {
        return;
    }

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect (*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        ts_id = sect.tableIdExtension();

        // Analyze the section payload:
        // This is a list of service_id/pmt_pid pairs
        const uint8_t* data (sect.payload());
        size_t remain (sect.payloadSize());

        while (remain >= 4) {
            // Extract one id/pid entry
            uint16_t id (GetUInt16 (data));
            uint16_t pid (GetUInt16 (data + 2) & 0x1FFF);
            data += 4;
            remain -= 4;

            // Register the PID
            if (id == 0) {
                nit_pid = pid;
            }
            else {
                pmts[id] = pid;
            }
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PAT::serialize (BinaryTable& table) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the sections
    uint8_t payload [MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    int section_number (0);
    uint8_t* data (payload);
    size_t remain (sizeof(payload));

    // Add the NIT PID in the first section
    if (nit_pid != PID_NULL) {
        PutUInt16 (data, 0); // pseudo service_id
        PutUInt16 (data + 2, nit_pid | 0xE000);
        data += 4;
        remain -= 4;
    }

    // Add all services
    for (ServiceMap::const_iterator it = pmts.begin(); it != pmts.end(); ++it) {

        // If current section payload is full, close the current section.
        // We always use last_section_number = section_number but the
        // table is allowed to grow (see BinaryTable::AddSection).
        if (remain < 4) {
            table.addSection (new Section (_table_id,
                                           false,   // is_private_section
                                           ts_id,   // tid_ext
                                           version,
                                           is_current,
                                           uint8_t(section_number),
                                           uint8_t(section_number), //last_section_number
                                           payload,
                                           data - payload)); // payload_size,
            section_number++;
            data = payload;
            remain = sizeof(payload);
        }

        // Add current service entry into the PAT section
        PutUInt16 (data, it->first);               // service_id
        PutUInt16 (data + 2, it->second | 0xE000); // pmt pid
        data += 4;
        remain -= 4;
    }

    // Add partial section (if there is one)

    if (data > payload || table.sectionCount() == 0) {
        table.addSection (new Section (_table_id,
                                       false,   // is_private_section
                                       ts_id,   // tid_ext
                                       version,
                                       is_current,
                                       uint8_t(section_number),
                                       uint8_t(section_number), //last_section_number
                                       payload,
                                       data - payload)); // payload_size,
    }
}


//----------------------------------------------------------------------------
// A static method to display a PAT section.
//----------------------------------------------------------------------------

void ts::PAT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    uint16_t tsid = section.tableIdExtension();

    strm << margin << ts::Format("TS id:   %5d (0x%04X)", int(tsid), int(tsid)) << std::endl;

    // Loop through all program / pid pairs
    while (size >= 4) {
        uint16_t program = GetUInt16(data);
        uint16_t pid = GetUInt16(data + 2) & 0x1FFF;
        data += 4; size -= 4;
        strm << margin
             << Format("%s %5d (0x%04X)  PID: %4d (0x%04X)",
                       program == 0 ? "NIT:    " : "Program:",
                       int(program), int(program),
                       int(pid), int(pid))
            << std::endl;
    }

    DisplayExtraData(display, data, size, indent);
}
