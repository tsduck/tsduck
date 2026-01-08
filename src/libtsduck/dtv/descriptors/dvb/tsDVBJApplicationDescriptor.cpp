//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBJApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dvb_j_application_descriptor"
#define MY_CLASS    ts::DVBJApplicationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_DVBJ_APP, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBJApplicationDescriptor::DVBJApplicationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DVBJApplicationDescriptor::DVBJApplicationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBJApplicationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DVBJApplicationDescriptor::clearContent()
{
    parameters.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : parameters) {
        buf.putStringWithByteLength(it);
    }
}

void ts::DVBJApplicationDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        parameters.push_back(buf.getStringWithByteLength());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBJApplicationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(1)) {
        disp << margin << "Parameter: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : parameters) {
        root->addElement(u"parameter")->setAttribute(u"value", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBJApplicationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& child : element->children(u"parameter", &ok)) {
        ok = child.getAttribute(parameters.emplace_back(), u"value", true);
    }
    return ok;
}
