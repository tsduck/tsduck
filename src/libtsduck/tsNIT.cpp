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
//  Representation of a Network Information Table (NIT)
//
//----------------------------------------------------------------------------

#include "tsNIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"NIT"

TS_XML_TABLE_FACTORY(ts::NIT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::NIT, ts::TID_NIT_ACT);
TS_ID_TABLE_FACTORY(ts::NIT, ts::TID_NIT_OTH);
TS_ID_SECTION_DISPLAY(ts::NIT::DisplaySection, ts::TID_NIT_ACT);
TS_ID_SECTION_DISPLAY(ts::NIT::DisplaySection, ts::TID_NIT_OTH);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::NIT::NIT(bool is_actual, uint8_t vers, bool cur, uint16_t id) :
    AbstractTransportListTable(uint8_t(is_actual ? TID_NIT_ACT : TID_NIT_OTH), MY_XML_NAME, id, vers, cur),
    network_id(_tid_ext)
{
}

ts::NIT::NIT(const BinaryTable& table, const DVBCharset* charset) :
    AbstractTransportListTable(TID_NIT_ACT, MY_XML_NAME, table, charset),  // TID updated by deserialize()
    network_id(_tid_ext)
{
}

ts::NIT::NIT(const NIT& other) :
    AbstractTransportListTable(other),
    network_id(_tid_ext)
{
}

ts::NIT& ts::NIT::operator=(const NIT& other)
{
    if (&other != this) {
        // Assign super class but leave uint16_t& network_id unchanged.
        AbstractTransportListTable::operator=(other);
    }
    return *this;
}


//----------------------------------------------------------------------------
// A static method to display a NIT section.
//----------------------------------------------------------------------------

void ts::NIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Network Id: %d (0x%X)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 2) {
        // Display network information
        size_t loop_length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;
        if (loop_length > size) {
            loop_length = size;
        }
        if (loop_length > 0) {
            strm << margin << "Network information:" << std::endl;
            display.displayDescriptorList(data, loop_length, indent, section.tableId());
        }
        data += loop_length; size -= loop_length;

        // Display transport information
        if (size >= 2) {
            loop_length = GetUInt16(data) & 0x0FFF;
            data += 2; size -= 2;
            if (loop_length > size) {
                loop_length = size;
            }

            // Loop across all transports
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

void ts::NIT::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"network_id", network_id, true);
    root->setBoolAttribute(u"actual", isActual());
    descs.toXML(root);

    for (TransportMap::const_iterator it = transports.begin(); it != transports.end(); ++it) {
        xml::Element* e = root->addElement(u"transport_stream");
        e->setIntAttribute(u"transport_stream_id", it->first.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", it->first.original_network_id, true);
        if (it->second.preferred_section >= 0) {
            e->setIntAttribute(u"preferred_section", it->second.preferred_section, false);
        }
        it->second.descs.toXML(e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::NIT::fromXML(const xml::Element* element)
{
    descs.clear();
    transports.clear();

    xml::ElementVector children;
    bool actual = true;

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(network_id, u"network_id", true, 0, 0x0000, 0xFFFF) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        descs.fromXML(children, element, u"transport_stream");

    setActual(actual);

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        TransportStreamId ts;
        _is_valid =
            children[index]->getIntAttribute<uint16_t>(ts.transport_stream_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
            children[index]->getIntAttribute<uint16_t>(ts.original_network_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
            transports[ts].descs.fromXML(children[index]);
        if (_is_valid && children[index]->hasAttribute(u"preferred_section")) {
            _is_valid = children[index]->getIntAttribute<int>(transports[ts].preferred_section, u"preferred_section", true, 0, 0, 255);
        }
        else {
            transports[ts].preferred_section = -1;
        }
    }
}
