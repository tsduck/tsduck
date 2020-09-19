//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsVBIDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"VBI_data_descriptor"
#define MY_CLASS ts::VBIDataDescriptor
#define MY_DID ts::DID_VBI_DATA
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    services()
{
}

ts::VBIDataDescriptor::VBIDataDescriptor(DuckContext& duck, const Descriptor& desc) :
    VBIDataDescriptor()
{
    deserialize(duck, desc);
}

void ts::VBIDataDescriptor::clearContent()
{
    services.clear();
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

void ts::VBIDataDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    while (size >= 2) {
        const uint8_t data_id = data[0];
        size_t length = data[1];
        data += 2; size -= 2;
        if (length > size) {
            length = size;
        }
        disp << margin << "Data service id: " << NameFromSection(u"VBIDataServiceId", data_id, names::HEXA_FIRST) << std::endl;
        if (!EntryHasReservedBytes(data_id)) {
            while (length > 0) {
                const uint8_t field_parity = (data[0] >> 5) & 0x01;
                const uint8_t line_offset = data[0] & 0x1F;
                data++; size--; length--;
                disp << margin << "Field parity: " << int(field_parity) << ", line offset: " << int(line_offset) << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Associated data", data, length, margin);
            data += length; size -= length;
        }
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto it1 = services.begin(); it1 != services.end(); ++it1) {
        buf.putUInt8(it1->data_service_id);
        buf.pushWriteSequenceWithLeadingLength(8); // data_service_descriptor_length
        if (it1->hasReservedBytes()) {
            buf.putBytes(it1->reserved);
        }
        else {
            for (auto it2 = it1->fields.begin(); it2 != it1->fields.end(); ++it2) {
                buf.putBits(0xFF, 2);
                buf.putBit(it2->field_parity);
                buf.putBits(it2->line_offset, 5);
            }
        }
        buf.popState(); // update data_service_descriptor_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Service service(buf.getUInt8());
        buf.pushReadSizeFromLength(8); // data_service_descriptor_length
        if (service.hasReservedBytes()) {
            buf.getBytes(service.reserved);
        }
        else {
            while (buf.canRead()) {
                Field fd;
                buf.skipBits(2);
                fd.field_parity = buf.getBit();
                fd.line_offset = buf.getBits<uint8_t>(5);
                service.fields.push_back(fd);
            }
        }
        services.push_back(service);
        buf.popState(); // data_service_descriptor_length
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = services.begin(); it1 != services.end(); ++it1) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"data_service_id", it1->data_service_id);
        if (it1->hasReservedBytes()) {
            e->addHexaTextChild(u"reserved", it1->reserved, true);
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

bool ts::VBIDataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector srv;
    bool ok = element->getChildren(srv, u"service");

    for (size_t srvIndex = 0; ok && srvIndex < srv.size(); ++srvIndex) {
        Service service;
        xml::ElementVector fld;
        ok = srv[srvIndex]->getIntAttribute<uint8_t>(service.data_service_id, u"data_service_id", true) &&
             srv[srvIndex]->getChildren(fld, u"field") &&
             srv[srvIndex]->getHexaTextChild(service.reserved, u"reserved", false);

        if (ok) {
            if (service.hasReservedBytes()) {
                if (!fld.empty()) {
                    element->report().error(u"no <field> allowed in <service>, line %d, when data_service_id='%d'", {srv[srvIndex]->lineNumber(), service.data_service_id});
                    ok = false;
                }
            }
            else {
                if (!service.reserved.empty()) {
                    element->report().error(u"no <reserved> allowed in <service>, line %d, when data_service_id='%d'", {srv[srvIndex]->lineNumber(), service.data_service_id});
                    ok = false;
                }
            }
        }

        for (size_t fldIndex = 0; ok && fldIndex < fld.size(); ++fldIndex) {
            Field field;
            ok = fld[fldIndex]->getBoolAttribute(field.field_parity, u"field_parity", false, false) &&
                 fld[fldIndex]->getIntAttribute<uint8_t>(field.line_offset, u"line_offset", false, 0x00, 0x00, 0x1F);
            service.fields.push_back(field);
        }
        services.push_back(service);
    }
    return ok;
}
