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

#include "tsDataContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    data_component_id(0),
    entry_component(0),
    selector_bytes(),
    component_refs(),
    ISO_639_language_code(),
    text()
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

void ts::DataContentDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(data_component_id);
    bbp->appendUInt8(entry_component);
    bbp->appendUInt8(uint8_t(selector_bytes.size()));
    bbp->append(selector_bytes);
    bbp->appendUInt8(uint8_t(component_refs.size()));
    bbp->append(component_refs);
    if (!SerializeLanguageCode(*bbp, ISO_639_language_code)) {
        desc.invalidate();
        return;
    }
    bbp->append(duck.encodedWithByteLength(text));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4;

    selector_bytes.clear();
    component_refs.clear();
    ISO_639_language_code.clear();
    text.clear();

    if (_is_valid) {
        data_component_id = GetUInt16(data);
        entry_component = data[2];
        size_t len1 = data[3];
        data += 4; size -= 4;
        _is_valid = len1 < size;

        if (_is_valid) {
            selector_bytes.copy(data, len1);
            size_t len2 = data[len1];
            data += len1 + 1; size -= len1 + 1;
            _is_valid = len2 + 4 <= size;

            if (_is_valid) {
                component_refs.copy(data, len2);
                ISO_639_language_code = DeserializeLanguageCode(data + len2);
                data += len2 + 3; size -= len2 + 3;

                _is_valid = duck.decodeWithByteLength(text, data, size) && size == 0;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataContentDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        strm << margin << "Data component id: " << NameFromSection(u"ISDBDataComponentId", GetUInt16(data), names::HEXA_FIRST) << std::endl
             << margin << UString::Format(u"Entry component: 0x%X (%d)", {data[2], data[2]}) << std::endl;

        size_t len = data[3];
        data += 4; size -= 4;
        len = std::min(len, size);
        display.displayPrivateData(u"Selector bytes", data, len, indent);
        data += len; size -= len;

        if (size > 0) {
            len = data[0];
            data++; size--;
            for (size_t i = 0; size > 0 && i < len; ++i) {
                strm << margin << UString::Format(u"Component ref: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                data++; size--;
            }

            if (size >= 4) {
                strm << margin << "Language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
                data += 3; size -= 3;
                strm << margin << "Text: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
            }
        }
    }

    display.displayExtraData(data, size, indent);
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
    for (auto it = component_refs.begin(); it != component_refs.end(); ++it) {
        root->addElement(u"component")->setIntAttribute(u"ref", *it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DataContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok =
        element->getIntAttribute<uint16_t>(data_component_id, u"data_component_id", true) &&
        element->getIntAttribute<uint8_t>(entry_component, u"entry_component", true) &&
        element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, UString(), 3, 3) &&
        element->getAttribute(text, u"text", true) &&
        element->getHexaTextChild(selector_bytes, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 8) &&
        element->getChildren(xcomp, u"component");

    for (auto it = xcomp.begin(); ok && it != xcomp.end(); ++it) {
        uint8_t ref = 0;
        ok = (*it)->getIntAttribute<uint8_t>(ref, u"ref", true);
        component_refs.push_back(ref);
    }
    return ok;
}
