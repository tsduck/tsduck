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

#include "tsDVBAC3Descriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0, MY_XML_NAME_LEGACY),
    component_type(),
    bsid(),
    mainid(),
    asvc(),
    additional_info()
{
}

void ts::DVBAC3Descriptor::clearContent()
{
    component_type.clear();
    bsid.clear();
    mainid.clear();
    asvc.clear();
    additional_info.clear();
}

ts::DVBAC3Descriptor::DVBAC3Descriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    component_type(),
    bsid(),
    mainid(),
    asvc(),
    additional_info()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Merge inside this object missing information which can be found in other object
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::merge(const DVBAC3Descriptor& other)
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
    if (additional_info.empty()) {
        additional_info = other.additional_info;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(component_type.set());
    buf.putBit(bsid.set());
    buf.putBit(mainid.set());
    buf.putBit(asvc.set());
    buf.putBits(0, 4); // reserved bits are zero here
    if (component_type.set()) {
        buf.putUInt8(component_type.value());
    }
    if (bsid.set()) {
        buf.putUInt8(bsid.value());
    }
    if (mainid.set()) {
        buf.putUInt8(mainid.value());
    }
    if (asvc.set()) {
        buf.putUInt8(asvc.value());
    }
    buf.putBytes(additional_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::deserializePayload(PSIBuffer& buf)
{
    const bool component_type_flag = buf.getBit() != 0;
    const bool bsid_flag = buf.getBit() != 0;
    const bool mainid_flag = buf.getBit() != 0;
    const bool asvc_flag = buf.getBit() != 0;
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
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBAC3Descriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 1) {
        uint8_t flags = data[0];
        data++; size--;
        if ((flags & 0x80) && size >= 1) { // component_type
            uint8_t type = data[0];
            data++; size--;
            disp << margin << "Component type: " << names::AC3ComponentType(type, names::FIRST) << std::endl;
        }
        if ((flags & 0x40) && size >= 1) { // bsid
            uint8_t bsid = data[0];
            data++; size--;
            disp << margin << UString::Format(u"AC-3 coding version: %d (0x%X)", {bsid, bsid}) << std::endl;
        }
        if ((flags & 0x20) && size >= 1) { // mainid
            uint8_t mainid = data[0];
            data++; size--;
            disp << margin << UString::Format(u"Main audio service id: %d (0x%X)", {mainid, mainid}) << std::endl;
        }
        if ((flags & 0x10) && size >= 1) { // asvc
            uint8_t asvc = data[0];
            data++; size--;
            disp << margin << UString::Format(u"Associated to: 0x%X", {asvc}) << std::endl;
        }
        disp.displayPrivateData(u"Additional information", data, size, margin);
    }
    else {
        disp.displayExtraData(data, size, margin);
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
