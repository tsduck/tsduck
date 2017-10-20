//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a generic linkage_descriptor.
//  Specialized classes exist, depending on the linkage type.
//
//----------------------------------------------------------------------------

#include "tsLinkageDescriptor.h"
#include "tsFormat.h"
#include "tsHexa.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::LinkageDescriptor, "linkage_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::LinkageDescriptor, ts::EDID(ts::DID_LINKAGE));
TS_ID_DESCRIPTOR_DISPLAY(ts::LinkageDescriptor::DisplayDescriptor, ts::EDID(ts::DID_LINKAGE));


//----------------------------------------------------------------------------
// Constructors for substructures
//----------------------------------------------------------------------------

ts::LinkageDescriptor::MobileHandoverInfo::MobileHandoverInfo() :
    handover_type(0),
    origin_type(0),
    network_id(0),
    initial_service_id(0)
{
}

void ts::LinkageDescriptor::MobileHandoverInfo::clear()
{
    handover_type = 0;
    origin_type = 0;
    network_id = 0;
    initial_service_id = 0;
}

ts::LinkageDescriptor::EventLinkageInfo::EventLinkageInfo() :
    target_event_id(0),
    target_listed(false),
    event_simulcast(false)
{
}

void ts::LinkageDescriptor::EventLinkageInfo::clear()
{
    target_event_id = 0;
    target_listed = false;
    event_simulcast = false;
}

