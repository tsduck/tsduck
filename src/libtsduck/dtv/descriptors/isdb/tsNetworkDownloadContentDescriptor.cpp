//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#define MY_CLASS    ts::NetworkDownloadContentDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_NETW_DOWNLOAD, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NetworkDownloadContentDescriptor::NetworkDownloadContentDescriptor() :
    AbstractDownloadContentDescriptor(MY_EDID, MY_XML_NAME)
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
    ip.reset();
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
    if (ip.has_value() + url.has_value() != 1) {
        buf.setUserError();
        return;
    }
    else if (ip.has_value()) {
        if (ip->generation() == IP::v4) {
            buf.putUInt8(0x00); // address_type = IPv4
            buf.putUInt32(ip.value().address4());
        }
        else {
            buf.putUInt8(0x01); // address_type = IPv6
            buf.putBytes(ip.value().address6());
        }
        buf.putUInt16(ip.value().port());
    }
    else if (url.has_value()) {
        buf.putUInt8(0x02); // address_type = URL
        buf.putUTF8WithLength(url.value());
    }

    if (!compatibility_descriptor.empty()) {
        compatibility_descriptor.serialize(buf);
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
        case 0x00: { // address_type = IPv4
            const uint32_t addr = buf.getUInt32();
            const uint16_t port = buf.getUInt16();
            ip.emplace(addr, port);
            break;
        }
        case 0x01: { // address_type = IPv6
            const ByteBlock addr = buf.getBytes(IPAddress::BYTES6);
            const uint16_t port = buf.getUInt16();
            ip.emplace(addr, port);
            break;
        }
        case 0x02: // address_type = URL
            url = buf.getUTF8WithLength();
            break;
        default:
            break;
    }

    if (compatibility_flag) {
        compatibility_descriptor.deserialize(buf);
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

void ts::NetworkDownloadContentDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
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
        disp << margin << "Address type: " << DataName(MY_XML_NAME, u"address_type", address_type, NamesFlags::NAME_VALUE) << std::endl;

        bool ok = true;
        switch (address_type) {
            case 0x00: { // address_type = IPv4
                ok = buf.canReadBytes(6);
                if (ok) {
                    const uint32_t addr = buf.getUInt32();
                    const uint16_t port = buf.getUInt16();
                    disp << margin << "IPv4: " << IPSocketAddress(addr, port) << std::endl;
                }
                break;
            }
            case 0x01: { // address_type = IPv6
                ok = buf.canReadBytes(18);
                if (ok) {
                    const ByteBlock addr = buf.getBytes(IPAddress::BYTES6);
                    const uint16_t port = buf.getUInt16();
                    disp << margin << "IPv6: " << IPSocketAddress(addr, port) << std::endl;
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
            ok = DSMCCCompatibilityDescriptor::Display(disp, buf, margin);
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

    if (ip.has_value()) {
        xml::Element* e = root->addElement(UString::Format(u"ipv%d", int(ip.value().generation())));
        e->setIPAttribute(u"address", IPAddress(ip.value()));
        e->setIntAttribute(u"port", ip.value().port());
    }
    else if (url.has_value()) {
        root->addElement(u"url")->setAttribute(u"url", url.value());
    }

    compatibility_descriptor.toXML(duck, root, true);
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
        compatibility_descriptor.fromXML(duck, element, false) &&
        element->getHexaTextChild(private_data, u"private_data", false) &&
        element->getChildren(xtext, u"text_info", 0, 1);

    if (xipv4.size() + xipv6.size() + xurl.size() != 1) {
        ok = false;
        element->report().error(u"exactly one of <ipv4>, <ipv6>, <url> required in <%s>, line %d", element->name(), element->lineNumber());
    }
    if (ok && !xipv4.empty()) {
        IPAddress address;
        uint16_t port = 0;
        ok = xipv4[0]->getIPAttribute(address, u"address", true) && xipv4[0]->getIntAttribute(port, u"port", true);
        ip.emplace(address, port);
    }
    else if (ok && !xipv6.empty()) {
        IPAddress address;
        uint16_t port = 0;
        ok = xipv6[0]->getIPAttribute(address, u"address", true) && xipv6[0]->getIntAttribute(port, u"port", true);
        ip.emplace(address, port);
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
