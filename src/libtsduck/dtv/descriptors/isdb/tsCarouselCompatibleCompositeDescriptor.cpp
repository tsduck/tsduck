//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCarouselCompatibleCompositeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"carousel_compatible_composite_descriptor"
#define MY_CLASS ts::CarouselCompatibleCompositeDescriptor
#define MY_DID ts::DID_ISDB_CAROUSEL_COMP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::PrivateDVB(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CarouselCompatibleCompositeDescriptor::CarouselCompatibleCompositeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CarouselCompatibleCompositeDescriptor::clearContent()
{
    subdescs.clear();
}

ts::CarouselCompatibleCompositeDescriptor::CarouselCompatibleCompositeDescriptor(DuckContext& duck, const Descriptor& desc) :
    CarouselCompatibleCompositeDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CarouselCompatibleCompositeDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& sub : subdescs) {
        buf.putUInt8(sub.type);
        buf.putUInt8(uint8_t(sub.payload.size()));
        buf.putBytes(sub.payload);
    }
}

void ts::CarouselCompatibleCompositeDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canReadBytes(2)) {
        subdescs.emplace_back();
        subdescs.back().type = buf.getUInt8();
        buf.getBytes(subdescs.back().payload, buf.getUInt8());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CarouselCompatibleCompositeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    for (size_t index = 0; buf.canReadBytes(2); ++index) {
        const uint8_t type = buf.getUInt8();
        const size_t size = buf.getUInt8();
        disp << margin << UString::Format(u"- Subdescriptor #%d, type: %n, %d bytes", index, type, size) << std::endl;
        disp.displayPrivateData(u"Payload", buf, size, margin + u"  ");
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CarouselCompatibleCompositeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& sub : subdescs) {
        xml::Element* e = root->addElement(u"subdescriptor");
        e->setIntAttribute(u"type", sub.type, true);
        e->addHexaText(sub.payload, true);
    }
}

bool ts::CarouselCompatibleCompositeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xsub;
    bool ok = element->getChildren(xsub, u"subdescriptor");
    for (size_t i = 0; ok && i < xsub.size(); ++i) {
        subdescs.emplace_back();
        ok = xsub[i]->getIntAttribute(subdescs.back().type, u"type", true) && xsub[i]->getHexaText(subdescs.back().payload);
    }
    return ok;
}
