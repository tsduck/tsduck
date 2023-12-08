//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTransportProtocolDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"transport_protocol_descriptor"
#define MY_CLASS ts::TransportProtocolDescriptor
#define MY_DID ts::DID_AIT_TRANSPORT_PROTO
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors:
//----------------------------------------------------------------------------

ts::TransportProtocolDescriptor::TransportProtocolDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TransportProtocolDescriptor::TransportProtocolDescriptor(DuckContext& duck, const Descriptor& desc) :
    TransportProtocolDescriptor()
{
    deserialize(duck, desc);
}

void ts::TransportProtocolDescriptor::clearContent()
{
    protocol_id = 0;
    transport_protocol_label = 0;
    carousel.clear();
    mpe.clear();
    http.clear();
    selector.clear();
}

void ts::TransportProtocolDescriptor::Carousel::clear()
{
    original_network_id.reset();
    transport_stream_id.reset();
    service_id.reset();
    component_tag= 0;
}

void ts::TransportProtocolDescriptor::MPE::clear()
{
    original_network_id.reset();
    transport_stream_id.reset();
    service_id.reset();
    alignment_indicator = 0;
    urls.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(protocol_id);
    buf.putUInt8(transport_protocol_label);
    switch (protocol_id) {
        case MHP_PROTO_CAROUSEL: {
            // See ETSI TS 101 812, section 10.8.1.1
            const bool remote = carousel.original_network_id.has_value() && carousel.transport_stream_id.has_value() && carousel.service_id.has_value();
            buf.putBit(remote);
            buf.putBits(0xFF, 7);
            if (remote) {
                buf.putUInt16(carousel.original_network_id.value());
                buf.putUInt16(carousel.transport_stream_id.value());
                buf.putUInt16(carousel.service_id.value());
            }
            buf.putUInt8(carousel.component_tag);
            break;
        }
        case MHP_PROTO_MPE: {
            // See ETSI TS 101 812, section 10.8.1.2
            const bool remote = mpe.original_network_id.has_value() && mpe.transport_stream_id.has_value() && mpe.service_id.has_value();
            buf.putBit(remote);
            buf.putBits(0xFF, 7);
            if (remote) {
                buf.putUInt16(mpe.original_network_id.value());
                buf.putUInt16(mpe.transport_stream_id.value());
                buf.putUInt16(mpe.service_id.value());
            }
            buf.putBit(mpe.alignment_indicator);
            buf.putBits(0xFF, 7);
            for (const auto& it : mpe.urls) {
                buf.putStringWithByteLength(it);
            }
            break;
        }
        case MHP_PROTO_HTTP: {
            for (const auto& it1 : http) {
                buf.putStringWithByteLength(it1.URL_base);
                buf.putUInt8(uint8_t(it1.URL_extensions.size()));
                for (const auto& it2 : it1.URL_extensions) {
                    buf.putStringWithByteLength(it2);
                }
            }
            break;
        }
        default: {
            buf.putBytes(selector);
            break;
        }
    }
}


//----------------------------------------------------------------------------
// When the protocol id is a known one, try to transfer the selector bytes
// into the appropriate structure. Return false on invalid selector bytes.
//----------------------------------------------------------------------------

