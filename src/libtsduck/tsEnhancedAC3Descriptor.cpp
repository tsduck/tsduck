//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of an enhanced_AC-3_descriptor
//
//----------------------------------------------------------------------------

#include "tsEnhancedAC3Descriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"enhanced_AC3_descriptor"
#define MY_DID ts::DID_ENHANCED_AC3

TS_XML_DESCRIPTOR_FACTORY(ts::EnhancedAC3Descriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::EnhancedAC3Descriptor, ts::EDID(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::EnhancedAC3Descriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::EnhancedAC3Descriptor::EnhancedAC3Descriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
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
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::EnhancedAC3Descriptor::EnhancedAC3Descriptor (const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
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
    deserialize (desc, charset);
}


//----------------------------------------------------------------------------
// Merge inside this object missing information which can be found in other object
//----------------------------------------------------------------------------

void ts::EnhancedAC3Descriptor::merge (const EnhancedAC3Descriptor& other)
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

void ts::EnhancedAC3Descriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt8 ((component_type.set() ? 0x80 : 0x00) |
                      (bsid.set()           ? 0x40 : 0x00) |
                      (mainid.set()         ? 0x20 : 0x00) |
                      (asvc.set()           ? 0x10 : 0x00) |
                      (mixinfoexists        ? 0x08 : 0x00) |
                      (substream1.set()     ? 0x04 : 0x00) |
                      (substream2.set()     ? 0x02 : 0x00) |
                      (substream3.set()     ? 0x01 : 0x00));
    if (component_type.set()) {
        bbp->appendUInt8 (component_type.value());
    }
    if (bsid.set()) {
        bbp->appendUInt8 (bsid.value());
    }
    if (mainid.set()) {
        bbp->appendUInt8 (mainid.value());
    }
    if (asvc.set()) {
        bbp->appendUInt8 (asvc.value());
    }
    if (substream1.set()) {
        bbp->appendUInt8 (substream1.value());
    }
    if (substream2.set()) {
        bbp->appendUInt8 (substream2.value());
    }
    if (substream3.set()) {
        bbp->appendUInt8 (substream3.value());
    }
    bbp->append (additional_info);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EnhancedAC3Descriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 1;

    component_type.reset();
    bsid.reset();
    mainid.reset();
    asvc.reset();
    substream1.reset();
    substream2.reset();
    substream3.reset();
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

void ts::EnhancedAC3Descriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
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
        if (size > 0) {
            strm << margin << "Additional information:" << std::endl
                 << UString::Dump(data, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EnhancedAC3Descriptor::buildXML(xml::Element* root) const
{
    root->setBoolAttribute(u"mixinfoexists", mixinfoexists);
    root->setOptionalIntAttribute(u"component_type", component_type, true);
    root->setOptionalIntAttribute(u"bsid", bsid, true);
    root->setOptionalIntAttribute(u"mainid", mainid, true);
    root->setOptionalIntAttribute(u"asvc", asvc, true);
    root->setOptionalIntAttribute(u"substream1", substream1, true);
    root->setOptionalIntAttribute(u"substream2", substream2, true);
    root->setOptionalIntAttribute(u"substream3", substream3, true);
    if (!additional_info.empty()) {
        root->addElement(u"additional_info")->addHexaText(additional_info);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::EnhancedAC3Descriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getBoolAttribute(mixinfoexists, u"mixinfoexists", true) &&
        element->getOptionalIntAttribute(component_type, u"component_type") &&
        element->getOptionalIntAttribute(bsid, u"bsid") &&
        element->getOptionalIntAttribute(mainid, u"mainid") &&
        element->getOptionalIntAttribute(asvc, u"asvc") &&
        element->getOptionalIntAttribute(substream1, u"substream1") &&
        element->getOptionalIntAttribute(substream2, u"substream2") &&
        element->getOptionalIntAttribute(substream3, u"substream3") &&
        element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 8);
}
