//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSVCExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"SVC_extension_descriptor"
#define MY_CLASS ts::SVCExtensionDescriptor
#define MY_DID ts::DID_SVC_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SVCExtensionDescriptor::SVCExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    width(0),
    height(0),
    frame_rate(0),
    average_bitrate(0),
    maximum_bitrate(0),
    dependency_id(0),
    quality_id_start(0),
    quality_id_end(0),
    temporal_id_start(0),
    temporal_id_end(0),
    no_sei_nal_unit_present(false)
{
}

ts::SVCExtensionDescriptor::SVCExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    SVCExtensionDescriptor()
{
    deserialize(duck, desc);
}

void ts::SVCExtensionDescriptor::clearContent()
{
    width = 0;
    height = 0;
    frame_rate = 0;
    average_bitrate = 0;
    maximum_bitrate = 0;
    dependency_id = 0;
    quality_id_start = 0;
    quality_id_end = 0;
    temporal_id_start = 0;
    temporal_id_end = 0;
    no_sei_nal_unit_present = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SVCExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(width);
    buf.putUInt16(height);
    buf.putUInt16(frame_rate);
    buf.putUInt16(average_bitrate);
    buf.putUInt16(maximum_bitrate);
    buf.putBits(dependency_id, 3);
    buf.putBits(0xFF, 5);
    buf.putBits(quality_id_start, 4);
    buf.putBits(quality_id_end, 4);
    buf.putBits(temporal_id_start, 3);
    buf.putBits(temporal_id_end, 3);
    buf.putBit(no_sei_nal_unit_present);
    buf.putBit(1);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SVCExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    width = buf.getUInt16();
    height = buf.getUInt16();
    frame_rate = buf.getUInt16();
    average_bitrate = buf.getUInt16();
    maximum_bitrate = buf.getUInt16();
    buf.getBits(dependency_id, 3);
    buf.skipBits(5);
    buf.getBits(quality_id_start, 4);
    buf.getBits(quality_id_end, 4);
    buf.getBits(temporal_id_start, 3);
    buf.getBits(temporal_id_end, 3);
    no_sei_nal_unit_present = buf.getBool();
    buf.skipBits(1);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SVCExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    const UString margin(indent, ' ');

    if (size >= 13) {
        disp << margin << UString::Format(u"Frame size: %dx%d", {GetUInt16(data), GetUInt16(data + 2)}) << std::endl
             << margin << UString::Format(u"Frame rate: %d frames / 256 seconds", {GetUInt16(data + 4)}) << std::endl
             << margin << UString::Format(u"Average bitrate: %d kb/s, maximum: %d kb/s", {GetUInt16(data + 6), GetUInt16(data + 8)}) << std::endl
             << margin << UString::Format(u"Dependency id: %d", {(data[10] >> 5) & 0x07}) << std::endl
             << margin << UString::Format(u"Quality id start: %d, end: %d", {(data[11] >> 4) & 0x0F, data[11] & 0x0F}) << std::endl
             << margin << UString::Format(u"Temporal id start: %d, end: %d", {(data[12] >> 5) & 0x07, (data[12] >> 2) & 0x07}) << std::endl
             << margin << UString::Format(u"No SEI NALunit present: %s", {(data[12] & 0x02) != 0}) << std::endl;
        data += 13; size -= 13;
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SVCExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"width", width);
    root->setIntAttribute(u"height", height);
    root->setIntAttribute(u"frame_rate", frame_rate);
    root->setIntAttribute(u"average_bitrate", average_bitrate);
    root->setIntAttribute(u"maximum_bitrate", maximum_bitrate);
    root->setIntAttribute(u"dependency_id", dependency_id);
    root->setIntAttribute(u"quality_id_start", quality_id_start);
    root->setIntAttribute(u"quality_id_end", quality_id_end);
    root->setIntAttribute(u"temporal_id_start", temporal_id_start);
    root->setIntAttribute(u"temporal_id_end", temporal_id_end);
    root->setBoolAttribute(u"no_sei_nal_unit_present", no_sei_nal_unit_present);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SVCExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(width, u"width", true) &&
            element->getIntAttribute(height, u"height", true) &&
            element->getIntAttribute(frame_rate, u"frame_rate", true) &&
            element->getIntAttribute(average_bitrate, u"average_bitrate", true) &&
            element->getIntAttribute(maximum_bitrate, u"maximum_bitrate", true) &&
            element->getIntAttribute(dependency_id, u"dependency_id", true, 0, 0x00, 0x07) &&
            element->getIntAttribute(quality_id_start, u"quality_id_start", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute(quality_id_end, u"quality_id_end", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute(temporal_id_start, u"temporal_id_start", true, 0, 0x00, 0x07) &&
            element->getIntAttribute(temporal_id_end, u"temporal_id_end", true, 0, 0x00, 0x07) &&
            element->getBoolAttribute(no_sei_nal_unit_present, u"no_sei_nal_unit_present", true);
}
