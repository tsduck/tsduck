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
//  Representation of a Time Offset Table (TOT)
//
//----------------------------------------------------------------------------

#include "tsTOT.h"
#include "tsMJD.h"
#include "tsBCD.h"
#include "tsCRC32.h"
#include "tsFormat.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_TABLE_FACTORY(ts::TOT, "TOT");
TS_ID_TABLE_FACTORY(ts::TOT, ts::TID_TOT);
TS_ID_SECTION_DISPLAY(ts::TOT::DisplaySection, ts::TID_TOT);


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::TOT::TOT(const Time& utc_time_) :
    AbstractTable(TID_TOT, "TOT"),
    utc_time(utc_time_),
    regions(),
    descs()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::TOT::TOT(const BinaryTable& table) :
    AbstractTable(TID_TOT, "TOT"),
    utc_time(),
    regions(),
    descs()
{
    deserialize(table);
}


//----------------------------------------------------------------------------
// Return the local time according to a region description
//----------------------------------------------------------------------------

ts::Time ts::TOT::localTime (const Region& reg) const
{
    // Add local time offset in milliseconds
    return utc_time + MilliSecond (reg.time_offset) * 60 * MilliSecPerSec;
}


//----------------------------------------------------------------------------
// Format a time offset in minutes
//----------------------------------------------------------------------------

