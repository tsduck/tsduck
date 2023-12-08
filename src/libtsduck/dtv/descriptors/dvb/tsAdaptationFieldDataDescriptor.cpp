//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAdaptationFieldDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"adaptation_field_data_descriptor"
#define MY_CLASS ts::AdaptationFieldDataDescriptor
#define MY_DID ts::DID_ADAPTFIELD_DATA
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AdaptationFieldDataDescriptor::AdaptationFieldDataDescriptor(uint8_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    adaptation_field_data_identifier(id)
{
}

void ts::AdaptationFieldDataDescriptor::clearContent()
{
    adaptation_field_data_identifier = 0;
}

ts::AdaptationFieldDataDescriptor::AdaptationFieldDataDescriptor(DuckContext& duck, const Descriptor& desc) :
    AdaptationFieldDataDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::AdaptationFieldDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(adaptation_field_data_identifier);
}

void ts::AdaptationFieldDataDescriptor::deserializePayload(PSIBuffer& buf)
{
    adaptation_field_data_identifier = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AdaptationFieldDataDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t id = buf.getUInt8();
        disp << margin << UString::Format(u"Adaptation field data identifier: 0x%X", {id}) << std::endl;
        for (int i = 0; i < 8; ++i) {
            if ((id & (1 << i)) != 0) {
                disp << margin << "  " << DataName(MY_XML_NAME, u"DataIdentifier", (1 << i), NamesFlags::HEXA_FIRST) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AdaptationFieldDataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"adaptation_field_data_identifier", adaptation_field_data_identifier, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AdaptationFieldDataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(adaptation_field_data_identifier, u"adaptation_field_data_identifier", true);
}
