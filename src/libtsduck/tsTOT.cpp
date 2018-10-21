//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"TOT"
#define MY_TID ts::TID_TOT

TS_XML_TABLE_FACTORY(ts::TOT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::TOT, MY_TID);
TS_ID_SECTION_DISPLAY(ts::TOT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TOT::TOT(const Time& utc_time_) :
    AbstractTable(MY_TID, MY_XML_NAME),
    utc_time(utc_time_),
    regions(),
    descs(this)
{
    _is_valid = true;
}

ts::TOT::TOT(const BinaryTable& table, const DVBCharset* charset) :
    TOT()
{
    deserialize(table, charset);
}

ts::TOT::TOT(const TOT& other) :
    AbstractTable(other),
    utc_time(other.utc_time),
    regions(other.regions),
    descs(this, other.descs)
{
}


//----------------------------------------------------------------------------
// Return the local time according to a region description
//----------------------------------------------------------------------------

ts::Time ts::TOT::localTime(const Region& reg) const
{
    // Add local time offset in milliseconds
    return utc_time + MilliSecond(reg.time_offset) * 60 * MilliSecPerSec;
}


//----------------------------------------------------------------------------
// Format a time offset in minutes
//----------------------------------------------------------------------------

ts::UString ts::TOT::timeOffsetFormat(int minutes)
{
    return UString::Format(u"%s%02d:%02d", {minutes < 0 ? u"-" : u"", ::abs(minutes) / 60, ::abs(minutes) % 60});
}


//----------------------------------------------------------------------------
// Add descriptors, filling regions from local_time_offset_descriptor's.
//----------------------------------------------------------------------------

void ts::TOT::addDescriptors(const DescriptorList& dlist)
{
    // Loop on all descriptors.
    for (size_t index = 0; index < dlist.count(); ++index) {
        if (!dlist[index].isNull() && dlist[index]->isValid()) {
            if (dlist[index]->tag() != DID_LOCAL_TIME_OFFSET) {
                // Not a local_time_offset_descriptor, add to descriptor list.
                descs.add(dlist[index]);
            }
            else {
                // Decode local_time_offset_descriptor in the list of regions.
                LocalTimeOffsetDescriptor lto(*dlist[index]);
                if (lto.isValid()) {
                    regions.insert(regions.end(), lto.regions.begin(), lto.regions.end());
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TOT::deserialize(const BinaryTable& table, const DVBCharset* charset)
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
    const Section& sect(*table.sectionAt(0));
    const uint8_t* data(sect.payload());
    size_t remain(sect.payloadSize());

    // Abort if not a TOT
    if (sect.tableId() != MY_TID) {
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
    if (CRC32(sect.content(), size) != GetUInt32(sect.content() + size)) {
        return;
    }

    // Analyze the section payload:
    // - 40-bit UTC time in MJD format.
    // - 16-bit length for descriptor loop
    if (remain < MJD_SIZE + 2) {
        return;
    }
    DecodeMJD(data, MJD_SIZE, utc_time);
    size_t length = GetUInt16(data + MJD_SIZE) & 0x0FFF;
    data += MJD_SIZE + 2;
    remain -= MJD_SIZE + 2;
    remain = std::min(length, remain);

    // Get descriptor list.
    // Build a descriptor list.
    DescriptorList dlist(nullptr);
    dlist.add(data, length);
    addDescriptors(dlist);

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TOT::serialize(BinaryTable& table, const DVBCharset* charset) const
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
    DescriptorList dlist(nullptr);

    // Add all regions in one or more local_time_offset_descriptor.
    LocalTimeOffsetDescriptor lto;
    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        lto.regions.push_back(*it);
        if (lto.regions.size() >= LocalTimeOffsetDescriptor::MAX_REGION) {
            dlist.add(lto);
            lto.regions.clear();
        }
    }
    if (!lto.regions.empty()) {
        dlist.add(lto);
    }

    // Append the "other" descriptors to the list
    dlist.add(descs);

    // Insert descriptor list (with leading length field).
    // Keep 4 bytes for CRC.
    remain -= 4;
    size_t next_index = dlist.lengthSerialize(data, remain);
    if (next_index != dlist.count()) {
        // Could not serialize all descriptors
        // No simple way to report this error.
        // Add error processing here later.
    }

    // Add the section in the table (include CRC)
    table.addSection(new Section(MY_TID,
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
            strm << margin << UString::Format(u"CRC32: 0x%X ", {sect_crc32});
            if (sect_crc32 == comp_crc32) {
                strm << "(OK)";
            }
            else {
                strm << UString::Format(u"(WRONG, expected 0x%X)", {comp_crc32.value()});
            }
            strm << std::endl;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TOT::buildXML(xml::Element* root) const
{
    root->setDateTimeAttribute(u"UTC_time", utc_time);

    // Add one local_time_offset_descriptor per set of regions.
    // Each local_time_offset_descriptor can contain up to 19 regions.
    LocalTimeOffsetDescriptor lto;
    for (RegionVector::const_iterator it = regions.begin(); it != regions.end(); ++it) {
        lto.regions.push_back(*it);
        if (lto.regions.size() >= LocalTimeOffsetDescriptor::MAX_REGION) {
            // The descriptor is full, flush it in the list.
            lto.toXML(root);
            lto.regions.clear();
        }
    }
    if (!lto.regions.empty()) {
        // The descriptor is not empty, flush it in the list.
        lto.toXML(root);
    }

    // Add other descriptors.
    descs.toXML(root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TOT::fromXML(const xml::Element* element)
{
    regions.clear();
    descs.clear();
    DescriptorList orig(this);

    // Get all descriptors in a separated list.
    _is_valid =
        checkXMLName(element) &&
        element->getDateTimeAttribute(utc_time, u"UTC_time", true) &&
        orig.fromXML(element);

    // Then, split local_time_offset_descriptor and others.
    addDescriptors(orig);
}
