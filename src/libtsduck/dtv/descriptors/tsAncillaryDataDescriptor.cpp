//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAncillaryDataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ancillary_data_descriptor"
#define MY_CLASS ts::AncillaryDataDescriptor
#define MY_DID ts::DID_ANCILLARY_DATA
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AncillaryDataDescriptor::AncillaryDataDescriptor(uint8_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ancillary_data_identifier(id)
{
}

void ts::AncillaryDataDescriptor::clearContent()
{
    ancillary_data_identifier = 0;
}

ts::AncillaryDataDescriptor::AncillaryDataDescriptor(DuckContext& duck, const Descriptor& desc) :
    AncillaryDataDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AncillaryDataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(ancillary_data_identifier);
}

void ts::AncillaryDataDescriptor::deserializePayload(PSIBuffer& buf)
{
    ancillary_data_identifier = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AncillaryDataDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        const uint8_t id = buf.getUInt8();
        disp << margin << UString::Format(u"Ancillary data identifier: 0x%X", {id}) << std::endl;
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

void ts::AncillaryDataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"ancillary_data_identifier", ancillary_data_identifier, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AncillaryDataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(ancillary_data_identifier, u"ancillary_data_identifier", true);
}
