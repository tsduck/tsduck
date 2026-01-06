//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAstraBouquetListDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"astra_bouquet_list_descriptor"
#define MY_CLASS    ts::AstraBouquetListDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_ASTRA_BOUQUET_LIST, ts::PDS_ASTRA)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AstraBouquetListDescriptor::AstraBouquetListDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::AstraBouquetListDescriptor::clearContent()
{
    bouquet_names.clear();
}

ts::AstraBouquetListDescriptor::AstraBouquetListDescriptor(DuckContext& duck, const Descriptor& desc) :
    AstraBouquetListDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AstraBouquetListDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& name : bouquet_names) {
        buf.putStringWithByteLength(name);
    }
}

void ts::AstraBouquetListDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        bouquet_names.push_back(buf.getStringWithByteLength());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AstraBouquetListDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canRead()) {
        disp << margin << "Bouquet name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AstraBouquetListDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& name : bouquet_names) {
        root->addElement(u"bouquet")->setAttribute(u"name", name);
    }
}

bool ts::AstraBouquetListDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& it : element->children(u"bouquet", &ok)) {
        auto& name(bouquet_names.emplace_back());
        ok = it.getAttribute(name, u"name");
    }
    return ok;
}
