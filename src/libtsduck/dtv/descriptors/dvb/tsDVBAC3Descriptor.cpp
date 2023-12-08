//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBAC3Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DVB_AC3_descriptor"
#define MY_XML_NAME_LEGACY u"AC3_descriptor"
#define MY_CLASS ts::DVBAC3Descriptor
#define MY_DID ts::DID_AC3
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBAC3Descriptor::DVBAC3Descriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY)
{
}

void ts::DVBAC3Descriptor::clearContent()
{
    component_type.reset();
    bsid.reset();
    mainid.reset();
    asvc.reset();
    additional_info.clear();
}

ts::DVBAC3Descriptor::DVBAC3Descriptor(DuckContext& duck, const Descriptor& desc) :
    DVBAC3Descriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(component_type.has_value());
    buf.putBit(bsid.has_value());
    buf.putBit(mainid.has_value());
    buf.putBit(asvc.has_value());
    buf.putBits(0, 4); // reserved bits are zero here
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
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::deserializePayload(PSIBuffer& buf)
{
    const bool component_type_flag = buf.getBool();
    const bool bsid_flag = buf.getBool();
    const bool mainid_flag = buf.getBool();
    const bool asvc_flag = buf.getBool();
    buf.skipBits(4);
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
    buf.getBytes(additional_info);
}


//----------------------------------------------------------------------------
// Name of an AC-3 Component Type.
//----------------------------------------------------------------------------

ts::UString ts::DVBAC3Descriptor::ComponentTypeName(uint8_t type, NamesFlags flags)
{
    UString s((type & 0x80) != 0 ? u"Enhanced AC-3" : u"AC-3");

    s += (type & 0x40) != 0 ? u", full" : u", combined";

    switch (type & 0x38) {
        case 0x00: s += u", complete main"; break;
        case 0x08: s += u", music and effects"; break;
        case 0x10: s += u", visually impaired"; break;
        case 0x18: s += u", hearing impaired"; break;
        case 0x20: s += u", dialogue"; break;
        case 0x28: s += u", commentary"; break;
        case 0x30: s += u", emergency"; break;
        case 0x38: s += (type & 0x40) ? u", karaoke" : u", voiceover"; break;
        default: assert(false); // unreachable
    }

    switch (type & 0x07) {
        case 0: s += u", mono"; break;
        case 1: s += u", 1+1 channel"; break;
        case 2: s += u", 2 channels"; break;
        case 3: s += u", 2 channels dolby surround"; break;
        case 4: s += u", multichannel > 2"; break;
        case 5: s += u", multichannel > 5.1"; break;
        case 6: s += u", multiple substreams"; break;
        case 7: s += u", reserved"; break;
        default: assert(false); // unreachable
    }

    return NamesFile::Formatted(type, s, flags, 8);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const bool component_type_flag = buf.getBool();
        const bool bsid_flag = buf.getBool();
        const bool mainid_flag = buf.getBool();
        const bool asvc_flag = buf.getBool();
        buf.skipBits(4);
        if (component_type_flag && buf.canReadBytes(1)) {
            disp << margin << "Component type: " << ComponentTypeName(buf.getUInt8(), NamesFlags::FIRST) << std::endl;
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
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalIntAttribute(u"component_type", component_type, true);
    root->setOptionalIntAttribute(u"bsid", bsid, true);
    root->setOptionalIntAttribute(u"mainid", mainid, true);
    root->setOptionalIntAttribute(u"asvc", asvc, true);
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBAC3Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getOptionalIntAttribute(component_type, u"component_type") &&
           element->getOptionalIntAttribute(bsid, u"bsid") &&
           element->getOptionalIntAttribute(mainid, u"mainid") &&
           element->getOptionalIntAttribute(asvc, u"asvc") &&
           element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 8);
}


//----------------------------------------------------------------------------
// These descriptors shall be merged when present in the same list.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::DVBAC3Descriptor::duplicationMode() const
{
    return DescriptorDuplication::MERGE;
}

bool ts::DVBAC3Descriptor::merge(const AbstractDescriptor& desc)
{
    const DVBAC3Descriptor* other = dynamic_cast<const DVBAC3Descriptor*>(&desc);
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
        if (additional_info.empty()) {
            additional_info = other->additional_info;
        }
        return true;
    }
}