std::string ts::TOT::timeOffsetFormat (int minutes)
{
    return Format ("%s%02d:%02d", minutes < 0 ? "-" : "", ::abs (minutes) / 60, ::abs (minutes) % 60);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TOT::deserialize (const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;
    regions.clear();
    descs.clear();

    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect (*table.sectionAt(0));
    const uint8_t* data (sect.payload());
    size_t remain (sect.payloadSize());

    // Abort if not a TOT
    if (sect.tableId() != TID_TOT) {
        return;
    }

    // A TOT section is a short section with a CRC32.
    // Normally, only long sections have a CRC32.
    // So, the generic code has not checked the CRC32.
    if (remain < 4) { // no CRC32
        return;
    }
    // Remove CRC32 from payload
    remain -= 4;
    // Verify CRC32 in section
    size_t size = sect.size() - 4;
    if (CRC32 (sect.content(), size) != GetUInt32 (sect.content() + size)) {
        return;
    }

    // Analyze the section payload:
    // - 40-bit UTC time in MJD format.
    // - 16-bit length for descriptor loop
    if (remain < MJD_SIZE + 2) {
        return;
    }
    DecodeMJD (data, MJD_SIZE, utc_time);
    size_t length (GetUInt16 (data + MJD_SIZE) & 0x0FFF);
    data += MJD_SIZE + 2;
    remain -= MJD_SIZE + 2;
    remain = std::min (length, remain);

    // Get descriptor list.
    // Analyze local_time_offset_descriptor, keep others as binary
    while (remain >= 2) {
        // Get descriptor tag and length
        uint8_t tag (data[0]);
        data += 2;
        remain -= 2;
        length = std::min (size_t (data[-1]), remain);
        if (tag != DID_LOCAL_TIME_OFFSET) {
            descs.add (data - 2, length + 2);
        }
        else {
            while (length >= 13) {
                // Decode one region
                Region reg;
                reg.country = std::string (reinterpret_cast <const char*> (data), 3);
                reg.region_id = data[3] >> 2;
                int sign = (data[3] & 0x01) ? -1 : 1;
                reg.time_offset = sign * (DecodeBCD (data[4]) * 60 + DecodeBCD (data[5]));
                DecodeMJD (data + 6, 5, reg.next_change);
                reg.next_time_offset = sign * (DecodeBCD (data[11]) * 60 + DecodeBCD (data[12]));
                regions.push_back (reg);
                // Move to next region
                data += 13;
                length -= 13;
                remain -= 13;
            }
        }
        // Move to next descriptor
        data += length;
        remain -= length;
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TOT::serialize (BinaryTable& table) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the section
    uint8_t payload [MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Encode the data in MJD in the payload (5 bytes)
    EncodeMJD (utc_time, data, MJD_SIZE);
    data += MJD_SIZE;
    remain -= MJD_SIZE;

    // Build a descriptor list.
    DescriptorList dlist;

    // Add all regions in one or more local_time_offset_descriptor
    uint8_t descbuf [MAX_DESCRIPTOR_SIZE];
    uint8_t* desc_data (descbuf + 2);
    size_t desc_remain (sizeof(descbuf) - 2);

    for (size_t i = 0; i < regions.size (); ++i) {
        const Region& reg (regions[i]);

        // Serialize one region in the descriptor
        ::memcpy (desc_data, reg.country.c_str(), 3);  // Flawfinder: ignore: memcpy()
        desc_data[3] = uint8_t(reg.region_id << 2) | (reg.time_offset < 0 ? 0x03 : 0x02);
        desc_data[4] = EncodeBCD(::abs(reg.time_offset) / 60);
        desc_data[5] = EncodeBCD(::abs(reg.time_offset) % 60);
        EncodeMJD(reg.next_change, desc_data + 6, 5);
        desc_data[11] = EncodeBCD(::abs(reg.next_time_offset) / 60);
        desc_data[12] = EncodeBCD(::abs(reg.next_time_offset) % 60);
        desc_data += 13;
        desc_remain -= 13;

        // Insert a descriptor in the list when there is no more space
        // for a new region in the current descriptor or at end of list.
        if (i == regions.size() - 1 || desc_remain < 13) {
            // Create one descriptor
            descbuf[0] = DID_LOCAL_TIME_OFFSET;
            descbuf[1] = uint8_t(desc_data - (descbuf + 2));
            dlist.add(descbuf);
            // Reinitialize descriptor buffer
            desc_data = descbuf + 2;
            desc_remain = sizeof(descbuf) - 2;
        }
    }

    // Append the "other" descriptors to the list
    dlist.add(descs);

    // Insert descriptor list (with leading length field).
    // Keep 4 bytes for CRC.
    remain -= 4;
    size_t next_index = dlist.lengthSerialize (data, remain);
    if (next_index != dlist.count()) {
        // Could not serialize all descriptors
        // No simple way to report this error.
        // Add error processing here later.
    }

    // Add the section in the table (include CRC)
    table.addSection(new Section(TID_TOT,
                                 true,   // is_private_section
                                 payload,
                                 data + 4 - payload));

    // Now artificially rebuild a CRC at end of section
    const Section& sect(*table.sectionAt(0));
    data = const_cast <uint8_t*>(sect.content());
    size_t size = sect.size();
    assert(size > 4);
    PutUInt32(data + size - 4, CRC32(data, size - 4).value());
}


//----------------------------------------------------------------------------
// A static method to display a TOT section.
//----------------------------------------------------------------------------

void ts::TOT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 5) {
        // Fixed part
        Time time;
        DecodeMJD(data, 5, time);
        data += 5; size -= 5;
        strm << margin << "UTC time: " << time.format(Time::DATE | Time::TIME) << std::endl;

        // Descriptor loop
        if (size >= 2) {
            size_t length = GetUInt16(data) & 0x0FFF;
            data += 2; size -= 2;
            if (length > size) {
                length = size;
            }
            display.displayDescriptorList(data, length, indent, section.tableId());
            data += length; size -= length;
        }

        // There is a CRC32 at the end of a TOT, even though we are in a short section.
        if (size >= 4) {
            CRC32 comp_crc32(section.content(), data - section.content());
            uint32_t sect_crc32 = GetUInt32(data);
            data += 4; size -= 4;
            strm << margin << Format("CRC32: 0x%08X ", sect_crc32);
            if (sect_crc32 == comp_crc32) {
                strm << "(OK)";
            }
            else {
                strm << Format("(WRONG, expected 0x%08X)", comp_crc32.value());
            }
            strm << std::endl;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::TOT::toXML(XML& xml, XML::Document& doc) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TOT::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
