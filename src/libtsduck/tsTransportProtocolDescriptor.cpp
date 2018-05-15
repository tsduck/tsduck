//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsTransportProtocolDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"transport_protocol_descriptor"
#define MY_DID ts::DID_AIT_TRANSPORT_PROTO
#define MY_TID ts::TID_AIT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::TransportProtocolDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::TransportProtocolDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::TransportProtocolDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Constructors:
//----------------------------------------------------------------------------

ts::TransportProtocolDescriptor::TransportProtocolDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    protocol_id(0),
    transport_protocol_label(0),
    carousel(),
    mpe(),
    http(),
    selector()
{
    _is_valid = true;
}

ts::TransportProtocolDescriptor::TransportProtocolDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    TransportProtocolDescriptor()
{
    deserialize(desc, charset);
}

void ts::TransportProtocolDescriptor::clear()
{
    protocol_id = 0;
    transport_protocol_label = 0;
    carousel.clear();
    mpe.clear();
    http.clear();
    selector.clear();
}


//----------------------------------------------------------------------------
// Constructors for specific selector bytes layouts.
//----------------------------------------------------------------------------

ts::TransportProtocolDescriptor::Carousel::Carousel() :
    original_network_id(),
    transport_stream_id(),
    service_id(),
    component_tag(0)
{
}

void ts::TransportProtocolDescriptor::Carousel::clear()
{
    original_network_id.reset();
    transport_stream_id.reset();
    service_id.reset();
    component_tag= 0;
}

ts::TransportProtocolDescriptor::MPE::MPE() :
    original_network_id(),
    transport_stream_id(),
    service_id(),
    alignment_indicator(false),
    urls()
{
}

void ts::TransportProtocolDescriptor::MPE::clear()
{
    original_network_id.reset();
    transport_stream_id.reset();
    service_id.reset();
    alignment_indicator = 0;
    urls.clear();
}

