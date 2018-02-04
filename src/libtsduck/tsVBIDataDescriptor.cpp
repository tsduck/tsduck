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
//  Representation of a VBI_data_descriptor
//
//----------------------------------------------------------------------------

#include "tsVBIDataDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"VBI_data_descriptor"
#define MY_DID ts::DID_VBI_DATA

TS_XML_DESCRIPTOR_FACTORY(ts::VBIDataDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::VBIDataDescriptor, ts::EDID(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::VBIDataDescriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::VBIDataDescriptor::Field::Field(bool parity, uint8_t offset) :
    field_parity(parity),
    line_offset(offset)
{
}

ts::VBIDataDescriptor::Service::Service(uint8_t id) :
    data_service_id(id),
    fields(),
    reserved()
{
}

ts::VBIDataDescriptor::VBIDataDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    services()
{
    _is_valid = true;
}

ts::VBIDataDescriptor::VBIDataDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    VBIDataDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Check if an entry has reserved bytes.
//----------------------------------------------------------------------------

bool ts::VBIDataDescriptor::EntryHasReservedBytes(uint8_t data_service_id)
{
    switch (data_service_id) {
        case 0x01:
        case 0x02:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return false;
        default:
            return true;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 2) {
        const uint8_t data_id = data[0];
        size_t length = data[1];
        data += 2; size -= 2;
        if (length > size) {
            length = size;
        }
        strm << margin << "Data service id: " << DVBNameFromSection(u"VBIDataServiceId", data_id, names::HEXA_FIRST) << std::endl;
        if (!EntryHasReservedBytes(data_id)) {
            while (length > 0) {
                const uint8_t field_parity = (data[0] >> 5) & 0x01;
                const uint8_t line_offset = data[0] & 0x1F;
                data++; size--; length--;
                strm << margin << "Field parity: " << int(field_parity) << ", line offset: " << int(line_offset) << std::endl;
            }
        }
        else if (length > 0) {
            strm << margin << "Associated data:" << std::endl
                << UString::Dump(data, length, UString::HEXA | UString::ASCII, indent);
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());

    for (ServiceList::const_iterator it1 = services.begin(); it1 != services.end(); ++it1) {
        bbp->appendUInt8(it1->data_service_id);
        if (it1->hasReservedBytes()) {
            bbp->appendUInt8(it1->reserved.size());
            bbp->append(it1->reserved);
        }
        else {
            bbp->appendUInt8(it1->fields.size()); // one byte per field entry
            for (FieldList::const_iterator it2 = it1->fields.begin(); it2 != it1->fields.end(); ++it2) {
                bbp->appendUInt8(0xC0 | (it2->field_parity ? 0x20 : 0x00) | (it2->line_offset & 0x1F));
            }
        }
    }

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    services.clear();

    if (!(_is_valid = desc.isValid() && desc.tag() == _tag)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    while (size >= 2) {
        Service service(data[0]);
        size_t length = data[1];
        data += 2; size -= 2;
        if (length > size) {
            length = size;
        }
        if (!service.hasReservedBytes()) {
            while (length > 0) {
                service.fields.push_back(Field((data[0] & 0x20) != 0, data[0] & 0x1F));
                data++; size--; length--;
            }
        }
        else if (length > 0) {
            service.reserved.copy(data, length);
            data += length; size -= length;
        }
        services.push_back(service);
    }

    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::buildXML(xml::Element* root) const
{
    for (ServiceList::const_iterator it1 = services.begin(); it1 != services.end(); ++it1) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"data_service_id", it1->data_service_id);
        if (it1->hasReservedBytes()) {
            if (!it1->reserved.empty()) {
                e->addElement(u"reserved")->addHexaText(it1->reserved);
            }
        }
        else {
            for (FieldList::const_iterator it2 = it1->fields.begin(); it2 != it1->fields.end(); ++it2) {
                xml::Element* f = e->addElement(u"field");
                f->setBoolAttribute(u"field_parity", it2->field_parity);
                f->setIntAttribute(u"line_offset", it2->line_offset);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::fromXML(const xml::Element* element)
{
    services.clear();

    xml::ElementVector srv;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(srv, u"service");

    for (size_t srvIndex = 0; _is_valid && srvIndex < srv.size(); ++srvIndex) {
        Service service;
        xml::ElementVector fld;

        _is_valid =
            srv[srvIndex]->getIntAttribute<uint8_t>(service.data_service_id, u"data_service_id", true) &&
            srv[srvIndex]->getChildren(fld, u"field") &&
            srv[srvIndex]->getHexaTextChild(service.reserved, u"reserved", false);

        if (_is_valid) {
            if (service.hasReservedBytes()) {
                if (!fld.empty()) {
                    element->report().error(u"no <field> allowed in <service>, line %d, when data_service_id='%d'", {srv[srvIndex]->lineNumber(), service.data_service_id});
                    _is_valid = false;
                }
            }
            else {
                if (!service.reserved.empty()) {
                    element->report().error(u"no <reserved> allowed in <service>, line %d, when data_service_id='%d'", {srv[srvIndex]->lineNumber(), service.data_service_id});
                    _is_valid = false;
                }
            }
        }

        for (size_t fldIndex = 0; _is_valid && fldIndex < fld.size(); ++fldIndex) {
            Field field;
            _is_valid =
                fld[fldIndex]->getBoolAttribute(field.field_parity, u"field_parity", false, false) &&
                fld[fldIndex]->getIntAttribute<uint8_t>(field.line_offset, u"line_offset", false, 0x00, 0x00, 0x1F);
            service.fields.push_back(field);
        }

        if (_is_valid) {
            services.push_back(service);
        }
    }
}
