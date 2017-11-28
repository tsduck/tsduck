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
//  Representation of a Bouquet Association Table (BAT)
//
//----------------------------------------------------------------------------

#include "tsBAT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsXMLTables.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"BAT"
#define MY_TID ts::TID_BAT

TS_XML_TABLE_FACTORY(ts::BAT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::BAT, MY_TID);
TS_ID_SECTION_DISPLAY(ts::BAT::DisplaySection, MY_TID);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BAT::BAT(uint8_t vers, bool cur, uint16_t id) :
    AbstractTransportListTable(MY_TID, MY_XML_NAME, id, vers, cur),
    bouquet_id(_tid_ext)
{
}

ts::BAT::BAT(const BinaryTable& table, const DVBCharset* charset) :
    AbstractTransportListTable(MY_TID, MY_XML_NAME, table, charset),
    bouquet_id(_tid_ext)
{
}


//----------------------------------------------------------------------------
// A static method to display a BAT section.
//----------------------------------------------------------------------------

void ts::BAT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Bouquet Id: %d (0x%04X)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 2) {
        // Display bouquet information
        size_t loop_length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;
        if (loop_length > size) {
            loop_length = size;
        }
        if (loop_length > 0) {
            strm << margin << "Bouquet information:" << std::endl;
            display.displayDescriptorList(data, loop_length, indent, section.tableId());
        }
        data += loop_length; size -= loop_length;

        // Loop across all transports
        if (size >= 2) {
            loop_length = GetUInt16(data) & 0x0FFF;
            data += 2; size -= 2;
            if (loop_length > size) {
                loop_length = size;
            }

            while (loop_length >= 6) {
                uint16_t tsid = GetUInt16(data);
                uint16_t nwid = GetUInt16(data + 2);
                size_t length = GetUInt16(data + 4) & 0x0FFF;
                data += 6; size -= 6; loop_length -= 6;
                if (length > loop_length) {
                    length = loop_length;
                }
                strm << margin << UString::Format(u"Transport Stream Id: %d (0x%X), Original Network Id: %d (0x%X)", {tsid, tsid, nwid, nwid}) << std::endl;
                display.displayDescriptorList(data, length, indent, section.tableId());
                data += length; size -= length; loop_length -= length;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::BAT::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "version", version);
    xml.setBoolAttribute(root, "current", is_current);
    xml.setIntAttribute(root, "bouquet_id", bouquet_id, true);
    XMLTables::ToXML(xml, root, descs);

    for (TransportMap::const_iterator it = transports.begin(); it != transports.end(); ++it) {
        XML::Element* e = xml.addElement(root, "transport_stream");
        xml.setIntAttribute(e, "transport_stream_id", it->first.transport_stream_id, true);
        xml.setIntAttribute(e, "original_network_id", it->first.original_network_id, true);
        XMLTables::ToXML(xml, e, it->second);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::BAT::fromXML(XML& xml, const XML::Element* element)
{
    descs.clear();
    transports.clear();

    XML::ElementVector children;
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint8_t>(version, element, "version", false, 0, 0, 31) &&
        xml.getBoolAttribute(is_current, element, "current", false, true) &&
        xml.getIntAttribute<uint16_t>(bouquet_id, element, "bouquet_id", true, 0, 0x0000, 0xFFFF) &&
        XMLTables::FromDescriptorListXML(descs, children, xml, element, "transport_stream");

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        TransportStreamId ts;
        _is_valid =
            xml.getIntAttribute<uint16_t>(ts.transport_stream_id, children[index], "transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
            xml.getIntAttribute<uint16_t>(ts.original_network_id, children[index], "original_network_id", true, 0, 0x0000, 0xFFFF) &&
            XMLTables::FromDescriptorListXML(transports[ts], xml, children[index]);
    }
}
