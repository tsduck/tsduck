//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNetworkDownloadContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"network_download_content_descriptor"
#define MY_CLASS ts::NetworkDownloadContentDescriptor
#define MY_DID ts::DID_ISDB_NETW_DOWNLOAD
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NetworkDownloadContentDescriptor::NetworkDownloadContentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::NetworkDownloadContentDescriptor::clearContent()
{
    reboot = false;
    add_on = false;
    component_size = 0;
    session_protocol_number = 0;
    session_id = 0;
    retry = 0;
    connect_timer = 0;
    ipv4.reset();
    ipv6.reset();
    url.reset();
    compatibility_descriptor.clear();
    private_data.clear();
    text_info.reset();
}

ts::NetworkDownloadContentDescriptor::NetworkDownloadContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    NetworkDownloadContentDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NetworkDownloadContentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(reboot);
    buf.putBit(add_on);
    buf.putBit(!compatibility_descriptor.empty());
    buf.putBit(text_info.has_value());
    buf.putReserved(4);
    buf.putUInt32(component_size);
    buf.putUInt8(session_protocol_number);
    buf.putUInt32(session_id);
    buf.putUInt8(retry);
    buf.putUInt24(connect_timer);

    // Exactly one of ipv4, ipv6, url must be set.
    if (ipv4.has_value() + ipv6.has_value() + url.has_value() != 1) {
        buf.setUserError();
        return;
    }
    else if (ipv4.has_value()) {
        buf.putUInt8(0x00); // address_type = IPv4
        buf.putUInt32(ipv4.value().address());
        buf.putUInt16(ipv4.value().port());
    }
    else if (ipv6.has_value()) {
        buf.putUInt8(0x01); // address_type = IPv6
        buf.putBytes(ipv6.value().toBytes());
        buf.putUInt16(ipv6.value().port());
    }
    else if (url.has_value()) {
        buf.putUInt8(0x02); // address_type = URL
        buf.putUTF8WithLength(url.value());
    }

    if (!compatibility_descriptor.empty()) {
        compatibility_descriptor.serializePayload(buf);
    }
    buf.putUInt8(uint8_t(private_data.size()));
    buf.putBytes(private_data);
    if (text_info.has_value()) {
        text_info.value().serializePayload(buf);
    }
}

