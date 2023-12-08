//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"linkage_descriptor"
#define MY_CLASS ts::LinkageDescriptor
#define MY_DID ts::DID_LINKAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors for substructures
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::MobileHandoverInfo::clear()
{
    handover_type = 0;
    origin_type = 0;
    network_id = 0;
    initial_service_id = 0;
}

void ts::LinkageDescriptor::EventLinkageInfo::clear()
{
    target_event_id = 0;
    target_listed = false;
    event_simulcast = false;
}

void ts::LinkageDescriptor::ExtendedEventLinkageInfo::clear()
{
    target_event_id = 0;
    target_listed = false;
    event_simulcast = false;
    link_type = 0;
    target_id_type = 0;
    user_defined_id = 0;
    target_transport_stream_id = 0;
    target_original_network_id.reset();
    target_service_id.reset();
}


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint8_t ltype) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    linkage_type(ltype)
{
}

void ts::LinkageDescriptor::clearContent()
{
    ts_id = 0;
    onetw_id = 0;
    service_id = 0;
    linkage_type = 0;
    mobile_handover_info.clear();
    event_linkage_info.clear();
    extended_event_linkage_info.clear();
    private_data.clear();
}

ts::LinkageDescriptor::LinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    LinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    // Fixed part.
    buf.putUInt16(ts_id);
    buf.putUInt16(onetw_id);
    buf.putUInt16(service_id);
    buf.putUInt8(linkage_type);

    // Known variable parts.
    if (linkage_type == LINKAGE_HAND_OVER) {
        buf.putBits(mobile_handover_info.handover_type, 4);
        buf.putBits(0xFF, 3);
        buf.putBit(mobile_handover_info.origin_type);
        if (mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
            buf.putUInt16(mobile_handover_info.network_id);
        }
        if (mobile_handover_info.origin_type == 0x00) {
            buf.putUInt16(mobile_handover_info.initial_service_id);
        }
    }
    else if (linkage_type == LINKAGE_EVENT) {
        buf.putUInt16(event_linkage_info.target_event_id);
        buf.putBit(event_linkage_info.target_listed);
        buf.putBit(event_linkage_info.event_simulcast);
        buf.putBits(0xFF, 6);
    }
    else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
        buf.pushWriteSequenceWithLeadingLength(8); // loop_length
        for (const auto& it : extended_event_linkage_info) {
            buf.putUInt16(it.target_event_id);
            buf.putBit(it.target_listed);
            buf.putBit(it.event_simulcast);
            buf.putBits(it.link_type, 2);
            buf.putBits(it.target_id_type, 2);
            buf.putBit(it.target_original_network_id.has_value());
            buf.putBit(it.target_service_id.has_value());
            if (it.target_id_type == 3) {
                buf.putUInt16(it.user_defined_id);
            }
            else {
                if (it.target_id_type == 1) {
                    buf.putUInt16(it.target_transport_stream_id);
                }
                if (it.target_original_network_id.has_value()) {
                    buf.putUInt16(it.target_original_network_id.value());
                }
                if (it.target_service_id.has_value()) {
                    buf.putUInt16(it.target_service_id.value());
                }
            }
        }
        buf.popState(); // update loop_length
    }

    // Finally, add private data.
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    // Fixed part.
    ts_id = buf.getUInt16();
    onetw_id = buf.getUInt16();
    service_id = buf.getUInt16();
    linkage_type = buf.getUInt8();

    // Known variable parts.
    if (linkage_type == LINKAGE_HAND_OVER) {
        buf.getBits(mobile_handover_info.handover_type, 4);
        buf.skipBits(3);
        mobile_handover_info.origin_type = buf.getBit();
        if (mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
            mobile_handover_info.network_id = buf.getUInt16();
        }
        if (mobile_handover_info.origin_type == 0x00) {
            mobile_handover_info.initial_service_id = buf.getUInt16();
        }
    }
    else if (linkage_type == LINKAGE_EVENT) {
        event_linkage_info.target_event_id = buf.getUInt16();
        event_linkage_info.target_listed = buf.getBool();
        event_linkage_info.event_simulcast = buf.getBool();
        buf.skipBits(6);
    }
    else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
        buf.pushReadSizeFromLength(8); // loop_length
        while (buf.canRead()) {
            ExtendedEventLinkageInfo info;
            info.target_event_id = buf.getUInt16();
            info.target_listed = buf.getBool();
            info.event_simulcast = buf.getBool();
            buf.getBits(info.link_type, 2);
            buf.getBits(info.target_id_type, 2);
            const bool onetw_flag = buf.getBool();
            const bool serv_flag = buf.getBool();
            if (info.target_id_type == 3) {
                info.user_defined_id = buf.getUInt16();
            }
            else {
                if (info.target_id_type == 1) {
                    info.target_transport_stream_id = buf.getUInt16();
                }
                if (onetw_flag) {
                    info.target_original_network_id = buf.getUInt16();
                }
                if (serv_flag) {
                    info.target_service_id = buf.getUInt16();
                }
            }
            extended_event_linkage_info.push_back(info);
        }
        buf.popState(); // end of loop_length
    }

    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"Transport stream id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Service id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        const uint8_t ltype = buf.getUInt8();
        disp << margin << UString::Format(u"Linkage type: %s", {DataName(MY_XML_NAME, u"linkage_type", ltype, NamesFlags::FIRST)}) << std::endl;

        // Variable part
        switch (ltype) {
            case 0x08:
                DisplayPrivateMobileHandover(disp, buf, margin, ltype);
                break;
            case 0x09:
                DisplayPrivateSSU(disp, buf, margin, ltype);
                break;
            case 0x0A:
                DisplayPrivateTableSSU(disp, buf, margin, ltype);
                break;
            case 0x0B:
                DisplayPrivateINT(disp, buf, margin, ltype);
                break;
            case 0x0C:
                DisplayPrivateDeferredINT(disp, buf, margin, ltype);
                break;
            default:
                break;
        }

        // Remaining private data
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for mobile hand-over
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateMobileHandover(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t ltype)
{
    if (buf.canReadBytes(1)) {
        const uint8_t hand_over = buf.getBits<uint8_t>(4);
        buf.skipBits(3);
        const uint8_t origin = buf.getBit();

        const UChar* name = u"";
        switch (hand_over) {
            case 0x01: name = u"identical service in neighbour country"; break;
            case 0x02: name = u"local variation of same service"; break;
            case 0x03: name = u"associated service"; break;
            default:   name = u"unknown"; break;
        }
        disp << margin << UString::Format(u"Hand-over type: 0x%X, %s, Origin: %s", {hand_over, name, origin ? u"SDT" : u"NIT"}) << std::endl;

        if (hand_over >= 1 && hand_over <= 3 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"Network id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        }
        if (origin == 0x00 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"Original service id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for System Software Update.
// See ETSI TS 102 006, section 6.1, system_software_update_link_structure()
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateSSU(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t ltype)
{
    buf.pushReadSizeFromLength(8); // OUI_data_length
    while (buf.canReadBytes(4)) {
        disp << margin << "OUI: " << NameFromOUI(buf.getUInt24(), NamesFlags::FIRST) << std::endl;
        const size_t len = buf.getUInt8();
        disp.displayPrivateData(u"Selector data", buf, len, margin);
    }
    disp.displayPrivateData(u"Extraneous OUI data", buf, NPOS, margin);
    buf.popState(); // end of OUI_data_length
}


//----------------------------------------------------------------------------
// Display linkage private data for TS with System Software Update BAT or NIT
// See ETSI TS 102 006, section 6.1.1
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateTableSSU(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t ltype)
{
    if (buf.canReadBytes(1)) {
        const uint8_t ttype = buf.getUInt8();
        disp << margin << "SSU table type: ";
        switch (ttype) {
            case 1:  disp << "NIT"; break;
            case 2:  disp << "BAT"; break;
            default: disp << UString::Hexa(ttype); break;
        }
        disp << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for INT.
// See ETSI EN 301 192, section 8.2.1.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateINT(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t ltype)
{
    buf.pushReadSizeFromLength(8); // platform_id_data_length
    while (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"- Platform id: %s", {DataName(u"INT", u"platform_id", buf.getUInt24(), NamesFlags::HEXA_FIRST)}) << std::endl;
        buf.pushReadSizeFromLength(8); // platform_name_loop_length
        while (buf.canReadBytes(4)) {
            disp << margin << "  Language: " << buf.getLanguageCode();
            disp << ", name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
        disp.displayPrivateData(u"Extraneous platform name data", buf, NPOS, margin + u"  ");
        buf.popState(); // end of platform_name_loop_length
    }
    disp.displayPrivateData(u"Extraneous platform data", buf, NPOS, margin);
    buf.popState(); // end of platform_id_data_length
}


//----------------------------------------------------------------------------
// Display linkage private data for deferred INT.
// See ETSI EN 301 192, section 8.2.2.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateDeferredINT(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint8_t ltype)
{
    if (buf.canReadBytes(1)) {
        const uint8_t ttype = buf.getUInt8();
        disp << margin << "INT linkage table type: ";
        switch (ttype) {
            case 0:  disp << "unspecified"; break;
            case 1:  disp << "NIT"; break;
            case 2:  disp << "BAT"; break;
            default: disp << UString::Hexa(ttype); break;
        }
        disp << std::endl;
        if (ttype == 0x02 && buf.canReadBytes(2)) {
            disp << margin << UString::Format(u"Bouquet id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration OriginTypeNames({{u"NIT", 0}, {u"SDT", 1}});
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_stream_id", ts_id, true);
    root->setIntAttribute(u"original_network_id", onetw_id, true);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"linkage_type", linkage_type, true);

    if (linkage_type == LINKAGE_HAND_OVER) {
        xml::Element* e = root->addElement(u"mobile_handover_info");
        e->setIntAttribute(u"handover_type", mobile_handover_info.handover_type, true);
        e->setIntEnumAttribute(OriginTypeNames, u"origin_type", mobile_handover_info.origin_type);
        if (mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
            e->setIntAttribute(u"network_id", mobile_handover_info.network_id, true);
        }
        if (mobile_handover_info.origin_type == 0x00) {
            e->setIntAttribute(u"initial_service_id", mobile_handover_info.initial_service_id, true);
        }
    }
    else if (linkage_type == LINKAGE_EVENT) {
        xml::Element* e = root->addElement(u"event_linkage_info");
        e->setIntAttribute(u"target_event_id", event_linkage_info.target_event_id, true);
        e->setBoolAttribute(u"target_listed", event_linkage_info.target_listed);
        e->setBoolAttribute(u"event_simulcast", event_linkage_info.event_simulcast);
    }
    else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
        xml::Element* extInfo = root->addElement(u"extended_event_linkage_info");
        for (const auto& it : extended_event_linkage_info) {
            xml::Element* e = extInfo->addElement(u"event");
            e->setIntAttribute(u"target_event_id", it.target_event_id, true);
            e->setBoolAttribute(u"target_listed", it.target_listed);
            e->setBoolAttribute(u"event_simulcast", it.event_simulcast);
            e->setIntAttribute(u"link_type", it.link_type, true);
            e->setIntAttribute(u"target_id_type", it.target_id_type, true);
            if (it.target_id_type == 3) {
                e->setIntAttribute(u"user_defined_id", it.user_defined_id, true);
            }
            else {
                if (it.target_id_type == 1) {
                    e->setIntAttribute(u"target_transport_stream_id", it.target_transport_stream_id, true);
                }
                if (it.target_original_network_id.has_value()) {
                    e->setIntAttribute(u"target_original_network_id", it.target_original_network_id.value(), true);
                }
                if (it.target_service_id.has_value()) {
                    e->setIntAttribute(u"target_service_id", it.target_service_id.value(), true);
                }
            }
        }
    }

    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(ts_id, u"transport_stream_id", true) &&
        element->getIntAttribute(onetw_id, u"original_network_id", true) &&
        element->getIntAttribute(service_id, u"service_id", true) &&
        element->getIntAttribute(linkage_type, u"linkage_type", true) &&
        element->getHexaTextChild(private_data, u"private_data", false);

    xml::ElementVector mobileElements;
    xml::ElementVector eventElements;
    xml::ElementVector extEventElements;

    if (ok) {
        const size_t mobileCount = linkage_type == LINKAGE_HAND_OVER ? 1 : 0;
        const size_t eventCount = linkage_type == LINKAGE_EVENT ? 1 : 0;
        const size_t extEventCount = linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX ? 1 : 0;
        ok = element->getChildren(mobileElements, u"mobile_handover_info", mobileCount, mobileCount) &&
             element->getChildren(eventElements, u"event_linkage_info", eventCount, eventCount) &&
             element->getChildren(extEventElements, u"extended_event_linkage_info", extEventCount, extEventCount);
    }

    if (ok && !mobileElements.empty()) {
        ok = mobileElements[0]->getIntAttribute(mobile_handover_info.handover_type, u"handover_type", true, 0, 0, 0x0F) &&
             mobileElements[0]->getIntEnumAttribute(mobile_handover_info.origin_type, OriginTypeNames, u"origin_type", true) &&
             mobileElements[0]->getIntAttribute(mobile_handover_info.network_id, u"network_id",
                                                          mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) &&
             mobileElements[0]->getIntAttribute(mobile_handover_info.initial_service_id, u"initial_service_id",
                                                          mobile_handover_info.origin_type == 0x00);
    }

    if (ok && !eventElements.empty()) {
        ok = eventElements[0]->getIntAttribute(event_linkage_info.target_event_id, u"target_event_id", true) &&
             eventElements[0]->getBoolAttribute(event_linkage_info.target_listed, u"target_listed", true) &&
             eventElements[0]->getBoolAttribute(event_linkage_info.event_simulcast, u"event_simulcast", true);
    }

    if (ok && !extEventElements.empty()) {
        ok = extEventElements[0]->getChildren(eventElements, u"event");
        for (size_t i = 0; ok && i < eventElements.size(); ++i) {
            ExtendedEventLinkageInfo info;
            ok = eventElements[i]->getIntAttribute(info.target_event_id, u"target_event_id", true) &&
                 eventElements[i]->getBoolAttribute(info.target_listed, u"target_listed", true) &&
                 eventElements[i]->getBoolAttribute(info.event_simulcast, u"event_simulcast", true) &&
                 eventElements[i]->getIntAttribute(info.link_type, u"link_type", true, 0, 0, 3) &&
                 eventElements[i]->getIntAttribute(info.target_id_type, u"target_id_type", true, 0, 0, 3) &&
                 eventElements[i]->getIntAttribute(info.user_defined_id, u"user_defined_id", info.target_id_type == 3) &&
                 eventElements[i]->getIntAttribute(info.target_transport_stream_id, u"target_transport_stream_id", info.target_id_type == 1) &&
                 eventElements[i]->getOptionalIntAttribute(info.target_original_network_id, u"target_original_network_id") &&
                 eventElements[i]->getOptionalIntAttribute(info.target_service_id, u"target_service_id");
            extended_event_linkage_info.push_back(info);
        }
    }
    return ok;
}
