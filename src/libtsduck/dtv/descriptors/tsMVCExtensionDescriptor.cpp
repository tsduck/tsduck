//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMVCExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MVC_extension_descriptor"
#define MY_CLASS ts::MVCExtensionDescriptor
#define MY_DID ts::DID_MVC_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MVCExtensionDescriptor::MVCExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MVCExtensionDescriptor::clearContent()
{
    average_bitrate = 0;
    maximum_bitrate = 0;
    view_association_not_present = false;
    base_view_is_left_eyeview = false;
    view_order_index_min = 0;
    view_order_index_max = 0;
    temporal_id_start = 0;
    temporal_id_end = 0;
    no_sei_nal_unit_present = false;
    no_prefix_nal_unit_present = false;
}

ts::MVCExtensionDescriptor::MVCExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    MVCExtensionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(average_bitrate);
    buf.putUInt16(maximum_bitrate);
    buf.putBit(view_association_not_present);
    buf.putBit(base_view_is_left_eyeview);
    buf.putBits(0xFF, 2);
    buf.putBits(view_order_index_min, 10);
    buf.putBits(view_order_index_max, 10);
    buf.putBits(temporal_id_start, 3);
    buf.putBits(temporal_id_end, 3);
    buf.putBit(no_sei_nal_unit_present);
    buf.putBit(no_prefix_nal_unit_present);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    average_bitrate = buf.getUInt16();
    maximum_bitrate = buf.getUInt16();
    view_association_not_present = buf.getBool();
    base_view_is_left_eyeview = buf.getBool();
    buf.skipBits(2);
    buf.getBits(view_order_index_min, 10);
    buf.getBits(view_order_index_max, 10);
    buf.getBits(temporal_id_start, 3);
    buf.getBits(temporal_id_end, 3);
    no_sei_nal_unit_present = buf.getBool();
    no_prefix_nal_unit_present = buf.getBool();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"Average bitrate: %d kb/s", {buf.getUInt16()});
        disp << UString::Format(u", maximum: %d kb/s", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"View association not present: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Base view is left eyeview: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(2);
        disp << margin << UString::Format(u"View order min: %d", {buf.getBits<uint16_t>(10)});
        disp << UString::Format(u", max: %d", {buf.getBits<uint16_t>(10)}) << std::endl;
        disp << margin << UString::Format(u"Temporal id start: %d", {buf.getBits<uint8_t>(3)});
        disp << UString::Format(u", end: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
        disp << margin << UString::Format(u"No SEI NALunit present: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"No prefix NALunit present: %s", {buf.getBool()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MVCExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"average_bitrate", average_bitrate);
    root->setIntAttribute(u"maximum_bitrate", maximum_bitrate);
    root->setBoolAttribute(u"view_association_not_present", view_association_not_present);
    root->setBoolAttribute(u"base_view_is_left_eyeview", base_view_is_left_eyeview);
    root->setIntAttribute(u"view_order_index_min", view_order_index_min);
    root->setIntAttribute(u"view_order_index_max", view_order_index_max);
    root->setIntAttribute(u"temporal_id_start", temporal_id_start);
    root->setIntAttribute(u"temporal_id_end", temporal_id_end);
    root->setBoolAttribute(u"no_sei_nal_unit_present", no_sei_nal_unit_present);
    root->setBoolAttribute(u"no_prefix_nal_unit_present", no_prefix_nal_unit_present);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MVCExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(average_bitrate, u"average_bitrate", true) &&
            element->getIntAttribute(maximum_bitrate, u"maximum_bitrate", true) &&
            element->getBoolAttribute(view_association_not_present, u"view_association_not_present", true) &&
            element->getBoolAttribute(base_view_is_left_eyeview, u"base_view_is_left_eyeview", true) &&
            element->getIntAttribute(view_order_index_min, u"view_order_index_min", true, 0, 0x0000, 0x03FF) &&
            element->getIntAttribute(view_order_index_max, u"view_order_index_max", true, 0, 0x0000, 0x03FF) &&
            element->getIntAttribute(temporal_id_start, u"temporal_id_start", true, 0, 0x00, 0x07) &&
            element->getIntAttribute(temporal_id_end, u"temporal_id_end", true, 0, 0x00, 0x07) &&
            element->getBoolAttribute(no_sei_nal_unit_present, u"no_sei_nal_unit_present", true) &&
            element->getBoolAttribute(no_prefix_nal_unit_present, u"no_prefix_nal_unit_present", true);
}
