//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBHTMLApplicationLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dvb_html_application_location_descriptor"
#define MY_CLASS    ts::DVBHTMLApplicationLocationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_HTML_APP_LOC, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationLocationDescriptor::DVBHTMLApplicationLocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DVBHTMLApplicationLocationDescriptor::clearContent()
{
    physical_root.clear();
    initial_path.clear();
}

ts::DVBHTMLApplicationLocationDescriptor::DVBHTMLApplicationLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBHTMLApplicationLocationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(physical_root);
    buf.putString(initial_path);
}

void ts::DVBHTMLApplicationLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getStringWithByteLength(physical_root);
    buf.getString(initial_path);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Physical root: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Initial path: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"physical_root", physical_root);
    root->setAttribute(u"initial_path", initial_path);
}

bool ts::DVBHTMLApplicationLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(physical_root, u"physical_root", true) &&
           element->getAttribute(initial_path, u"initial_path", true);
}
