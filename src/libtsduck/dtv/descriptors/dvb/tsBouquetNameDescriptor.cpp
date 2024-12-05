//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBouquetNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"bouquet_name_descriptor"
#define MY_CLASS    ts::BouquetNameDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_BOUQUET_NAME, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::BouquetNameDescriptor::BouquetNameDescriptor(const UString& name_) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    name(name_)
{
}

ts::BouquetNameDescriptor::BouquetNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    name()
{
    deserialize(duck, desc);
}

void ts::BouquetNameDescriptor::clearContent()
{
    name.clear();
}

ts::DescriptorDuplication ts::BouquetNameDescriptor::duplicationMode() const
{
    return DescriptorDuplication::REPLACE;
}


//----------------------------------------------------------------------------
// Binary serialization
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(name);
}

void ts::BouquetNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Name: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"bouquet_name", name);
}

bool ts::BouquetNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(name, u"bouquet_name", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