ts::LinkageDescriptor::ExtendedEventLinkageInfo::ExtendedEventLinkageInfo() :
    target_event_id(0),
    target_listed(false),
    event_simulcast(false),
    link_type(0),
    target_id_type(0),
    user_defined_id(0),
    target_transport_stream_id(0),
    target_original_network_id(),
    target_service_id()
{
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

void ts::LinkageDescriptor::clear()
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


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint8_t ltype) :
    AbstractDescriptor(DID_LINKAGE, "linkage_descriptor"),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    linkage_type(ltype),
    mobile_handover_info(),
    event_linkage_info(),
    extended_event_linkage_info(),
    private_data()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(DID_LINKAGE, "linkage_descriptor"),
    ts_id(0),
    onetw_id(0),
    service_id(0),
    linkage_type(0),
    mobile_handover_info(),
    event_linkage_info(),
    extended_event_linkage_info(),
    private_data()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    if (!_is_valid) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    // Fixed part.
    bbp->appendUInt16(ts_id);
    bbp->appendUInt16(onetw_id);
    bbp->appendUInt16(service_id);
    bbp->appendUInt8(linkage_type);

    // Known variable parts.
    if (linkage_type == LINKAGE_HAND_OVER) {
        bbp->appendUInt8((mobile_handover_info.handover_type << 4) | 0x0E | (mobile_handover_info.origin_type & 0x01));
        if (mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
            bbp->appendUInt16(mobile_handover_info.network_id);
        }
        if (mobile_handover_info.origin_type == 0x00) {
            bbp->appendUInt16(mobile_handover_info.initial_service_id);
        }
    }
    else if (linkage_type == LINKAGE_EVENT) {
        bbp->appendUInt16(event_linkage_info.target_event_id);
        bbp->appendUInt8((event_linkage_info.target_listed ? 0x80 : 0x00) |
                         (event_linkage_info.event_simulcast ? 0x40 : 0x00) |
                         0x3F);
    }
    else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
        const size_t length_index = bbp->size();
        bbp->appendUInt8(0); // placeholder for loop_length
        for (ExtendedEventLinkageList::const_iterator it = extended_event_linkage_info.begin(); it != extended_event_linkage_info.end(); ++it) {
            bbp->appendUInt16(it->target_event_id);
            bbp->appendUInt8((it->target_listed ? 0x80 : 0x00) |
                             (it->event_simulcast ? 0x40 : 0x00) |
                             ((it->link_type & 0x03) << 4) |
                             ((it->target_id_type & 0x03) << 2) |
                             (it->target_original_network_id.set() ? 0x02 : 0x00) |
                             (it->target_service_id.set() ? 0x01 : 0x00));
            if (it->target_id_type == 3) {
                bbp->appendUInt16(it->user_defined_id);
            }
            if (it->target_id_type == 1) {
                bbp->appendUInt16(it->target_transport_stream_id);
            }
            if (it->target_original_network_id.set()) {
                bbp->appendUInt16(it->target_original_network_id.value());
            }
            if (it->target_service_id.set()) {
                bbp->appendUInt16(it->target_service_id.value());
            }
        }
        // Update loop_length.
        (*bbp)[length_index] = uint8_t(bbp->size() - length_index - 1);
    }

    // Finally, add private data.
    bbp->append(private_data);

    // We have serialized many things, check that it fits in a descriptor.
    if (bbp->size() <= MAX_DESCRIPTOR_SIZE) {
        (*bbp)[0] = _tag;
        (*bbp)[1] = uint8_t(bbp->size() - 2);
        Descriptor d(bbp, SHARE);
        desc = d;
    }
    else {
        desc.invalidate();
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    clear();
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 7;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();

        // Fixed part.
        ts_id = GetUInt16(data);
        onetw_id = GetUInt16(data + 2);
        service_id = GetUInt16(data + 4);
        linkage_type = data[6];
        data += 7; size -= 7;

        // Known variable parts.
        if (linkage_type == LINKAGE_HAND_OVER) {
            _is_valid = size >= 1;
            if (_is_valid) {
                mobile_handover_info.handover_type = data[0] >> 4;
                mobile_handover_info.origin_type = data[0] & 0x01;
                data += 1; size -= 1;
            }
            if (_is_valid && mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
                _is_valid = size >= 2;
                if (_is_valid) {
                    mobile_handover_info.network_id = GetUInt16(data);
                    data += 2; size -= 2;
                }
            }
            if (_is_valid && mobile_handover_info.origin_type == 0x00) {
                _is_valid = size >= 2;
                if (_is_valid) {
                    mobile_handover_info.initial_service_id = GetUInt16(data);
                    data += 2; size -= 2;
                }
            }
        }

        else if (linkage_type == LINKAGE_EVENT) {
            _is_valid = size >= 3;
            if (_is_valid) {
                event_linkage_info.target_event_id = GetUInt16(data);
                event_linkage_info.target_listed = (data[2] & 0x80) != 0;
                event_linkage_info.event_simulcast = (data[2] & 0x40) != 0;
                data += 3; size -= 3;
            }
        }

        else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
            _is_valid = size >= 1;
            if (_is_valid) {
                size_t loop_length = data[0];
                data += 1; size -= 1;
                while (_is_valid && loop_length > 0) {
                    _is_valid = loop_length >= 3;
                    ExtendedEventLinkageInfo info;
                    bool onetw_flag = false;
                    bool serv_flag = false;
                    if (_is_valid) {
                        info.target_event_id = GetUInt16(data);
                        info.target_listed = (data[2] & 0x80) != 0;
                        info.event_simulcast = (data[2] & 0x40) != 0;
                        info.link_type = (data[2] >> 4) & 0x03;
                        info.target_id_type = (data[2] >> 2) & 0x03;
                        onetw_flag = (data[2] & 0x02) != 0;
                        serv_flag = (data[2] & 0x01) != 0;
                        data += 3; size -= 3; loop_length -= 3;
                    }
                    if (_is_valid && info.target_id_type == 3) {
                        _is_valid = size >= 2;
                        if (_is_valid) {
                            info.user_defined_id = GetUInt16(data);
                            data += 2; size -= 2; loop_length -= 2;
                        }
                    }
                    if (_is_valid && info.target_id_type == 1) {
                        _is_valid = size >= 2;
                        if (_is_valid) {
                            info.target_transport_stream_id = GetUInt16(data);
                            data += 2; size -= 2; loop_length -= 2;
                        }
                    }
                    if (_is_valid && onetw_flag) {
                        _is_valid = size >= 2;
                        if (_is_valid) {
                            info.target_original_network_id = GetUInt16(data);
                            data += 2; size -= 2; loop_length -= 2;
                        }
                    }
                    if (_is_valid && serv_flag) {
                        _is_valid = size >= 2;
                        if (_is_valid) {
                            info.target_service_id = GetUInt16(data);
                            data += 2; size -= 2; loop_length -= 2;
                        }
                    }
                    if (_is_valid) {
                        extended_event_linkage_info.push_back(info);
                    }
                }
            }
        }

        // Remaining bytes are private data.
        if (_is_valid) {
            private_data.copy(data, size);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 7) {

        // Fixed part
        uint16_t tsid = GetUInt16(data);
        uint16_t onid = GetUInt16(data + 2);
        uint16_t servid = GetUInt16(data + 4);
        uint8_t ltype = data[6];
        data += 7; size -= 7;
        strm << margin << "Transport stream id: " << tsid << Format(" (0x%04X)", int(tsid)) << std::endl
             << margin << "Original network Id: " << onid << Format(" (0x%04X)", int(onid)) << std::endl
             << margin << "Service id: " << servid << Format(" (0x%04X)", int(servid)) << std::endl
             << margin << "Linkage type: " << names::LinkageType(ltype, names::FIRST) << std::endl;

        // Variable part
        if (ltype == 0x08 && size >= 1) {
            // Mobile hand-over
            uint8_t hand_over = *data >> 4;
            uint8_t origin = *data & 0x01;
            data += 1; size -= 1;
            const char *name;
            switch (hand_over) {
                case 0x01: name = "identical service in neighbour country"; break;
                case 0x02: name = "local variation of same service"; break;
                case 0x03: name = "associated service"; break;
                default:   name = "unknown"; break;
            }
            strm << margin << "Hand-over type: " << Format("0x%02X", int(hand_over))
                 << ", " << name << ", Origin: " << (origin ? "SDT" : "NIT") << std::endl;
            if ((hand_over == 0x01 || hand_over == 0x02 || hand_over == 0x03) && size >= 2) {
                uint16_t nwid = GetUInt16(data);
                data += 2; size -= 2;
                strm << margin << "Network id: " << nwid << Format(" (0x%04X)", int(nwid)) << std::endl;
            }
            if (origin == 0x00 && size >= 2) {
                uint16_t org_servid = GetUInt16(data);
                data += 2; size -= 2;
                strm << margin << "Original service id: " << org_servid << Format(" (0x%04X)", int(org_servid)) << std::endl;
            }
        }
        else if (ltype == 0x09 && size >= 1) {
            // System Software Update (ETSI TS 102 006)
            uint8_t dlength = data[0];
            data += 1; size -= 1;
            if (dlength > size) {
                dlength = uint8_t(size);
            }
            while (dlength >= 4) {
                uint32_t oui = GetUInt32(data - 1) & 0x00FFFFFF; // 24 bits
                uint8_t slength = data[3];
                data += 4; size -= 4; dlength -= 4;
                const uint8_t* sdata = data;
                if (slength > dlength) {
                    slength = dlength;
                }
                data += slength; size -= slength; dlength -= slength;
                strm << margin << "OUI: " << names::OUI(oui, names::FIRST) << std::endl;
                if (slength > 0) {
                    strm << margin << "Selector data:" << std::endl
                         << Hexa(sdata, slength, hexa::HEXA | hexa::ASCII, indent);
                }
            }
        }
        else if (ltype == 0x0A && size >= 1) {
            // TS with System Software Update BAT or NIT (ETSI TS 102 006)
            uint8_t ttype = data[0];
            data += 1; size -= 1;
            strm << margin << "SSU table type: ";
            switch (ttype) {
                case 0x01: strm << "NIT"; break;
                case 0x02: strm << "BAT"; break;
                default:   strm << Format("0x%02x", int(ttype)); break;
            }
            strm << std::endl;
        }

        // Remaining private data
        if (size > 0) {
            strm << margin << "Private data:" << std::endl
                 << Hexa(data, size, hexa::HEXA | hexa::ASCII | hexa::OFFSET, indent);
            data += size; size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration OriginTypeNames(
        "NIT", 0,
        "SDT", 1,
        TS_NULL);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::LinkageDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "transport_stream_id", ts_id, true);
    xml.setIntAttribute(root, "original_network_id", onetw_id, true);
    xml.setIntAttribute(root, "service_id", service_id, true);
    xml.setIntAttribute(root, "linkage_type", linkage_type, true);

    if (linkage_type == LINKAGE_HAND_OVER) {
        XML::Element* e = xml.addElement(root, "mobile_handover_info");
        xml.setIntAttribute(e, "handover_type", mobile_handover_info.handover_type, true);
        xml.setIntEnumAttribute(OriginTypeNames, e, "origin_type", mobile_handover_info.origin_type);
        if (mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) {
            xml.setIntAttribute(e, "network_id", mobile_handover_info.network_id, true);
        }
        if (mobile_handover_info.origin_type == 0x00) {
            xml.setIntAttribute(e, "initial_service_id", mobile_handover_info.initial_service_id, true);
        }
    }
    else if (linkage_type == LINKAGE_EVENT) {
        XML::Element* e = xml.addElement(root, "event_linkage_info");
        xml.setIntAttribute(e, "target_event_id", event_linkage_info.target_event_id, true);
        xml.setBoolAttribute(e, "target_listed", event_linkage_info.target_listed);
        xml.setBoolAttribute(e, "event_simulcast", event_linkage_info.event_simulcast);
    }
    else if (linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX) {
        XML::Element* extInfo = xml.addElement(root, "extended_event_linkage_info");
        for (ExtendedEventLinkageList::const_iterator it = extended_event_linkage_info.begin(); it != extended_event_linkage_info.end(); ++it) {
            XML::Element* e = xml.addElement(extInfo, "event");
            xml.setIntAttribute(e, "target_event_id", it->target_event_id, true);
            xml.setBoolAttribute(e, "target_listed", it->target_listed);
            xml.setBoolAttribute(e, "event_simulcast", it->event_simulcast);
            xml.setIntAttribute(e, "link_type", it->link_type, true);
            xml.setIntAttribute(e, "target_id_type", it->target_id_type, true);
            if (it->target_id_type == 3) {
                xml.setIntAttribute(e, "user_defined_id", it->user_defined_id, true);
            }
            if (it->target_id_type == 1) {
                xml.setIntAttribute(e, "target_transport_stream_id", it->target_transport_stream_id, true);
            }
            if (it->target_original_network_id.set()) {
                xml.setIntAttribute(e, "target_original_network_id", it->target_original_network_id.value(), true);
            }
            if (it->target_service_id.set()) {
                xml.setIntAttribute(e, "target_service_id", it->target_service_id.value(), true);
            }
        }
    }

    if (!private_data.empty()) {
        xml.addHexaText(xml.addElement(root, "private_data"), private_data);
    }
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    clear();

    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint16_t>(ts_id, element, "transport_stream_id", true) &&
        xml.getIntAttribute<uint16_t>(onetw_id, element, "original_network_id", true) &&
        xml.getIntAttribute<uint16_t>(service_id, element, "service_id", true) &&
        xml.getIntAttribute<uint8_t>(linkage_type, element, "linkage_type", true) &&
        xml.getHexaTextChild(private_data, element, "private_data", false);

    XML::ElementVector mobileElements;
    XML::ElementVector eventElements;
    XML::ElementVector extEventElements;

    if (_is_valid) {
        const size_t mobileCount = linkage_type == LINKAGE_HAND_OVER ? 1 : 0;
        const size_t eventCount = linkage_type == LINKAGE_EVENT ? 1 : 0;
        const size_t extEventCount = linkage_type >= LINKAGE_EXT_EVENT_MIN && linkage_type <= LINKAGE_EXT_EVENT_MAX ? 1 : 0;
        _is_valid =
            xml.getChildren(mobileElements, element, "mobile_handover_info", mobileCount, mobileCount) &&
            xml.getChildren(eventElements, element, "event_linkage_info", eventCount, eventCount) &&
            xml.getChildren(extEventElements, element, "extended_event_linkage_info", extEventCount, extEventCount);
    }

    if (_is_valid && !mobileElements.empty()) {
        _is_valid =
            xml.getIntAttribute<uint8_t>(mobile_handover_info.handover_type, mobileElements[0], "handover_type", true, 0, 0, 0x0F) &&
            xml.getIntEnumAttribute(mobile_handover_info.origin_type, OriginTypeNames, mobileElements[0], "origin_type", true) &&
            xml.getIntAttribute<uint16_t>(mobile_handover_info.network_id, mobileElements[0], "network_id",
                                          mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) &&
            xml.getIntAttribute<uint16_t>(mobile_handover_info.initial_service_id, mobileElements[0], "initial_service_id",
                                          mobile_handover_info.origin_type == 0x00);
    }

    if (_is_valid && !eventElements.empty()) {
        _is_valid =
            xml.getIntAttribute<uint16_t>(event_linkage_info.target_event_id, eventElements[0], "target_event_id", true) &&
            xml.getBoolAttribute(event_linkage_info.target_listed, eventElements[0], "target_listed", true) &&
            xml.getBoolAttribute(event_linkage_info.event_simulcast, eventElements[0], "event_simulcast", true);
    }
    
    if (_is_valid && !extEventElements.empty()) {
        _is_valid = xml.getChildren(eventElements, extEventElements[0], "event");
        for (size_t i = 0; _is_valid && i < eventElements.size(); ++i) {
            ExtendedEventLinkageInfo info;
            _is_valid =
                xml.getIntAttribute<uint16_t>(info.target_event_id, eventElements[i], "target_event_id", true) &&
                xml.getBoolAttribute(info.target_listed, eventElements[i], "target_listed", true) &&
                xml.getBoolAttribute(info.event_simulcast, eventElements[i], "event_simulcast", true) &&
                xml.getIntAttribute<uint8_t>(info.link_type, eventElements[i], "link_type", true, 0, 0, 3) &&
                xml.getIntAttribute<uint8_t>(info.target_id_type, eventElements[i], "target_id_type", true, 0, 0, 3) &&
                xml.getIntAttribute<uint16_t>(info.user_defined_id, eventElements[i], "user_defined_id", info.target_id_type == 3) &&
                xml.getIntAttribute<uint16_t>(info.target_transport_stream_id, eventElements[i], "target_transport_stream_id", info.target_id_type == 1) &&
                xml.getOptionalIntAttribute<uint16_t>(info.target_original_network_id, eventElements[i], "target_original_network_id") &&
                xml.getOptionalIntAttribute<uint16_t>(info.target_service_id, eventElements[i], "target_service_id");
            if (_is_valid) {
                extended_event_linkage_info.push_back(info);
            }
        }
    }
}
