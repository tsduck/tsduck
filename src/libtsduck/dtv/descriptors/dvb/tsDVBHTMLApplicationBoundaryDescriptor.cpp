//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBHTMLApplicationBoundaryDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dvb_html_application_boundary_descriptor"
#define MY_CLASS    ts::DVBHTMLApplicationBoundaryDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_HTML_APP_BOUND, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationBoundaryDescriptor::DVBHTMLApplicationBoundaryDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DVBHTMLApplicationBoundaryDescriptor::clearContent()
{
    label.clear();
    regular_expression.clear();
}

ts::DVBHTMLApplicationBoundaryDescriptor::DVBHTMLApplicationBoundaryDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBHTMLApplicationBoundaryDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationBoundaryDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(label);
    buf.putString(regular_expression);
}

void ts::DVBHTMLApplicationBoundaryDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getStringWithByteLength(label);
    buf.getString(regular_expression);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationBoundaryDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Label: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Regexp: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationBoundaryDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"label", label);
    root->setAttribute(u"regular_expression", regular_expression);
}

bool ts::DVBHTMLApplicationBoundaryDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(label, u"label", true) &&
           element->getAttribute(regular_expression, u"regular_expression", true);
}
