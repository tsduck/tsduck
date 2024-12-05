//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSULocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"SSU_location_descriptor"
#define MY_CLASS    ts::SSULocationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_UNT_SSU_LOCATION, ts::Standards::DVB, ts::TID_UNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSULocationDescriptor::SSULocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SSULocationDescriptor::clearContent()
{
    data_broadcast_id = 0;
    association_tag = 0;
    private_data.clear();
}

ts::SSULocationDescriptor::SSULocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSULocationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSULocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(data_broadcast_id);
    if (data_broadcast_id == 0x000A) {
        buf.putUInt16(association_tag);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSULocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_broadcast_id = buf.getUInt16();
    if (data_broadcast_id == 0x000A) {
        association_tag = buf.getUInt16();
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSULocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        const uint16_t id = buf.getUInt16();
        disp << margin << "Data broadcast id: " << names::DataBroadcastId(id, NamesFlags::HEXA_FIRST) << std::endl;
        if (id == 0x000A && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"Association tag: %n", buf.getUInt16()) << std::endl;
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSULocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_broadcast_id", data_broadcast_id, true);
    if (data_broadcast_id == 0x000A) {
        root->setIntAttribute(u"association_tag", association_tag, true);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSULocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(data_broadcast_id, u"data_broadcast_id", true) &&
           element->getIntAttribute(association_tag, u"association_tag", data_broadcast_id == 0x000A) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}
