//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBEnhancedAC3Descriptor.h"
#include "tsDVBAC3Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DVB_enhanced_AC3_descriptor"
#define MY_XML_NAME_LEGACY u"enhanced_AC3_descriptor"
#define MY_CLASS ts::DVBEnhancedAC3Descriptor
#define MY_DID ts::DID_ENHANCED_AC3
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBEnhancedAC3Descriptor::DVBEnhancedAC3Descriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY)
{
}

ts::DVBEnhancedAC3Descriptor::DVBEnhancedAC3Descriptor(DuckContext& duck, const Descriptor& desc) :
    DVBEnhancedAC3Descriptor()
{
    deserialize(duck, desc);
}

void ts::DVBEnhancedAC3Descriptor::clearContent()
{
    component_type.reset();
    bsid.reset();
    mainid.reset();
    asvc.reset();
    mixinfoexists = false;
    substream1.reset();
    substream2.reset();
    substream3.reset();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(component_type.has_value());
    buf.putBit(bsid.has_value());
    buf.putBit(mainid.has_value());
    buf.putBit(asvc.has_value());
    buf.putBit(mixinfoexists);
    buf.putBit(substream1.has_value());
    buf.putBit(substream2.has_value());
    buf.putBit(substream3.has_value());
    if (component_type.has_value()) {
        buf.putUInt8(component_type.value());
    }
    if (bsid.has_value()) {
        buf.putUInt8(bsid.value());
    }
    if (mainid.has_value()) {
        buf.putUInt8(mainid.value());
    }
    if (asvc.has_value()) {
        buf.putUInt8(asvc.value());
    }
    if (substream1.has_value()) {
        buf.putUInt8(substream1.value());
    }
    if (substream2.has_value()) {
        buf.putUInt8(substream2.value());
    }
    if (substream3.has_value()) {
        buf.putUInt8(substream3.value());
    }
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::deserializePayload(PSIBuffer& buf)
{
    const bool component_type_flag = buf.getBool();
    const bool bsid_flag = buf.getBool();
    const bool mainid_flag = buf.getBool();
    const bool asvc_flag = buf.getBool();
    mixinfoexists = buf.getBool();
    const bool substream1_flag = buf.getBool();
    const bool substream2_flag = buf.getBool();
    const bool substream3_flag = buf.getBool();

    if (component_type_flag) {
        component_type = buf.getUInt8();
    }
    if (bsid_flag) {
        bsid = buf.getUInt8();
    }
    if (mainid_flag) {
        mainid = buf.getUInt8();
    }
    if (asvc_flag) {
        asvc = buf.getUInt8();
    }
    if (substream1_flag) {
        substream1 = buf.getUInt8();
    }
    if (substream2_flag) {
        substream2 = buf.getUInt8();
    }
    if (substream3_flag) {
        substream3 = buf.getUInt8();
    }
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const bool component_type_flag = buf.getBool();
        const bool bsid_flag = buf.getBool();
        const bool mainid_flag = buf.getBool();
        const bool asvc_flag = buf.getBool();
        const bool mixinfoexists = buf.getBool();
        const bool substream1_flag = buf.getBool();
        const bool substream2_flag = buf.getBool();
        const bool substream3_flag = buf.getBool();

        if (component_type_flag && buf.canReadBytes(1)) {
            disp << margin << "Component type: " << DVBAC3Descriptor::ComponentTypeName(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        }
        if (bsid_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"AC-3 coding version: %d (0x%<X)", {buf.getUInt8()}) << std::endl;
        }
        if (mainid_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Main audio service id: %d (0x%<X)", {buf.getUInt8()}) << std::endl;
        }
        if (asvc_flag && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Associated to: 0x%X", {buf.getUInt8()}) << std::endl;
        }
        if (mixinfoexists) {
            disp << margin << "Substream 0: Mixing control metadata" << std::endl;
        }
        if (substream1_flag && buf.canReadBytes(1)) {
            disp << margin << "Substream 1: " << DVBAC3Descriptor::ComponentTypeName(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        }
        if (substream2_flag && buf.canReadBytes(1)) {
            disp << margin << "Substream 2: " << DVBAC3Descriptor::ComponentTypeName(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        }
        if (substream3_flag && buf.canReadBytes(1)) {
            disp << margin << "Substream 3: " << DVBAC3Descriptor::ComponentTypeName(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        }
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"mixinfoexists", mixinfoexists);
    root->setOptionalIntAttribute(u"component_type", component_type, true);
    root->setOptionalIntAttribute(u"bsid", bsid, true);
    root->setOptionalIntAttribute(u"mainid", mainid, true);
    root->setOptionalIntAttribute(u"asvc", asvc, true);
    root->setOptionalIntAttribute(u"substream1", substream1, true);
    root->setOptionalIntAttribute(u"substream2", substream2, true);
    root->setOptionalIntAttribute(u"substream3", substream3, true);
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBEnhancedAC3Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(mixinfoexists, u"mixinfoexists", true) &&
            element->getOptionalIntAttribute(component_type, u"component_type") &&
            element->getOptionalIntAttribute(bsid, u"bsid") &&
            element->getOptionalIntAttribute(mainid, u"mainid") &&
            element->getOptionalIntAttribute(asvc, u"asvc") &&
            element->getOptionalIntAttribute(substream1, u"substream1") &&
            element->getOptionalIntAttribute(substream2, u"substream2") &&
            element->getOptionalIntAttribute(substream3, u"substream3") &&
            element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 8);
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::DVBEnhancedAC3Descriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::DVBEnhancedAC3Descriptor::merge(const AbstractDescriptor& desc)
{
    const DVBEnhancedAC3Descriptor* other = dynamic_cast<const DVBEnhancedAC3Descriptor*>(&desc);
    if (other == nullptr) {
        return false;
    }
    else {
        if (!component_type.has_value()) {
            component_type = other->component_type;
        }
        if (!bsid.has_value()) {
            bsid = other->bsid;
        }
        if (!mainid.has_value()) {
            mainid = other->mainid;
        }
        if (!asvc.has_value()) {
            asvc = other->asvc;
        }
        mixinfoexists = mixinfoexists || other->mixinfoexists;
        if (!substream1.has_value()) {
            substream1 = other->substream1;
        }
        if (!substream2.has_value()) {
            substream2 = other->substream2;
        }
        if (!substream3.has_value()) {
            substream3 = other->substream3;
        }
        if (additional_info.empty()) {
            additional_info = other->additional_info;
        }
        return true;
    }
}
