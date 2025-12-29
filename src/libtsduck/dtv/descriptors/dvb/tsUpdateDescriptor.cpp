//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUpdateDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"update_descriptor"
#define MY_CLASS    ts::UpdateDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_UNT_UPDATE, ts::Standards::DVB, ts::TID_UNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::UpdateDescriptor::UpdateDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::UpdateDescriptor::clearContent()
{
    update_flag = 0;
    update_method = 0;
    update_priority = 0;
    private_data.clear();
}

ts::UpdateDescriptor::UpdateDescriptor(DuckContext& duck, const Descriptor& desc) :
    UpdateDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::UpdateDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(update_flag, 2);
    buf.putBits(update_method, 4);
    buf.putBits(update_priority, 2);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::UpdateDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(update_flag, 2);
    buf.getBits(update_method, 4);
    buf.getBits(update_priority, 2);
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::UpdateDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canRead()) {
        disp << margin << "Update flag: " << DataName(MY_XML_NAME, u"SSUUpdateFlag", buf.getBits<uint8_t>(2), NamesFlags::DEC_VALUE_NAME) << std::endl;
        disp << margin << "Update method: " << DataName(MY_XML_NAME, u"SSUUpdateMethod", buf.getBits<uint8_t>(4), NamesFlags::DEC_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Update priority: %d", buf.getBits<uint8_t>(2)) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::UpdateDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"update_flag", update_flag, false);
    root->setIntAttribute(u"update_method", update_method, false);
    root->setIntAttribute(u"update_priority", update_priority, false);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::UpdateDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(update_flag, u"update_flag", true, 0, 0, 3) &&
           element->getIntAttribute(update_method, u"update_method", true, 0, 0, 15) &&
           element->getIntAttribute(update_priority, u"update_priority", true, 0, 0, 3) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 3);
}
