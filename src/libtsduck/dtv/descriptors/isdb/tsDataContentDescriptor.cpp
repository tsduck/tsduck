//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDataContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"data_content_descriptor"
#define MY_CLASS ts::DataContentDescriptor
#define MY_DID ts::DID_ISDB_DATA_CONTENT
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DataContentDescriptor::DataContentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::DataContentDescriptor::clearContent()
{
    data_component_id = 0;
    entry_component = 0;
    selector_bytes.clear();
    component_refs.clear();
    ISO_639_language_code.clear();
    text.clear();
}

ts::DataContentDescriptor::DataContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    DataContentDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(data_component_id);
    buf.putUInt8(entry_component);
    buf.putUInt8(uint8_t(selector_bytes.size()));
    buf.putBytes(selector_bytes);
    buf.putUInt8(uint8_t(component_refs.size()));
    buf.putBytes(component_refs);
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_component_id = buf.getUInt16();
    entry_component = buf.getUInt8();
    size_t len = buf.getUInt8();
    buf.getBytes(selector_bytes, len);
    len = buf.getUInt8();
    buf.getBytes(component_refs, len);
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Data component id: " << DataName(MY_XML_NAME, u"DataComponentId", buf.getUInt16(), NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << UString::Format(u"Entry component: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        size_t len = buf.getUInt8();
        disp.displayPrivateData(u"Selector bytes", buf, len, margin);
        len = buf.canRead() ? buf.getUInt8() : 0;
        for (size_t i = 0; buf.canRead() && i < len; ++i) {
            disp << margin << UString::Format(u"Component ref: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (buf.canReadBytes(3)) {
            disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
            disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_component_id", data_component_id, true);
    root->setIntAttribute(u"entry_component", entry_component, true);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->setAttribute(u"text", text);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
    for (const auto& it : component_refs) {
        root->addElement(u"component")->setIntAttribute(u"ref", it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DataContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok =
        element->getIntAttribute(data_component_id, u"data_component_id", true) &&
        element->getIntAttribute(entry_component, u"entry_component", true) &&
        element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, UString(), 3, 3) &&
        element->getAttribute(text, u"text", true) &&
        element->getHexaTextChild(selector_bytes, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 8) &&
        element->getChildren(xcomp, u"component");

    for (auto it = xcomp.begin(); ok && it != xcomp.end(); ++it) {
        uint8_t ref = 0;
        ok = (*it)->getIntAttribute(ref, u"ref", true);
        component_refs.push_back(ref);
    }
    return ok;
}
