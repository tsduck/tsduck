//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEASMetadataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"EAS_metadata_descriptor"
#define MY_CLASS ts::EASMetadataDescriptor
#define MY_DID ts::DID_EAS_METADATA
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EASMetadataDescriptor::EASMetadataDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::EASMetadataDescriptor::EASMetadataDescriptor(DuckContext& duck, const Descriptor& desc) :
    EASMetadataDescriptor()
{
    deserialize(duck, desc);
}

void ts::EASMetadataDescriptor::clearContent()
{
    fragment_number = 1;
    XML_fragment.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASMetadataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(fragment_number);
    buf.putUTF8WithLength(XML_fragment);
}

void ts::EASMetadataDescriptor::deserializePayload(PSIBuffer& buf)
{
    fragment_number = buf.getUInt8();
    buf.getUTF8WithLength(XML_fragment);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASMetadataDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Fragment number: " << int(buf.getUInt8()) << std::endl;
        disp << margin << "XML fragment: \"" << buf.getUTF8WithLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASMetadataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"fragment_number", fragment_number, false);
    root->addText(XML_fragment, true);
}

bool ts::EASMetadataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(fragment_number, u"fragment_number", false, 1, 1, 255) &&
           element->getText(XML_fragment, false, 0, 253);
}
