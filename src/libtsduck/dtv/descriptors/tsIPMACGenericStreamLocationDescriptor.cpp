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

#include "tsIPMACGenericStreamLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"IPMAC_generic_stream_location_descriptor"
#define MY_CLASS ts::IPMACGenericStreamLocationDescriptor
#define MY_DID ts::DID_INT_GEN_STREAM_LOC
#define MY_TID ts::TID_INT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

namespace {
    const ts::Enumeration ModulationTypeNames({
        {u"DVB-S2",  0},
        {u"DVB-T2",  1},
        {u"DVB-C2",  2},
        {u"DVB-NGH", 3},
    });
}

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IPMACGenericStreamLocationDescriptor::IPMACGenericStreamLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    interactive_network_id(0),
    modulation_system_type(0),
    modulation_system_id(0),
    PHY_stream_id(0),
    selector_bytes()
{
}

ts::IPMACGenericStreamLocationDescriptor::IPMACGenericStreamLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    IPMACGenericStreamLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::IPMACGenericStreamLocationDescriptor::clearContent()
{
    interactive_network_id = 0;
    modulation_system_type = 0;
    modulation_system_id = 0;
    PHY_stream_id = 0;
    selector_bytes.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(interactive_network_id);
    bbp->appendUInt8(modulation_system_type);
    bbp->appendUInt16(modulation_system_id);
    bbp->appendUInt16(PHY_stream_id);
    bbp->append(selector_bytes);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 7;

    if (_is_valid) {
        interactive_network_id = GetUInt16(data);
        modulation_system_type = GetUInt8(data + 2);
        modulation_system_id = GetUInt16(data + 3);
        PHY_stream_id = GetUInt16(data + 5);
        selector_bytes.copy(data + 7, size - 7);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 7) {
        const uint16_t netid = GetUInt16(data);
        const uint8_t systype = GetUInt8(data + 2);
        const uint16_t sysid = GetUInt16(data + 3);
        const uint16_t strid  = GetUInt16(data + 5);
        strm << margin << UString::Format(u"Interactive network id: 0x%X (%d)", {netid, netid}) << std::endl
             << margin << UString::Format(u"Modulation system type: 0x%X (%s)", {systype, ModulationTypeNames.name(systype)}) << std::endl
             << margin << UString::Format(u"Modulation system id: 0x%X (%d)", {sysid, sysid}) << std::endl
             << margin << UString::Format(u"Physical stream id: 0x%X (%d)", {strid, strid}) << std::endl;
        display.displayPrivateData(u"Selector bytes", data + 7, size - 7, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"interactive_network_id", interactive_network_id, true);
    root->setIntEnumAttribute(ModulationTypeNames, u"modulation_system_type", modulation_system_type);
    root->setIntAttribute(u"modulation_system_id", modulation_system_id, true);
    root->setIntAttribute(u"PHY_stream_id", PHY_stream_id, true);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::IPMACGenericStreamLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(interactive_network_id, u"interactive_network_id", true) &&
           element->getIntEnumAttribute(modulation_system_type, ModulationTypeNames, u"modulation_system_type", true) &&
           element->getIntAttribute(modulation_system_id, u"modulation_system_id", false) &&
           element->getIntAttribute(PHY_stream_id, u"PHY_stream_id", false) &&
           element->getHexaTextChild(selector_bytes, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 9);
}
