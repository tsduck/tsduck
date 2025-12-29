//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVBIDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"VBI_data_descriptor"
#define MY_CLASS    ts::VBIDataDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_VBI_DATA, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::VBIDataDescriptor::VBIDataDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
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

void ts::VBIDataDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(2)) {
        const uint8_t data_id = buf.getUInt8();
        disp << margin << "Data service id: " << DataName(MY_XML_NAME, u"ServiceId", data_id, NamesFlags::HEX_VALUE_NAME) << std::endl;
        buf.pushReadSizeFromLength(8); // data_service_descriptor_length
        if (!EntryHasReservedBytes(data_id)) {
            while (buf.canReadBytes(1)) {
                buf.skipBits(2);
                disp << margin << "Field parity: " << int(buf.getBool());
                disp << ", line offset: " << buf.getBits<uint16_t>(5) << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Associated data", buf, NPOS, margin);
        }
        buf.popState(); // data_service_descriptor_length
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VBIDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : services) {
        buf.putUInt8(it1.data_service_id);
        buf.pushWriteSequenceWithLeadingLength(8); // data_service_descriptor_length
        if (it1.hasReservedBytes()) {
            buf.putBytes(it1.reserved);
        }
        else {
            for (const auto& it2 : it1.fields) {
                buf.putBits(0xFF, 2);
                buf.putBit(it2.field_parity);
                buf.putBits(it2.line_offset, 5);
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
                fd.field_parity = buf.getBool();
                buf.getBits(fd.line_offset, 5);
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
    for (const auto& it1 : services) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"data_service_id", it1.data_service_id);
        if (it1.hasReservedBytes()) {
            e->addHexaTextChild(u"reserved", it1.reserved, true);
        }
        else {
            for (const auto& it2 : it1.fields) {
                xml::Element* f = e->addElement(u"field");
                f->setBoolAttribute(u"field_parity", it2.field_parity);
                f->setIntAttribute(u"line_offset", it2.line_offset);
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
        ok = srv[srvIndex]->getIntAttribute(service.data_service_id, u"data_service_id", true) &&
             srv[srvIndex]->getChildren(fld, u"field") &&
             srv[srvIndex]->getHexaTextChild(service.reserved, u"reserved", false);

        if (ok) {
            if (service.hasReservedBytes()) {
                if (!fld.empty()) {
                    element->report().error(u"no <field> allowed in <service>, line %d, when data_service_id='%d'", srv[srvIndex]->lineNumber(), service.data_service_id);
                    ok = false;
                }
            }
            else {
                if (!service.reserved.empty()) {
                    element->report().error(u"no <reserved> allowed in <service>, line %d, when data_service_id='%d'", srv[srvIndex]->lineNumber(), service.data_service_id);
                    ok = false;
                }
            }
        }

        for (size_t fldIndex = 0; ok && fldIndex < fld.size(); ++fldIndex) {
            Field field;
            ok = fld[fldIndex]->getBoolAttribute(field.field_parity, u"field_parity", false, false) &&
                 fld[fldIndex]->getIntAttribute(field.line_offset, u"line_offset", false, 0x00, 0x00, 0x1F);
            service.fields.push_back(field);
        }
        services.push_back(service);
    }
    return ok;
}
