//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCResourceDescriptor.h"
#include "tsDSMCC.h"
#include "tsNames.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Total number of bytes to serialize the dsmccResourceDescriptor().
//----------------------------------------------------------------------------

size_t ts::DSMCCResourceDescriptor::binarySize() const
{
    // Common size.
    const size_t size = 14 + resourceDescriptorDataFields.size();

    // Add typeOwnerId and typeOwnerValue fields when resourceDescriptorType == 0xFFFF.
    return resourceDescriptorType == DSMCC_RDTYPE_TYPE_OWNER ? size + 6 : size;
}


//----------------------------------------------------------------------------
// Clear the content of the dsmccResourceDescriptor() structure.
//----------------------------------------------------------------------------

void ts::DSMCCResourceDescriptor::clear()
{
    resourceRequestId = 0;
    resourceDescriptorType = 0;
    resourceNum = 0;
    associationTag = 0;
    resourceFlags = 0;
    resourceStatus = 0;
    resourceDataFieldCount = 0;
    typeOwnerId = 0;
    typeOwnerValue = 0;
    resourceDescriptorDataFields.clear();
}


//----------------------------------------------------------------------------
// Serialize the dsmccResourceDescriptor().
//----------------------------------------------------------------------------

void ts::DSMCCResourceDescriptor::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(resourceRequestId);
    buf.putUInt16(resourceDescriptorType);
    buf.putUInt16(resourceNum);
    buf.putUInt16(associationTag);
    buf.putUInt8(resourceFlags);
    buf.putUInt8(resourceStatus);
    buf.putUInt16(uint16_t(resourceDescriptorDataFields.size()));
    buf.putUInt16(resourceDataFieldCount);
    if (resourceDescriptorType == DSMCC_RDTYPE_TYPE_OWNER) {
        buf.putUInt24(typeOwnerId);
        buf.putUInt24(typeOwnerValue);
    }
    buf.putBytes(resourceDescriptorDataFields);
}


//----------------------------------------------------------------------------
// Deserialize the dsmccResourceDescriptor().
//----------------------------------------------------------------------------

void ts::DSMCCResourceDescriptor::deserialize(PSIBuffer& buf)
{
    resourceRequestId = buf.getUInt16();
    resourceDescriptorType = buf.getUInt16();
    resourceNum = buf.getUInt16();
    associationTag = buf.getUInt16();
    resourceFlags = buf.getUInt8();
    resourceStatus = buf.getUInt8();
    const size_t resourceDescriptorDataFieldsLength = buf.getUInt16();
    resourceDataFieldCount = buf.getUInt16();
    if (resourceDescriptorType == DSMCC_RDTYPE_TYPE_OWNER) {
        typeOwnerId = buf.getUInt24();
        typeOwnerValue = buf.getUInt24();
    }
    else {
        typeOwnerId = typeOwnerValue = 0;
    }
    buf.getBytes(resourceDescriptorDataFields, resourceDescriptorDataFieldsLength);
}


//----------------------------------------------------------------------------
// A static method to display a dsmccResourceDescriptor().
//----------------------------------------------------------------------------

bool ts::DSMCCResourceDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(14)) {
        return false;
    }
    disp << margin << UString::Format(u"DSM-CC Resource descriptor: request id: %n", buf.getUInt16()) << std::endl;
    const uint16_t resourceDescriptorType = buf.getUInt16();
    disp << margin << "  Descriptor type: " << NameFromSection(u"dtv", u"DSMCC.resourceDescriptorType", resourceDescriptorType, NamesFlags::HEX_VALUE_NAME) << std::endl;
    disp << margin << UString::Format(u"  Resource number: %n", buf.getUInt16()) << std::endl;
    disp << margin << UString::Format(u"  Association tag: %n", buf.getUInt16()) << std::endl;
    disp << margin << UString::Format(u"  Resource flags: %n", buf.getUInt8()) << std::endl;
    disp << margin << UString::Format(u"  Resource status: %n", buf.getUInt8()) << std::endl;
    const size_t resourceDescriptorDataFieldsLength = buf.getUInt16();
    disp << margin << "  Data fields length: " << resourceDescriptorDataFieldsLength << " bytes" << std::endl;
    disp << margin << "  Data fields count: " << buf.getUInt16() << std::endl;
    if (resourceDescriptorType == DSMCC_RDTYPE_TYPE_OWNER) {
        disp << margin << UString::Format(u"  Type owner id: %n", buf.getUInt24()) << std::endl;
        disp << margin << UString::Format(u"  Type owner value: %n", buf.getUInt24()) << std::endl;
    }
    disp.displayPrivateData(u"Resource data fields", buf, resourceDescriptorDataFieldsLength, margin + u"  ");
    return !buf.error();
}


//----------------------------------------------------------------------------
// This method converts a dsmccResourceDescriptor() to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::DSMCCResourceDescriptor::toXML(DuckContext& duck, xml::Element* parent, const UChar* xml_name) const
{
    xml::Element* element = parent->addElement(xml_name);
    element->setIntAttribute(u"resourceRequestId", resourceRequestId, true);
    element->setIntAttribute(u"resourceDescriptorType", resourceDescriptorType, true);
    element->setIntAttribute(u"resourceNum", resourceNum, true);
    element->setIntAttribute(u"associationTag", associationTag, true);
    element->setIntAttribute(u"resourceFlags", resourceFlags, true);
    element->setIntAttribute(u"resourceStatus", resourceStatus, true);
    element->setIntAttribute(u"resourceDataFieldCount", resourceDataFieldCount);
    if (resourceDescriptorType == DSMCC_RDTYPE_TYPE_OWNER) {
        element->setIntAttribute(u"typeOwnerId", typeOwnerId, true);
        element->setIntAttribute(u"typeOwnerValue", typeOwnerValue, true);
    }
    element->addHexaTextChild(u"resourceDescriptorDataFields", resourceDescriptorDataFields, true);
    return element;
}


//----------------------------------------------------------------------------
// This method decodes an XML dsmccResourceDescriptor().
//----------------------------------------------------------------------------

bool ts::DSMCCResourceDescriptor::fromXML(DuckContext& duck, const xml::Element* parent, const UChar* xml_name)
{
    clear();

    // Get the dsmccResourceDescriptor() element.
    const xml::Element* e = parent;
    if (xml_name != nullptr) {
        xml::ElementVector children;
        if (!parent->getChildren(children, xml_name, 1, 1)) {
            return false;
        }
        e = children[0];
    }

    // Analyze the dsmccResourceDescriptor() element.
    return e->getIntAttribute(resourceRequestId, u"resourceRequestId", true) &&
           e->getIntAttribute(resourceDescriptorType, u"resourceDescriptorType", true) &&
           e->getIntAttribute(resourceNum, u"resourceNum", true) &&
           e->getIntAttribute(associationTag, u"associationTag", true) &&
           e->getIntAttribute(resourceFlags, u"resourceFlags", true) &&
           e->getIntAttribute(resourceStatus, u"resourceStatus", true) &&
           e->getIntAttribute(resourceDataFieldCount, u"resourceDataFieldCount", true) &&
           (resourceDescriptorType != DSMCC_RDTYPE_TYPE_OWNER || e->getIntAttribute(typeOwnerId, u"typeOwnerId", true, 0, 0, 0x00FFFFFF)) &&
           (resourceDescriptorType != DSMCC_RDTYPE_TYPE_OWNER || e->getIntAttribute(typeOwnerValue, u"typeOwnerValue", true, 0, 0, 0x00FFFFFF)) &&
           e->getHexaTextChild(resourceDescriptorDataFields, u"resourceDescriptorDataFields", false);
}