ts::TransportProtocolDescriptor::HTTPEntry::HTTPEntry() :
    URL_base(),
    URL_extensions()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(protocol_id);
    bbp->appendUInt8(transport_protocol_label);
    switch (protocol_id) {
        case MHP_PROTO_CAROUSEL: {
            if (carousel.original_network_id.set() && carousel.transport_stream_id.set() && carousel.service_id.set()) {
                bbp->appendUInt8(0xFF);
                bbp->appendUInt16(carousel.original_network_id.value());
                bbp->appendUInt16(carousel.transport_stream_id.value());
                bbp->appendUInt16(carousel.service_id.value());
            }
            else {
                bbp->appendUInt8(0x7F);
            }
            bbp->appendUInt8(carousel.component_tag);
            break;
        }
        case MHP_PROTO_MPE: {
            if (mpe.original_network_id.set() && mpe.transport_stream_id.set() && mpe.service_id.set()) {
                bbp->appendUInt8(0xFF);
                bbp->appendUInt16(mpe.original_network_id.value());
                bbp->appendUInt16(mpe.transport_stream_id.value());
                bbp->appendUInt16(mpe.service_id.value());
            }
            else {
                bbp->appendUInt8(0x7F);
            }
            bbp->appendUInt8(mpe.alignment_indicator ? 0xFF : 0x7F);
            for (auto it = mpe.urls.begin(); it != mpe.urls.end(); ++it) {
                bbp->append(it->toDVBWithByteLength(0, UString::NPOS, charset));
            }
            break;
        }
        case MHP_PROTO_HTTP: {
            for (auto it1 = http.begin(); it1 != http.end(); ++it1) {
                bbp->append(it1->URL_base.toDVBWithByteLength(0, UString::NPOS, charset));
                bbp->appendUInt8(uint8_t(it1->URL_extensions.size()));
                for (auto it2 = it1->URL_extensions.begin(); it2 != it1->URL_extensions.end(); ++it2) {
                    bbp->append(it2->toDVBWithByteLength(0, UString::NPOS, charset));
                }
            }
            break;
        }
        default: {
            bbp->append(selector);
            break;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// When the protocol id is a known one, try to transfer the selector bytes
// into the appropriate structure. Return false on invalid selector bytes.
//----------------------------------------------------------------------------

bool ts::TransportProtocolDescriptor::transferSelectorBytes()
{
    switch (protocol_id) {
        case MHP_PROTO_CAROUSEL: {
            //@@@@@
            selector.clear();
            return true;
        }
        case MHP_PROTO_MPE: {
            //@@@@@
            selector.clear();
            return true;
        }
        case MHP_PROTO_HTTP: {
            //@@@@@
            selector.clear();
            return true;
        }
        default: {
            // Other protocols, do not interpret.
            return true;
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 3;

    if (_is_valid) {
        protocol_id = GetUInt16(data);
        transport_protocol_label = data[2];
        selector.copy(data + 3, size - 3);
        _is_valid = transferSelectorBytes();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        strm << margin << "Protocol id: " << DVBNameFromSection(u"MHPTransportProtocolId", GetUInt16(data), names::BOTH_FIRST) << std::endl
             << margin << UString::Format(u"Transport protocol label: 0x%X (%d)", {data[2], data[2]}) << std::endl;
        data += 3; size -= 3;

        //@@@@
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"transport_protocol_label", transport_protocol_label, true);
    switch (protocol_id) {
        case MHP_PROTO_CAROUSEL: {
            xml::Element* proto = root->addElement(u"object_carousel");
            proto->setOptionalIntAttribute(u"original_network_id", carousel.original_network_id, true);
            proto->setOptionalIntAttribute(u"transport_stream_id", carousel.transport_stream_id, true);
            proto->setOptionalIntAttribute(u"service_id", carousel.service_id, true);
            proto->setIntAttribute(u"component_tag", carousel.component_tag, true);
            break;
        }
        case MHP_PROTO_MPE: {
            xml::Element* proto = root->addElement(u"ip_mpe");
            proto->setOptionalIntAttribute(u"original_network_id", mpe.original_network_id, true);
            proto->setOptionalIntAttribute(u"transport_stream_id", mpe.transport_stream_id, true);
            proto->setOptionalIntAttribute(u"service_id", mpe.service_id, true);
            proto->setBoolAttribute(u"alignment_indicator", mpe.alignment_indicator);
            for (auto it = mpe.urls.begin(); it != mpe.urls.end(); ++it) {
                proto->addElement(u"url")->setAttribute(u"value", *it);
            }
            break;
        }
        case MHP_PROTO_HTTP: {
            xml::Element* proto = root->addElement(u"http");
            for (auto it1 = http.begin(); it1 != http.end(); ++it1) {
                xml::Element* url = root->addElement(u"url");
                url->setAttribute(u"base", it1->URL_base);
                for (auto it2 = it1->URL_extensions.begin(); it2 != it1->URL_extensions.end(); ++it2) {
                    url->addElement(u"extension")->setAttribute(u"value", *it2);
                }
            }
            break;
        }
        default: {
            xml::Element* proto = root->addElement(u"protocol");
            proto->setIntAttribute(u"id", protocol_id, true);
            if (!selector.empty()) {
                proto->addHexaText(selector);
            }
            break;
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::fromXML(const xml::Element* element)
{
    clear();

    xml::ElementVector objcar;
    xml::ElementVector ip;
    xml::ElementVector htt;
    xml::ElementVector proto;

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(transport_protocol_label, u"transport_protocol_label", true) &&
        element->getChildren(objcar, u"object_carousel", 0, 1) &&
        element->getChildren(ip, u"ip_mpe", 0, 1) &&
        element->getChildren(htt, u"http", 0, 1) &&
        element->getChildren(proto, u"protocol", 0, 1);

    if (_is_valid && (objcar.size() + ip.size() + htt.size() + proto.size()) != 1) {
        _is_valid = false;
        element->report().error(u"specify exactly one of <object_carousel>, <ip_mpe>, <http>, <protocol> in <%s>, line %d", {element->name(), element->lineNumber()});
    }
    else if (_is_valid && !objcar.empty()) {
        protocol_id = MHP_PROTO_CAROUSEL;
        //@@@@
    }
    else if (_is_valid && !ip.empty()) {
        protocol_id = MHP_PROTO_MPE;
        //@@@@
    }
    else if (_is_valid && !htt.empty()) {
        protocol_id = MHP_PROTO_HTTP;
        //@@@@
    }
    else if (_is_valid && !proto.empty()) {
        _is_valid =
            proto[0]->getIntAttribute<uint16_t>(protocol_id, u"id", true) &&
            proto[0]->getHexaText(selector) &&
            transferSelectorBytes();
    }
}
