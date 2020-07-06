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

#include "tsDVBHTMLApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dvb_html_application_descriptor"
#define MY_CLASS ts::DVBHTMLApplicationDescriptor
#define MY_DID ts::DID_AIT_HTML_APP
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBHTMLApplicationDescriptor::DVBHTMLApplicationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    application_ids(),
    parameter()
{
}

void ts::DVBHTMLApplicationDescriptor::clearContent()
{
    application_ids.clear();
    parameter.clear();
}

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
    bbp->append(duck.encoded(parameter));
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

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        size_t len = data[0];
        data++; size--;
        _is_valid = len % 2 == 0 && len <= size;
        if (_is_valid) {
            while (len >= 2) {
                application_ids.push_back(GetUInt16(data));
                data += 2; size -= 2; len -= 2;
            }
            duck.decode(parameter, data, size);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBHTMLApplicationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
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
            strm << margin << "Parameter: \"" << duck.decoded(data, size) << "\"" << std::endl;
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

bool ts::DVBHTMLApplicationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getAttribute(parameter, u"parameter", false) && element->getChildren(children, u"application");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint16_t id;
        ok = children[i]->getIntAttribute<uint16_t>(id, u"id", true);
        application_ids.push_back(id);
    }
    return ok;
}
