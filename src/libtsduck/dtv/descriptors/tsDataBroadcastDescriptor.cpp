//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDataBroadcastDescriptor.h"
#include "tsDescriptor.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"data_broadcast_descriptor"
#define MY_CLASS ts::DataBroadcastDescriptor
#define MY_DID ts::DID_DATA_BROADCAST
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DataBroadcastDescriptor::DataBroadcastDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DataBroadcastDescriptor::DataBroadcastDescriptor(DuckContext& duck, const Descriptor& desc) :
    DataBroadcastDescriptor()
{
    deserialize(duck, desc);
}

void ts::DataBroadcastDescriptor::clearContent()
{
    data_broadcast_id = 0;
    component_tag = 0;
    selector_bytes.clear();
    language_code.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataBroadcastDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        const uint16_t dbid = buf.getUInt16();
        disp << margin << "Data broadcast id: " << names::DataBroadcastId(dbid, NamesFlags::BOTH_FIRST) << std::endl;
        disp << margin << UString::Format(u"Component tag: %d (0x%<X), ", {buf.getUInt8()}) << std::endl;

        buf.pushReadSizeFromLength(8); // selector_length
        DataBroadcastIdDescriptor::DisplaySelectorBytes(disp, buf, margin, dbid);
        buf.popState(); // end of selector_length

        if (buf.canReadBytes(3)) {
            disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
            disp << margin << "Description: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(data_broadcast_id);
    buf.putUInt8(component_tag);
    buf.putUInt8(uint8_t(selector_bytes.size()));
    buf.putBytes(selector_bytes);
    buf.putLanguageCode(language_code);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataBroadcastDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_broadcast_id = buf.getUInt16();
    component_tag = buf.getUInt8();
    const size_t length = buf.getUInt8();
    buf.getBytes(selector_bytes, length);
    buf.getLanguageCode(language_code);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DataBroadcastDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_broadcast_id", data_broadcast_id, true);
    root->setIntAttribute(u"component_tag", component_tag, true);
    root->setAttribute(u"language_code", language_code);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
    root->addElement(u"text")->addText(text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DataBroadcastDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(data_broadcast_id, u"data_broadcast_id", true) &&
            element->getIntAttribute(component_tag, u"component_tag", true) &&
            element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
            element->getHexaTextChild(selector_bytes, u"selector_bytes", true) &&
            element->getTextChild(text, u"text");
}
