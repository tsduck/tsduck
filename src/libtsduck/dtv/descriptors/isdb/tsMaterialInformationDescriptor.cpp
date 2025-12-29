//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMaterialInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"material_information_descriptor"
#define MY_CLASS    ts::MaterialInformationDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_MATERIAL_INFO, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MaterialInformationDescriptor::MaterialInformationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::MaterialInformationDescriptor::MaterialInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    MaterialInformationDescriptor()
{
    deserialize(duck, desc);
}

void ts::MaterialInformationDescriptor::clearContent()
{
    descriptor_number = 0;
    last_descriptor_number = 0;
    material.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MaterialInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(descriptor_number, 4);
    buf.putBits(last_descriptor_number, 4);
    buf.putUInt8(uint8_t(material.size()));
    for (const auto& mat : material) {
        buf.putUInt8(mat.material_type);
        buf.putStringWithByteLength(mat.material_name);
        buf.putUInt8(mat.material_code_type);
        buf.putStringWithByteLength(mat.material_code);
        buf.putBit(mat.material_period.has_value());
        buf.putReserved(7);
        if (mat.material_period.has_value()) {
            buf.putSecondsBCD(mat.material_period.value());
        }
        buf.putUInt8(mat.material_url_type);
        buf.putStringWithByteLength(mat.material_url);
        buf.putUInt8(uint8_t(mat.reserved.size()));
        buf.putBytes(mat.reserved);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MaterialInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(descriptor_number, 4);
    buf.getBits(last_descriptor_number, 4);
    const size_t number_of_material_set = buf.getUInt8();
    for (size_t i = 0; i < number_of_material_set; ++i) {
        Material mat;
        mat.material_type = buf.getUInt8();
        buf.getStringWithByteLength(mat.material_name);
        mat.material_code_type = buf.getUInt8();
        buf.getStringWithByteLength(mat.material_code);
        const bool material_period_flag = buf.getBool();
        buf.skipReservedBits(7);
        if (material_period_flag) {
            mat.material_period.emplace(0);
            buf.getSecondsBCD(mat.material_period.value());
        }
        mat.material_url_type = buf.getUInt8();
        buf.getStringWithByteLength(mat.material_url);
        buf.getBytes(mat.reserved, buf.getUInt8());
        material.push_back(mat);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MaterialInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Descriptor number: " << buf.getBits<uint16_t>(4);
        disp << ", last: " << buf.getBits<uint16_t>(4) << std::endl;
        const size_t number_of_material_set = buf.getUInt8();
        disp << margin << "Number of material sets: " << number_of_material_set << std::endl;
        for (size_t i = 0; i < number_of_material_set && buf.canReadBytes(2); ++i) {
            disp << margin << UString::Format(u"- Material type: %n", buf.getUInt8()) << std::endl;
            disp << margin << "  Material name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            if (!buf.canReadBytes(2)) {
                break;
            }
            disp << margin << "  Material code type: " << DataName(MY_XML_NAME, u"material_code_type", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME)  << std::endl;
            disp << margin << "  Material code: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            if (!buf.canReadBytes(2)) {
                break;
            }
            const bool material_period_flag = buf.getBool();
            buf.skipReservedBits(7);
            if (material_period_flag && buf.canReadBytes(3)) {
                const int hour = buf.getBCD<int>(2);
                const int min = buf.getBCD<int>(2);
                const int sec = buf.getBCD<int>(2);
                disp << margin << UString::Format(u"  Material period: %02d:%02d:%02d", hour, min, sec) << std::endl;
            }
            if (!buf.canReadBytes(2)) {
                break;
            }
            disp << margin << UString::Format(u"  Material URL type: %n", buf.getUInt8()) << std::endl;
            disp << margin << "  Material URL: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            if (buf.canReadBytes(1)) {
                disp.displayPrivateData(u"Reserved", buf, buf.getUInt8(), margin + u"  ");
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MaterialInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_number", descriptor_number);
    root->setIntAttribute(u"last_descriptor_number", last_descriptor_number);
    for (const auto& mat : material) {
        xml::Element* e = root->addElement(u"material");
        e->setIntAttribute(u"material_type", mat.material_type, true);
        e->setAttribute(u"material_name", mat.material_name);
        e->setIntAttribute(u"material_code_type", mat.material_code_type, true);
        e->setAttribute(u"material_code", mat.material_code);
        if (mat.material_period.has_value()) {
            e->setTimeAttribute(u"material_period", mat.material_period.value());
        }
        e->setIntAttribute(u"material_url_type", mat.material_url_type, true);
        e->setAttribute(u"material_url", mat.material_url);
        e->addHexaTextChild(u"reserved_future_use", mat.reserved, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MaterialInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xmat;
    bool ok =
        element->getIntAttribute(descriptor_number, u"descriptor_number", true, 0, 0x00, 0x0F) &&
        element->getIntAttribute(last_descriptor_number, u"last_descriptor_number", true, 0, 0x00, 0x0F) &&
        element->getChildren(xmat, u"material");

    for (const auto& xm : xmat) {
        Material mat;
        ok = xm->getIntAttribute(mat.material_type, u"material_type", true) &&
             xm->getAttribute(mat.material_name, u"material_name", true) &&
             xm->getIntAttribute(mat.material_code_type, u"material_code_type", true) &&
             xm->getAttribute(mat.material_code, u"material_code", true) &&
             xm->getOptionalTimeAttribute(mat.material_period, u"material_period") &&
             xm->getIntAttribute(mat.material_url_type, u"material_url_type", true) &&
             xm->getAttribute(mat.material_url, u"material_url", true) &&
             xm->getHexaTextChild(mat.reserved, u"reserved_future_use") &&
             ok;
        material.push_back(mat);
    }
    return ok;
}