bool ts::TransportProtocolDescriptor::transferSelectorBytes(DuckContext& duck)
{
    PSIBuffer buf(duck, selector.data(), selector.size(), true);

    switch (protocol_id) {
        case MHP_PROTO_CAROUSEL: {
            carousel.clear();
            const bool remote = buf.getBool();
            buf.skipBits(7);
            if (remote) {
                carousel.original_network_id = buf.getUInt16();
                carousel.transport_stream_id = buf.getUInt16();
                carousel.service_id = buf.getUInt16();
            }
            carousel.component_tag = buf.getUInt8();
            break;
        }
        case MHP_PROTO_MPE: {
            mpe.clear();
            const bool remote = buf.getBool();
            buf.skipBits(7);
            if (remote) {
                mpe.original_network_id = buf.getUInt16();
                mpe.transport_stream_id = buf.getUInt16();
                mpe.service_id = buf.getUInt16();
            }
            mpe.alignment_indicator = buf.getBool();
            buf.skipBits(7);
            while (buf.canRead()) {
                mpe.urls.push_back(buf.getStringWithByteLength());
            }
            break;
        }
        case MHP_PROTO_HTTP: {
            http.clear();
            while (buf.canRead()) {
                HTTPEntry e;
                buf.getStringWithByteLength(e.URL_base);
                const size_t count = buf.getUInt8();
                for (size_t i = 0; i < count && !buf.error(); ++i) {
                    e.URL_extensions.push_back(buf.getStringWithByteLength());
                }
                http.push_back(e);
            }
            break;
        }
        default: {
            // Other protocols, do not interpret, keep selector byte array.
            return true;
        }
    }

    if (buf.error()) {
        return false;
    }
    else {
        // When a protocol was built, clear the selector byte array.
        selector.clear();
        return true;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::deserializePayload(PSIBuffer& buf)
{
    protocol_id = buf.getUInt16();
    transport_protocol_label = buf.getUInt8();
    buf.getBytes(selector);

    if (!transferSelectorBytes(buf.duck())) {
        invalidate();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        const uint16_t proto = buf.getUInt16();
        disp << margin << "Protocol id: " << NameFromDTV(u"MHPTransportProtocolId", proto, NamesFlags::BOTH_FIRST) << std::endl;
        disp << margin << UString::Format(u"Transport protocol label: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;

        switch (proto) {
            case MHP_PROTO_CAROUSEL: {
                if (buf.canReadBytes(1)) {
                    const bool remote = buf.getBool();
                    buf.skipBits(7);
                    if (remote && buf.canReadBytes(6)) {
                        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                    }
                    if (buf.canReadBytes(1)) {
                        disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
                    }
                }
                break;
            }
            case MHP_PROTO_MPE: {
                if (buf.canReadBytes(1)) {
                    const bool remote = buf.getBool();
                    buf.skipBits(7);
                    if (remote && buf.canReadBytes(6)) {
                        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                        disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                    }
                    if (buf.canReadBytes(1)) {
                        disp << margin << UString::Format(u"Alignment indicator: %d", {buf.getBool()}) << std::endl;
                        buf.skipBits(7);
                        while (buf.canRead()) {
                            disp << margin << "URL: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
                        }
                    }
                }
                break;
            }
            case MHP_PROTO_HTTP: {
                while (buf.canReadBytes(1)) {
                    disp << margin << "URL base: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
                    if (buf.canReadBytes(1)) {
                        const size_t count = buf.getUInt8();
                        for (size_t i = 0; i < count && buf.canReadBytes(1); ++i) {
                            disp << margin << "  Extension: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
                        }
                    }
                }
                break;
            }
            default: {
                disp.displayPrivateData(u"Selector", buf, NPOS, margin);
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TransportProtocolDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
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
            for (const auto& it : mpe.urls) {
                proto->addElement(u"url")->setAttribute(u"value", it);
            }
            break;
        }
        case MHP_PROTO_HTTP: {
            xml::Element* proto = root->addElement(u"http");
            for (const auto& it1 : http) {
                xml::Element* url = proto->addElement(u"url");
                url->setAttribute(u"base", it1.URL_base);
                for (const auto& it2 : it1.URL_extensions) {
                    url->addElement(u"extension")->setAttribute(u"value", it2);
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

bool ts::TransportProtocolDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector objcar;
    xml::ElementVector ip;
    xml::ElementVector htt;
    xml::ElementVector proto;
    xml::ElementVector urls;

    bool ok =
        element->getIntAttribute(transport_protocol_label, u"transport_protocol_label", true) &&
        element->getChildren(objcar, u"object_carousel", 0, 1) &&
        element->getChildren(ip, u"ip_mpe", 0, 1) &&
        element->getChildren(htt, u"http", 0, 1) &&
        element->getChildren(proto, u"protocol", 0, 1);

    if (ok && (objcar.size() + ip.size() + htt.size() + proto.size()) != 1) {
        element->report().error(u"specify exactly one of <object_carousel>, <ip_mpe>, <http>, <protocol> in <%s>, line %d", {element->name(), element->lineNumber()});
        return false;
    }
    else if (ok && !objcar.empty()) {
        protocol_id = MHP_PROTO_CAROUSEL;
        ok = objcar[0]->getOptionalIntAttribute(carousel.original_network_id, u"original_network_id") &&
             objcar[0]->getOptionalIntAttribute(carousel.transport_stream_id, u"transport_stream_id") &&
             objcar[0]->getOptionalIntAttribute(carousel.service_id, u"service_id") &&
             objcar[0]->getIntAttribute(carousel.component_tag, u"component_tag", true);
    }
    else if (ok && !ip.empty()) {
        protocol_id = MHP_PROTO_MPE;
        ok = ip[0]->getOptionalIntAttribute(mpe.original_network_id, u"original_network_id") &&
             ip[0]->getOptionalIntAttribute(mpe.transport_stream_id, u"transport_stream_id") &&
             ip[0]->getOptionalIntAttribute(mpe.service_id, u"service_id") &&
             ip[0]->getBoolAttribute(mpe.alignment_indicator, u"alignment_indicator", true) &&
             ip[0]->getChildren(urls, u"url");
        for (size_t i = 0; ok && i < urls.size(); ++i) {
            UString u;
            ok = urls[i]->getAttribute(u, u"value");
            mpe.urls.push_back(u);
        }
    }
    else if (ok && !htt.empty()) {
        protocol_id = MHP_PROTO_HTTP;
        ok = htt[0]->getChildren(urls, u"url");
        for (size_t i = 0; ok && i < urls.size(); ++i) {
            HTTPEntry e;
            xml::ElementVector exts;
            ok = urls[i]->getAttribute(e.URL_base, u"base") &&
                 urls[i]->getChildren(exts, u"extension");
            for (size_t ie = 0; ok && ie < exts.size(); ++ie) {
                UString u;
                ok = exts[ie]->getAttribute(u, u"value");
                e.URL_extensions.push_back(u);
            }
            http.push_back(e);
        }
    }
    else if (ok && !proto.empty()) {
        ok = proto[0]->getIntAttribute(protocol_id, u"id", true) &&
            proto[0]->getHexaText(selector) &&
            transferSelectorBytes(duck);
    }
    return ok;
}
