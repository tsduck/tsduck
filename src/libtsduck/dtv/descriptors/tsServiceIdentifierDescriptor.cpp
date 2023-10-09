//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"service_identifier_descriptor"
#define MY_CLASS ts::ServiceIdentifierDescriptor
#define MY_DID ts::DID_SERVICE_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceIdentifierDescriptor::ServiceIdentifierDescriptor(const UString& id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    identifier(id)
{
}

void ts::ServiceIdentifierDescriptor::clearContent()
{
    identifier.clear();
}

ts::ServiceIdentifierDescriptor::ServiceIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceIdentifierDescriptor()
{
    deserialize(duck, desc);
}

ts::DescriptorDuplication ts::ServiceIdentifierDescriptor::duplicationMode() const
{
    return DescriptorDuplication::REPLACE;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putString(identifier);
}

void ts::ServiceIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getString(identifier);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    disp << margin << "Service identifier: \"" << buf.getString() << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"service_identifier", identifier);
}

bool ts::ServiceIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(identifier, u"service_identifier", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
