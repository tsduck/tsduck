//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractPreferredNameIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractPreferredNameIdentifierDescriptor::AbstractPreferredNameIdentifierDescriptor(uint8_t id,
                                                                                         DID tag,
                                                                                         const UChar* xml_name,
                                                                                         Standards standards,
                                                                                         PDS pds,
                                                                                         const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name),
    name_id(id)
{
}

void ts::AbstractPreferredNameIdentifierDescriptor::clearContent()
{
    name_id = 0;
}

ts::AbstractPreferredNameIdentifierDescriptor::AbstractPreferredNameIdentifierDescriptor(DuckContext& duck,
                                                                                         const Descriptor& desc,
                                                                                         DID tag,
                                                                                         const UChar* xml_name,
                                                                                         Standards standards,
                                                                                         PDS pds,
                                                                                         const UChar* xml_legacy_name) :
    AbstractDescriptor(tag, xml_name, standards, pds, xml_legacy_name)
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(name_id);
}

void ts::AbstractPreferredNameIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    name_id = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Name identifier: " << int(buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::AbstractPreferredNameIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"name_id", name_id, true);
}

bool ts::AbstractPreferredNameIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(name_id, u"name_id", true, 0, 0x00, 0xFF);
}
