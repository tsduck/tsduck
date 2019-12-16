//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsDVBHTMLApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dvb_html_application_descriptor"
#define MY_DID ts::DID_AIT_HTML_APP
#define MY_TID ts::TID_AIT
#define MY_STD ts::STD_DVB

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::DVBHTMLApplicationDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::DVBHTMLApplicationDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_FACTORY_REGISTER(ts::DVBHTMLApplicationDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationDescriptor::DVBHTMLApplicationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    application_ids(),
    parameter()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationDescriptor::DVBHTMLApplicationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBHTMLApplicationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(application_ids.size() * sizeof(uint16_t)));
    for (size_t i = 0; i < application_ids.size(); ++i) {
        bbp->appendUInt16(application_ids[i]);
    }
    bbp->append(duck.toDVB(parameter));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    application_ids.clear();
    parameter.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 1;

    if (_is_valid) {
        size_t len = data[0];
        data++; size--;
        _is_valid = len % 2 == 0 && len <= size;
        if (_is_valid) {
            while (len >= 2) {
                application_ids.push_back(GetUInt16(data));
                data += 2; size -= 2; len -= 2;
            }
            parameter = duck.fromDVB(data, size);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t len = data[0];
        if (len % 2 == 0 && len + 1 <= size) {
            data++; size--;
            while (len >= 2) {
                const uint16_t id = GetUInt16(data);
                data += 2; size -= 2; len -= 2;
                strm << margin << UString::Format(u"Application id: 0x%X (%d)", {id, id}) << std::endl;
            }
            strm << margin << "Parameter: \"" << display.duck().fromDVB(data, size) << "\"" << std::endl;
            size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"parameter", parameter);
    for (auto it = application_ids.begin(); it != application_ids.end(); ++it) {
        root->addElement(u"application")->setIntAttribute(u"id", *it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    application_ids.clear();
    parameter.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(parameter, u"parameter", false) &&
        element->getChildren(children, u"application");

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        uint16_t id;
        _is_valid = children[i]->getIntAttribute<uint16_t>(id, u"id", true);
        if (_is_valid) {
            application_ids.push_back(id);
        }
    }
}
