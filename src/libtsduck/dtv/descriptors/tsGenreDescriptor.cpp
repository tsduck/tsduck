//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGenreDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"genre_descriptor"
#define MY_CLASS ts::GenreDescriptor
#define MY_DID ts::DID_ATSC_GENRE
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::GenreDescriptor::GenreDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::GenreDescriptor::GenreDescriptor(DuckContext& duck, const Descriptor& desc) :
    GenreDescriptor()
{
    deserialize(duck, desc);
}

void ts::GenreDescriptor::clearContent()
{
    attributes.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 3);
    buf.putBits(attributes.size(), 5);
    buf.putBytes(attributes);
}

void ts::GenreDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(3);
    const size_t count = buf.getBits<size_t>(5);
    buf.getBytes(attributes, count);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::GenreDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(3);
        size_t count = buf.getBits<size_t>(5);
        disp << margin << UString::Format(u"Attribute count: %d", {count}) << std::endl;
        while (count-- > 0 && buf.canReadBytes(1)) {
            disp << margin << " - Attribute: " << DataName(MY_XML_NAME, u"code", buf.getUInt8(), NamesFlags::FIRST) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::GenreDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (size_t i = 0; i < attributes.size(); ++i) {
        root->addElement(u"attribute")->setIntAttribute(u"value", attributes[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::GenreDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"attribute", 0, 0x1F);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint8_t attr = 0;
        ok = children[i]->getIntAttribute(attr, u"value", true);
        attributes.push_back(attr);
    }
    return ok;
}
