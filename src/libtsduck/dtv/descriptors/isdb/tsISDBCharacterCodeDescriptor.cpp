//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBCharacterCodeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_character_code_descriptor"
#define MY_CLASS    ts::ISDBCharacterCodeDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_CHAR_CODE, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBCharacterCodeDescriptor::ISDBCharacterCodeDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ISDBCharacterCodeDescriptor::ISDBCharacterCodeDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBCharacterCodeDescriptor()
{
    deserialize(duck, desc);
}

void ts::ISDBCharacterCodeDescriptor::clearContent()
{
    character_code = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBCharacterCodeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(character_code);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBCharacterCodeDescriptor::deserializePayload(PSIBuffer& buf)
{
    character_code = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBCharacterCodeDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canRead()) {
        disp << margin << "Character code: " << DataName(MY_XML_NAME, u"character_code", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBCharacterCodeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"character_code", character_code);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBCharacterCodeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(character_code, u"character_code", true);
}
