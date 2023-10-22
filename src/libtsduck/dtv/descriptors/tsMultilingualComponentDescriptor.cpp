//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMultilingualComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"multilingual_component_descriptor"
#define MY_XML_ATTR u"description"
#define MY_CLASS ts::MultilingualComponentDescriptor
#define MY_DID ts::DID_MLINGUAL_COMPONENT

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultilingualComponentDescriptor::MultilingualComponentDescriptor() :
    AbstractMultilingualDescriptor(MY_DID, MY_XML_NAME, MY_XML_ATTR)
{
}

void ts::MultilingualComponentDescriptor::clearContent()
{
    component_tag = 0;
}

ts::MultilingualComponentDescriptor::MultilingualComponentDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultilingualComponentDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialize / deserialize / display.
//----------------------------------------------------------------------------

// Unlike other multilingual descriptors, there is a one-byte leading field in
// a multilingual_component_descriptor. So, we override all three methods,
// process the first byte and then delegate the rest to the super-class.

void ts::MultilingualComponentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(component_tag);
    AbstractMultilingualDescriptor::serializePayload(buf);
}

void ts::MultilingualComponentDescriptor::deserializePayload(PSIBuffer& buf)
{
    component_tag = buf.getUInt8();
    AbstractMultilingualDescriptor::deserializePayload(buf);
}

void ts::MultilingualComponentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        AbstractMultilingualDescriptor::DisplayDescriptor(disp, buf, margin, did, tid, pds);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::MultilingualComponentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    AbstractMultilingualDescriptor::buildXML(duck, root);
    root->setIntAttribute(u"component_tag", component_tag);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MultilingualComponentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return AbstractMultilingualDescriptor::analyzeXML(duck, element) &&
           element->getIntAttribute(component_tag, u"component_tag", true);
}
