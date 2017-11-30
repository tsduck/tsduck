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
//  Representation of a Program Map Table (PMT)
//
//----------------------------------------------------------------------------

#include "tsPMT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsXMLTables.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"PMT"
#define MY_TID ts::TID_PMT

TS_XML_TABLE_FACTORY(ts::PMT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::PMT, MY_TID);
TS_ID_SECTION_DISPLAY(ts::PMT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::PMT::PMT(uint8_t version_, bool is_current_, uint16_t service_id_, PID pcr_pid_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, version_, is_current_),
    service_id(service_id_),
    pcr_pid(pcr_pid_),
    descs(),
    streams()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::PMT::PMT(const BinaryTable& table, const DVBCharset* charset) :
    AbstractLongTable(MY_TID, MY_XML_NAME),
    service_id(0),
    pcr_pid(PID_NULL),
    descs(),
    streams()
{
    deserialize(table, charset);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PMT::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    service_id = 0;
    pcr_pid = PID_NULL;
    descs.clear();
    streams.clear();

    if (!table.isValid() || table.tableId() != _table_id) {
        return;
    }

    // Loop on all sections (although a PMT is not allowed to use more than
    // one section, see ISO/IEC 13818-1:2000 2.4.4.8 & 2.4.4.9)
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        service_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data(sect.payload());
        size_t remain(sect.payloadSize());

        // Get PCR PID
        if (remain < 2) {
            return;
        }
        pcr_pid = GetUInt16(data) & 0x1FFF;
        data += 2;
        remain -= 2;

        // Get program information descriptor list
        if (remain < 2) {
            return;
        }
        size_t info_length(GetUInt16(data) & 0x0FFF);
        data += 2;
        remain -= 2;
        info_length = std::min(info_length, remain);
        descs.add(data, info_length);
        data += info_length;
        remain -= info_length;

        // Get elementary streams description
        while (remain >= 5) {
            PID pid = GetUInt16(data + 1) & 0x1FFF;
            Stream& str(streams[pid]);
            str.stream_type = data[0];
            info_length = GetUInt16(data + 3) & 0x0FFF;
            data += 5;
            remain -= 5;
            info_length = std::min(info_length, remain);
            str.descs.add(data, info_length);
            data += info_length;
            remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PMT::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the section. Note that a PMT is not allowed to use more than
    // one section, see ISO/IEC 13818-1:2000 2.4.4.8 & 2.4.4.9
    uint8_t payload [MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data(payload);
    size_t remain(sizeof(payload));

    // Add PCR PID
    PutUInt16(data, pcr_pid | 0xE000);
    data += 2;
    remain -= 2;

    // Insert program_info descriptor list (with leading length field)
    descs.lengthSerialize(data, remain);

    // Add description of all elementary streams
    for (StreamMap::const_iterator it = streams.begin(); it != streams.end() && remain >= 5; ++it) {

        // Insert stream type and pid
        data[0] = it->second.stream_type;
        PutUInt16(data + 1, it->first | 0xE000); // PID
        data += 3;
        remain -= 3;

        // Insert descriptor list for elem. stream (with leading length field)
        size_t next_index = it->second.descs.lengthSerialize(data, remain);
        if (next_index != it->second.descs.count()) {
            // Not enough space to serialize all descriptors in the section.
            // A PMT cannot have more than one section.
            // Return with table left in invalid state.
            return;
        }
    }

    // Add one single section in the table
    table.addSection(new Section(MY_TID,          // tid
                                 false,            // is_private_section
                                 service_id,       // tid_ext
                                 version,
                                 is_current,
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// Check if an elementary stream carries audio, video or subtitles.
// Does not just look at the stream type.
// Also analyzes the descriptor list for addional information.
//----------------------------------------------------------------------------

bool ts::PMT::Stream::isVideo() const
{
    return IsVideoST(stream_type);
}

bool ts::PMT::Stream::isAudio() const
{
    // AC-3 or HE-AAC components may have "PES private data" stream type
    // but are identifier by specific descriptors.

    return IsAudioST(stream_type) ||
        descs.search(DID_DTS) < descs.count() ||
        descs.search(DID_AC3) < descs.count() ||
        descs.search(DID_ENHANCED_AC3) < descs.count() ||
        descs.search(DID_AAC) < descs.count();
}

bool ts::PMT::Stream::isSubtitles() const
{
    // A subtitling descriptor always indicates subtitles.
    if (descs.search(DID_SUBTITLING) < descs.count()) {
        return true;
    }
    // A teletext descriptor may indicate subtitles
    for (size_t index = 0; (index = descs.search(DID_TELETEXT, index)) < descs.count(); ++index) {
        // Get descriptor payload
        const uint8_t* data = descs[index]->payload();
        size_t size = descs[index]->payloadSize();
        // Loop on all language entries, check if teletext type is a subtitle
        while (size >= 5) {
            uint8_t ttype = data[3] >> 3;
            if (ttype == 0x02 || ttype == 0x05) {
                return true; // teletext subtitles types
            }
            data += 5;
            size -= 5;
        }
    }
    // After all, no subtitle here...
    return false;
}


//----------------------------------------------------------------------------
// A static method to display a PMT section.
//----------------------------------------------------------------------------

void ts::PMT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 4) {
        // Fixed part
        PID pid = GetUInt16(data) & 0x1FFF;
        size_t info_length = GetUInt16(data + 2) & 0x0FFF;
        data += 4; size -= 4;
        if (info_length > size) {
            info_length = size;
        }
        strm << margin << UString::Format(u"Program: %d (0x%X)", {section.tableIdExtension(), section.tableIdExtension()})
             << ", PCR PID: ";
        if (pid == PID_NULL) {
            strm << "none";
        }
        else {
            strm << pid << UString::Format(u" (0x%X)", {pid});
        }
        strm << std::endl;

        // Process and display "program info"
        if (info_length > 0) {
            strm << margin << "Program information:" << std::endl;
            display.displayDescriptorList(data, info_length, indent, section.tableId());
        }
        data += info_length; size -= info_length;

        // Process and display "elementary stream info"
        while (size >= 5) {
            uint8_t stream = *data;
            PID es_pid = GetUInt16(data + 1) & 0x1FFF;
            size_t es_info_length = GetUInt16(data + 3) & 0x0FFF;
            data += 5; size -= 5;
            if (es_info_length > size) {
                es_info_length = size;
            }
            strm << margin << "Elementary stream: type " << names::StreamType(stream, names::FIRST)
                 << ", PID: " << es_pid << UString::Format(u" (0x%X)", {es_pid}) << std::endl;
            display.displayDescriptorList(data, es_info_length, indent, section.tableId());
            data += es_info_length; size -= es_info_length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::PMT::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, u"version", version);
    xml.setBoolAttribute(root, u"current", is_current);
    xml.setIntAttribute(root, u"service_id", service_id, true);
    if (pcr_pid != PID_NULL) {
        xml.setIntAttribute(root, u"PCR_PID", pcr_pid, true);
    }
    XMLTables::ToXML(xml, root, descs);

    for (StreamMap::const_iterator it = streams.begin(); it != streams.end(); ++it) {
        XML::Element* e = xml.addElement(root, u"component");
        xml.setIntAttribute(e, u"elementary_PID", it->first, true);
        xml.setIntAttribute(e, u"stream_type", it->second.stream_type, true);
        XMLTables::ToXML(xml, e, it->second.descs);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::PMT::fromXML(XML& xml, const XML::Element* element)
{
    descs.clear();
    streams.clear();

    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint8_t>(version, element, u"version", false, 0, 0, 31) &&
        xml.getBoolAttribute(is_current, element, u"current", false, true) &&
        xml.getIntAttribute<uint16_t>(service_id, element, u"service_id", true, 0, 0x0000, 0xFFFF) &&
        xml.getIntAttribute<PID>(pcr_pid, element, u"PCR_PID", false, PID_NULL, 0x0000, 0x1FFF) &&
        XMLTables::FromDescriptorListXML(descs, children, xml, element, u"component");

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        PID pid = PID_NULL;
        Stream stream;
        _is_valid =
            xml.getIntAttribute<uint8_t>(stream.stream_type, children[index], u"stream_type", true, 0, 0x00, 0xFF) &&
            xml.getIntAttribute<PID>(pid, children[index], u"elementary_PID", true, 0, 0x0000, 0x1FFF) &&
            XMLTables::FromDescriptorListXML(stream.descs, xml, children[index]);
        if (_is_valid) {
            streams[pid] = stream;
        }
    }
}