void ts::NetworkDownloadContentDescriptor::deserializePayload(PSIBuffer& buf)
{
    reboot = buf.getBool();
    add_on = buf.getBool();
    const bool compatibility_flag = buf.getBool();
    const bool text_info_flag = buf.getBool();
    buf.skipReservedBits(4);
    component_size = buf.getUInt32();
    session_protocol_number = buf.getUInt8();
    session_id = buf.getUInt32();
    retry = buf.getUInt8();
    connect_timer = buf.getUInt24();

    switch (buf.getUInt8()) {
        case 0x00: // address_type = IPv4
            ipv4.emplace(buf.getUInt32());
            ipv4.value().setPort(buf.getUInt16());
            break;
        case 0x01: // address_type = IPv6
            ipv6.emplace(buf.getBytes(IPv6Address::BYTES));
            ipv6.value().setPort(buf.getUInt16());
            break;
        case 0x02: // address_type = URL
            url = buf.getUTF8WithLength();
            break;
        default:
            break;
    }

    if (compatibility_flag) {
        compatibility_descriptor.deserializePayload(buf);
    }
    buf.getBytes(private_data, buf.getUInt8());
    if (text_info_flag) {
        text_info.emplace();
        text_info.value().deserializePayload(buf);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NetworkDownloadContentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(15)) {
        disp << margin << "Reboot: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Add-on: " << UString::TrueFalse(buf.getBool()) << std::endl;
        const bool compatibility_flag = buf.getBool();
        const bool text_info_flag = buf.getBool();
        buf.skipReservedBits(4);
        disp << margin << UString::Format(u"Component size: %d bytes", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"Session protocol number: %n", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"Session id: %n", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"Retry: %d", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"Connect timer: %d", buf.getUInt24()) << std::endl;

        const uint8_t address_type = buf.getUInt8();
        disp << margin << "Address type: " << DataName(MY_XML_NAME, u"address_type", address_type, NamesFlags::VALUE) << std::endl;

        bool ok = true;
        switch (address_type) {
            case 0x00: { // address_type = IPv4
                ok = buf.canReadBytes(6);
                if (ok) {
                    IPv4SocketAddress ipv4(buf.getUInt32());
                    ipv4.setPort(buf.getUInt16());
                    disp << margin << "IPv4: " << ipv4 << std::endl;
                }
                break;
            }
            case 0x01: { // address_type = IPv6
                ok = buf.canReadBytes(18);
                if (ok) {
                    IPv6SocketAddress ipv6(buf.getBytes(IPv6Address::BYTES));
                    ipv6.setPort(buf.getUInt16());
                    disp << margin << "IPv6: " << ipv6 << std::endl;
                }
                break;
            }
            case 0x02: { // address_type = URL
                disp << margin << "URL: " << buf.getUTF8WithLength() << std::endl;
                break;
            }
            default:
                break;
        }

        if (ok && compatibility_flag) {
            ok = CompatibilityDescriptor::Display(disp, buf, margin);
        }
        if (ok) {
            ok = buf.canReadBytes(1);
            if (ok) {
                const size_t count = buf.getUInt8();
                ok = buf.canReadBytes(count);
                disp.displayPrivateData(u"Private data", buf, count, margin);
            }
        }
        if (ok && text_info_flag) {
            disp << margin << "Text info:" << std::endl;
            TextInfo::Display(disp, buf, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NetworkDownloadContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"reboot", reboot);
    root->setBoolAttribute(u"add_on", add_on);
    root->setIntAttribute(u"component_size", component_size);
    root->setIntAttribute(u"session_protocol_number", session_protocol_number, true);
    root->setIntAttribute(u"session_id", session_id, true);
    root->setIntAttribute(u"retry", retry);
    root->setIntAttribute(u"connect_timer", connect_timer);

    if (ipv4.has_value()) {
        xml::Element* e = root->addElement(u"ipv4");
        e->setIPv4Attribute(u"address", IPv4Address(ipv4.value()));
        e->setIntAttribute(u"port", ipv4.value().port());
    }
    else if (ipv6.has_value()) {
        xml::Element* e = root->addElement(u"ipv6");
        e->setIPv6Attribute(u"address", IPv6Address(ipv6.value()));
        e->setIntAttribute(u"port", ipv6.value().port());
    }
    else if (url.has_value()) {
        root->addElement(u"url")->setAttribute(u"url", url.value());
    }

    if (!compatibility_descriptor.empty()) {
        compatibility_descriptor.buildXML(duck, root);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
    if (text_info.has_value()) {
        text_info.value().buildXML(duck, root);
    }
}

bool ts::NetworkDownloadContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xipv4, xipv6, xurl, xtext;
    bool ok =
        element->getBoolAttribute(reboot, u"reboot", true) &&
        element->getBoolAttribute(add_on, u"add_on", true) &&
        element->getIntAttribute(component_size, u"component_size", true) &&
        element->getIntAttribute(session_protocol_number, u"session_protocol_number", true) &&
        element->getIntAttribute(session_id, u"session_id", true) &&
        element->getIntAttribute(retry, u"retry", true) &&
        element->getIntAttribute(connect_timer, u"connect_timer", true, 0, 0, 0x00FFFFFF) &&
        element->getChildren(xipv4, u"ipv4", 0, 1) &&
        element->getChildren(xipv6, u"ipv6", 0, 1) &&
        element->getChildren(xurl, u"url", 0, 1) &&
        compatibility_descriptor.analyzeXML(duck, element) &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(xtext, u"text_info", 0, 1);

    if (xipv4.size() + xipv6.size() + xurl.size() != 1) {
        ok = false;
        element->report().error(u"exactly one of <ipv4>, <ipv6>, <url> required in <%s>, line %d", element->name(), element->lineNumber());
    }
    if (ok && !xipv4.empty()) {
        IPv4Address address;
        uint16_t port = 0;
        ok = xipv4[0]->getIPv4Attribute(address, u"address", true) && xipv4[0]->getIntAttribute(port, u"port", true);
        ipv4.emplace(address, port);
    }
    else if (ok && !xipv6.empty()) {
        IPv6Address address;
        uint16_t port = 0;
        ok = xipv6[0]->getIPv6Attribute(address, u"address", true) && xipv6[0]->getIntAttribute(port, u"port", true);
        ipv6.emplace(address, port);
    }
    else if (ok && !xurl.empty()) {
        UString str;
        ok = xurl[0]->getAttribute(str, u"url", true);
        url.emplace(str);
    }

    if (ok && !xtext.empty()) {
        text_info.emplace();
        ok = text_info.value().analyzeXML(duck, xtext[0]);
    }
    return ok;
}
