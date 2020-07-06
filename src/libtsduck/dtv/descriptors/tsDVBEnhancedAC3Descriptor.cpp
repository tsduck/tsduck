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

#include "tsDVBEnhancedAC3Descriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY),
    component_type(),
    bsid(),
    mainid(),
    asvc(),
    mixinfoexists(false),
    substream1(),
    substream2(),
    substream3(),
    additional_info()
{
}

ts::DVBEnhancedAC3Descriptor::DVBEnhancedAC3Descriptor(DuckContext& duck, const Descriptor& desc) :
    DVBEnhancedAC3Descriptor()
{
    deserialize(duck, desc);
}

void ts::DVBEnhancedAC3Descriptor::clearContent()
{
    component_type.clear();
    bsid.clear();
    mainid.clear();
    asvc.clear();
    mixinfoexists = false;
    substream1.clear();
    substream2.clear();
    substream3.clear();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Merge inside this object missing information which can be found in other object
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::merge(const DVBEnhancedAC3Descriptor& other)
{
    if (!component_type.set()) {
        component_type = other.component_type;
    }
    if (!bsid.set()) {
        bsid = other.bsid;
    }
    if (!mainid.set()) {
        mainid = other.mainid;
    }
    if (!asvc.set()) {
        asvc = other.asvc;
    }
    mixinfoexists = mixinfoexists || other.mixinfoexists;
    if (!substream1.set()) {
        substream1 = other.substream1;
    }
    if (!substream2.set()) {
        substream2 = other.substream2;
    }
    if (!substream3.set()) {
        substream3 = other.substream3;
    }
    if (additional_info.empty()) {
        additional_info = other.additional_info;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((component_type.set() ? 0x80 : 0x00) |
                     (bsid.set()           ? 0x40 : 0x00) |
                     (mainid.set()         ? 0x20 : 0x00) |
                     (asvc.set()           ? 0x10 : 0x00) |
                     (mixinfoexists        ? 0x08 : 0x00) |
                     (substream1.set()     ? 0x04 : 0x00) |
                     (substream2.set()     ? 0x02 : 0x00) |
                     (substream3.set()     ? 0x01 : 0x00));
    if (component_type.set()) {
        bbp->appendUInt8(component_type.value());
    }
    if (bsid.set()) {
        bbp->appendUInt8(bsid.value());
    }
    if (mainid.set()) {
        bbp->appendUInt8(mainid.value());
    }
    if (asvc.set()) {
        bbp->appendUInt8(asvc.value());
    }
    if (substream1.set()) {
        bbp->appendUInt8(substream1.value());
    }
    if (substream2.set()) {
        bbp->appendUInt8(substream2.value());
    }
    if (substream3.set()) {
        bbp->appendUInt8(substream3.value());
    }
    bbp->append(additional_info);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 1;

    component_type.clear();
    bsid.clear();
    mainid.clear();
    asvc.clear();
    substream1.clear();
    substream2.clear();
    substream3.clear();
    additional_info.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        const uint8_t flags = *data;
        mixinfoexists = (flags & 0x08) != 0;
        data++; size--;
        if ((flags & 0x80) != 0 && size >= 1) {
            component_type = *data;
            data++; size--;
        }
        if ((flags & 0x40) != 0 && size >= 1) {
            bsid = *data;
            data++; size--;
        }
        if ((flags & 0x20) != 0 && size >= 1) {
            mainid = *data;
            data++; size--;
        }
        if ((flags & 0x10) != 0 && size >= 1) {
            asvc = *data;
            data++; size--;
        }
        if ((flags & 0x04) != 0 && size >= 1) {
            substream1 = *data;
            data++; size--;
        }
        if ((flags & 0x02) != 0 && size >= 1) {
            substream2 = *data;
            data++; size--;
        }
        if ((flags & 0x01) != 0 && size >= 1) {
            substream3 = *data;
            data++; size--;
        }
        additional_info.copy (data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBEnhancedAC3Descriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        uint8_t flags = data[0];
        data++; size--;

        if ((flags & 0x80) && size >= 1) { // component_type
            uint8_t type = data[0];
            data++; size--;
            strm << margin << "Component type: " << names::AC3ComponentType(type, names::FIRST) << std::endl;
        }
        if ((flags & 0x40) && size >= 1) { // bsid
            uint8_t bsid = data[0];
            data++; size--;
            strm << margin << UString::Format(u"AC-3 coding version: %d (0x%X)", {bsid, bsid}) << std::endl;
        }
        if ((flags & 0x20) && size >= 1) { // mainid
            uint8_t mainid = data[0];
            data++; size--;
            strm << margin << UString::Format(u"Main audio service id: %d (0x%X)", {mainid, mainid}) << std::endl;
        }
        if ((flags & 0x10) && size >= 1) { // asvc
            uint8_t asvc = data[0];
            data++; size--;
            strm << margin << UString::Format(u"Associated to: 0x%X", {asvc}) << std::endl;
        }
        if (flags & 0x08) {
            strm << margin << "Substream 0: Mixing control metadata" << std::endl;
        }
        if ((flags & 0x04) && size >= 1) { // substream1
            uint8_t type = data[0];
            data++; size--;
            strm << margin << "Substream 1: " << names::AC3ComponentType(type, names::FIRST) << std::endl;
        }
        if ((flags & 0x02) && size >= 1) { // substream2
            uint8_t type = data[0];
            data++; size--;
            strm << margin << "Substream 2: " << names::AC3ComponentType(type, names::FIRST) << std::endl;
        }
        if ((flags & 0x01) && size >= 1) { // substream3
            uint8_t type = data[0];
            data++; size--;
            strm << margin << "Substream 3: " << names::AC3ComponentType(type, names::FIRST) << std::endl;
        }
        display.displayPrivateData(u"Additional information", data, size, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
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
