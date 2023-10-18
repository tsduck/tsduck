//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSimpleApplicationBoundaryDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"simple_application_boundary_descriptor"
#define MY_CLASS ts::SimpleApplicationBoundaryDescriptor
#define MY_DID ts::DID_AIT_APP_BOUNDARY
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SimpleApplicationBoundaryDescriptor::clearContent()
{
    boundary_extension.clear();
}

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor(DuckContext& duck, const Descriptor& desc) :
    SimpleApplicationBoundaryDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uint8_t(boundary_extension.size()));
    for (const auto& it : boundary_extension) {
        buf.putStringWithByteLength(it);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::deserializePayload(PSIBuffer& buf)
{
    const size_t count = buf.getUInt8();
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        boundary_extension.push_back(buf.getStringWithByteLength());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const size_t count = buf.getUInt8();
        disp << margin << UString::Format(u"Number of prefixes: %d", {count}) << std::endl;
        for (size_t i = 0; i < count && buf.canRead(); ++i) {
            disp << margin << "Boundary extension: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : boundary_extension) {
        root->addElement(u"prefix")->setAttribute(u"boundary_extension", it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SimpleApplicationBoundaryDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"prefix");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        UString s;
        ok = children[i]->getAttribute(s, u"boundary_extension", true);
        boundary_extension.push_back(s);
    }
    return ok;
}
