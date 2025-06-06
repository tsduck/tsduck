//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBJApplicationLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dvb_j_application_location_descriptor"
#define MY_CLASS    ts::DVBJApplicationLocationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_DVBJ_APP_LOC, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBJApplicationLocationDescriptor::DVBJApplicationLocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DVBJApplicationLocationDescriptor::DVBJApplicationLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBJApplicationLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DVBJApplicationLocationDescriptor::clearContent()
{
    base_directory.clear();
    classpath_extension.clear();
    initial_class.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putStringWithByteLength(base_directory);
    buf.putStringWithByteLength(classpath_extension);
    buf.putString(initial_class);
}

void ts::DVBJApplicationLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getStringWithByteLength(base_directory);
    buf.getStringWithByteLength(classpath_extension);
    buf.getString(initial_class);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp << margin << "Base directory: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    disp << margin << "Classpath ext: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    disp << margin << "Initial class: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"base_directory", base_directory);
    root->setAttribute(u"classpath_extension", classpath_extension);
    root->setAttribute(u"initial_class", initial_class);
}

bool ts::DVBJApplicationLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(base_directory, u"base_directory", true) &&
           element->getAttribute(classpath_extension, u"classpath_extension", true) &&
           element->getAttribute(initial_class, u"initial_class", true);
}
