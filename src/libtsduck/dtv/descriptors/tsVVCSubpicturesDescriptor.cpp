//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVVCSubpicturesDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"vvc_subpictures_descriptor"
#define MY_CLASS ts::VVCSubpicturesDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_VVC_SUBPICTURES
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VVCSubpicturesDescriptor::VVCSubpicturesDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::VVCSubpicturesDescriptor::clearContent()
{
    default_service_mode = false;
    component_tag.clear();
    vvc_subpicture_id.clear();
    processing_mode = 0;
    service_description.erase();
}

ts::VVCSubpicturesDescriptor::VVCSubpicturesDescriptor(DuckContext& duck, const Descriptor& desc) :
    VVCSubpicturesDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::VVCSubpicturesDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VVCSubpicturesDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(default_service_mode);
    bool service_description_present = (service_description.length() > 0);
    buf.putBit(service_description_present);
    const size_t number_of_vvc_subpictures = std::min<size_t>(0x3F, std::min(component_tag.size(), vvc_subpicture_id.size()));
    buf.putBits(number_of_vvc_subpictures, 6);
    for (size_t i = 0; i < number_of_vvc_subpictures; i++) {
        buf.putUInt8(component_tag[i]);
        buf.putUInt8(vvc_subpicture_id[i]);
    }
    buf.putBits(0, 5); // reserved bits are zero here
    buf.putBits(processing_mode, 3);
    if (service_description_present) {
        buf.putStringWithByteLength(service_description);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VVCSubpicturesDescriptor::deserializePayload(PSIBuffer& buf)
{
    default_service_mode = buf.getBool();
    bool service_description_present = buf.getBool();
    size_t number_of_vvc_subpictures = 0;
    buf.getBits(number_of_vvc_subpictures, 6);
    for (size_t i = 0; i < number_of_vvc_subpictures; i++) {
        component_tag.push_back(buf.getUInt8());
        vvc_subpicture_id.push_back(buf.getUInt8());
    }
    buf.skipBits(5);
    buf.getBits(processing_mode, 3);
    if (service_description_present) {
        uint8_t service_description_length = buf.getUInt8();
        buf.getString(service_description, service_description_length);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VVCSubpicturesDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Default service mode: " << UString::TrueFalse(buf.getBool());
        const bool sdpresent = buf.getBool();
        disp << ", service description present: " << UString::TrueFalse(sdpresent) << std::endl;
        uint8_t num_subpics = 0;
        buf.getBits(num_subpics, 6);
        for (uint8_t i = 0; i < num_subpics; i++) {
            uint8_t tag = buf.getUInt8();
            uint8_t id = buf.getUInt8();
            disp << margin << UString::Format(u"subpicture[%d] component_tag: %d, vvc_subpicture_id: %d", { i, tag, id }) << std::endl;
        }
        buf.skipReservedBits(5, 0);
        disp << margin << UString::Format(u"Processing mode: %d", {buf.getBits<uint16_t>(3)}) << std::endl;
        if (sdpresent) {
            disp << margin << "Service description: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VVCSubpicturesDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"default_service_mode", default_service_mode);
    const size_t number_of_vvc_subpictures = std::min<size_t>(0x3F, std::min(component_tag.size(), vvc_subpicture_id.size()));
    for (size_t i = 0; i < number_of_vvc_subpictures; i++) {
        ts::xml::Element* element = root->addElement(u"subpicture");
        element->setIntAttribute(u"component_tag", component_tag[i]);
        element->setIntAttribute(u"subpicture_id", vvc_subpicture_id[i]);
    }
    root->setIntAttribute(u"processing_mode", processing_mode);
    root->setAttribute(u"service_description", service_description, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VVCSubpicturesDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getBoolAttribute(default_service_mode, u"default_service_mode", true) &&
        element->getIntAttribute(processing_mode, u"processing_mode", true, 0, 0, 7) &&
        element->getAttribute(service_description, u"service_description", false) &&
        element->getChildren(children, u"subpicture", 0, 0x3F);
    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint8_t ctag, sp_id;
        ok = children[i]->getIntAttribute(ctag, u"component_tag", true, 0, 0, 0xFF) &&
             children[i]->getIntAttribute(sp_id, u"subpicture_id", true, 0, 0, 0xFF);
        component_tag.push_back(ctag);
        vvc_subpicture_id.push_back(sp_id);
    }
    return ok;
}
