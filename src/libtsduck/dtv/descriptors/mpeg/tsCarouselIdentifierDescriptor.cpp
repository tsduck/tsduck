//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCarouselIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"carousel_identifier_descriptor"
#define MY_CLASS ts::CarouselIdentifierDescriptor
#define MY_DID ts::DID_CAROUSEL_IDENTIFIER
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CarouselIdentifierDescriptor::CarouselIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CarouselIdentifierDescriptor::clearContent()
{
    carousel_id = 0;
    private_data.clear();
}

ts::CarouselIdentifierDescriptor::CarouselIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    CarouselIdentifierDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CarouselIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(carousel_id);
    buf.putBytes(private_data);
}

void ts::CarouselIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    carousel_id = buf.getUInt32();
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CarouselIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Carousel id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CarouselIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"carousel_id", carousel_id, true);
    if (!private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}

bool ts::CarouselIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(carousel_id, u"carousel_id", true) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 6);
}
