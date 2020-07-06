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

#include "tsLinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"linkage_descriptor"
#define MY_CLASS ts::LinkageDescriptor
#define MY_DID ts::DID_LINKAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


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
    target_original_network_id.clear();
    target_service_id.clear();
}


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor(uint16_t ts, uint16_t onetw, uint16_t service, uint8_t ltype) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ts_id(ts),
    onetw_id(onetw),
    service_id(service),
    linkage_type(ltype),
    mobile_handover_info(),
    event_linkage_info(),
    extended_event_linkage_info(),
    private_data()
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

void ts::LinkageDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    // Fixed part.
    bbp->appendUInt16(ts_id);
    bbp->appendUInt16(onetw_id);
    bbp->appendUInt16(service_id);
    bbp->appendUInt8(linkage_type);

    // Known variable parts.
    if (linkage_type == LINKAGE_HAND_OVER) {
        bbp->appendUInt8(uint8_t(mobile_handover_info.handover_type << 4) | 0x0E | (mobile_handover_info.origin_type & 0x01));
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
                             uint8_t((it->link_type & 0x03) << 4) |
                             uint8_t((it->target_id_type & 0x03) << 2) |
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

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    clear();
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 7;

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
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 7) {

        // Fixed part
        uint16_t tsid = GetUInt16(data);
        uint16_t onid = GetUInt16(data + 2);
        uint16_t servid = GetUInt16(data + 4);
        uint8_t ltype = data[6];
        data += 7; size -= 7;
        strm << margin << UString::Format(u"Transport stream id: %d (0x%X)", {tsid, tsid}) << std::endl
             << margin << UString::Format(u"Original network Id: %d (0x%X)", {onid, onid}) << std::endl
             << margin << UString::Format(u"Service id: %d (0x%X)", {servid, servid}) << std::endl
             << margin << UString::Format(u"Linkage type: %s", {names::LinkageType(ltype, names::FIRST)}) << std::endl;

        // Variable part
        switch (ltype) {
            case 0x08:
                DisplayPrivateMobileHandover(display, data, size, indent, ltype);
                break;
            case 0x09:
                DisplayPrivateSSU(display, data, size, indent, ltype);
                break;
            case 0x0A:
                DisplayPrivateTableSSU(display, data, size, indent, ltype);
                break;
            case 0x0B:
                DisplayPrivateINT(display, data, size, indent, ltype);
                break;
            case 0x0C:
                DisplayPrivateDeferredINT(display, data, size, indent, ltype);
                break;
            default:
                break;
        }

        // Remaining private data
        display.displayPrivateData(u"Private data", data, size, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for mobile hand-over
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateMobileHandover(TablesDisplay& display, const uint8_t*& data, size_t& size, int indent, uint8_t ltype)
{
    if (size < 1) {
        return;
    }

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    uint8_t hand_over = *data >> 4;
    uint8_t origin = *data & 0x01;
    data += 1; size -= 1;

    const UChar *name;
    switch (hand_over) {
        case 0x01: name = u"identical service in neighbour country"; break;
        case 0x02: name = u"local variation of same service"; break;
        case 0x03: name = u"associated service"; break;
        default:   name = u"unknown"; break;
    }
    strm << margin << UString::Format(u"Hand-over type: 0x%X, %s, Origin: %s", {hand_over, name, origin ? u"SDT" : u"NIT"}) << std::endl;

    if ((hand_over == 0x01 || hand_over == 0x02 || hand_over == 0x03) && size >= 2) {
        uint16_t nwid = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << UString::Format(u"Network id: %d (0x%X)", {nwid, nwid}) << std::endl;
    }

    if (origin == 0x00 && size >= 2) {
        uint16_t org_servid = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << UString::Format(u"Original service id: %d (0x%X)", {org_servid, org_servid}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for System Software Update (ETSI TS 102 006)
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateSSU(TablesDisplay& display, const uint8_t*& data, size_t& size, int indent, uint8_t ltype)
{
    if (size < 1) {
        return;
    }

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    uint8_t dlength = data[0];
    data += 1; size -= 1;
    if (dlength > size) {
        dlength = uint8_t(size);
    }

    while (dlength >= 4) {
        uint32_t oui = GetUInt24(data);
        uint8_t slength = data[3];
        data += 4; size -= 4; dlength -= 4;
        const uint8_t* sdata = data;
        if (slength > dlength) {
            slength = dlength;
        }
        data += slength; size -= slength; dlength -= slength;
        strm << margin << "OUI: " << names::OUI(oui, names::FIRST) << std::endl;
        display.displayPrivateData(u"Selector data", sdata, slength, indent);
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for TS with System Software Update
// BAT or NIT (ETSI TS 102 006)
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateTableSSU(TablesDisplay& display, const uint8_t*& data, size_t& size, int indent, uint8_t ltype)
{
    if (size >= 1) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        uint8_t ttype = data[0];
        data += 1; size -= 1;

        strm << margin << "SSU table type: ";
        switch (ttype) {
            case 0x01: strm << "NIT"; break;
            case 0x02: strm << "BAT"; break;
            default:   strm << UString::Hexa(ttype); break;
        }
        strm << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for INT.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateINT(TablesDisplay& display, const uint8_t*& data, size_t& size, int indent, uint8_t ltype)
{
    if (size < 1) {
        return;
    }

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    uint8_t data_length = data[0];
    data += 1; size -= 1;
    if (data_length > size) {
        data_length = uint8_t(size);
    }

    while (data_length >= 4) {

        uint32_t plf_id = GetUInt24(data);
        uint8_t loop_length = data[3];
        data += 4; size -= 4; data_length -= 4;
        if (loop_length > data_length) {
            loop_length = data_length;
        }

        strm << margin << UString::Format(u"- Platform id: %s", {ts::names::PlatformId(plf_id, names::HEXA_FIRST)}) << std::endl;

        while (loop_length >= 4) {
            const UString lang(DeserializeLanguageCode(data));
            uint8_t name_length = data[3];
            data += 4; size -= 4;  data_length -= 4; loop_length -= 4;
            if (name_length > loop_length) {
                name_length = loop_length;
            }

            const UString name(duck.decoded(data, name_length));
            data += name_length; size -= name_length; data_length -= name_length; loop_length -= name_length;

            strm << margin << "  Language: " << lang << ", name: \"" << name << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Display linkage private data for deferred INT.
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::DisplayPrivateDeferredINT(TablesDisplay& display, const uint8_t*& data, size_t& size, int indent, uint8_t ltype)
{
    if (size >= 1) {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        uint8_t ttype = data[0];
        data += 1; size -= 1;

        strm << margin << "INT linkage table type: ";
        switch (ttype) {
            case 0x00: strm << "unspecified"; break;
            case 0x01: strm << "NIT"; break;
            case 0x02: strm << "BAT"; break;
            default:   strm << UString::Hexa(ttype); break;
        }
        strm << std::endl;

        if (ttype == 0x02 && size >= 2) {
            const uint16_t bid = GetUInt16(data);
            data += 2; size -= 2;
            strm << margin << UString::Format(u"Bouquet id: 0x%X (%d)", {bid, bid}) << std::endl;
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
        for (ExtendedEventLinkageList::const_iterator it = extended_event_linkage_info.begin(); it != extended_event_linkage_info.end(); ++it) {
            xml::Element* e = extInfo->addElement(u"event");
            e->setIntAttribute(u"target_event_id", it->target_event_id, true);
            e->setBoolAttribute(u"target_listed", it->target_listed);
            e->setBoolAttribute(u"event_simulcast", it->event_simulcast);
            e->setIntAttribute(u"link_type", it->link_type, true);
            e->setIntAttribute(u"target_id_type", it->target_id_type, true);
            if (it->target_id_type == 3) {
                e->setIntAttribute(u"user_defined_id", it->user_defined_id, true);
            }
            if (it->target_id_type == 1) {
                e->setIntAttribute(u"target_transport_stream_id", it->target_transport_stream_id, true);
            }
            if (it->target_original_network_id.set()) {
                e->setIntAttribute(u"target_original_network_id", it->target_original_network_id.value(), true);
            }
            if (it->target_service_id.set()) {
                e->setIntAttribute(u"target_service_id", it->target_service_id.value(), true);
            }
        }
    }

    if (!private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute<uint16_t>(ts_id, u"transport_stream_id", true) &&
        element->getIntAttribute<uint16_t>(onetw_id, u"original_network_id", true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true) &&
        element->getIntAttribute<uint8_t>(linkage_type, u"linkage_type", true) &&
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
        ok = mobileElements[0]->getIntAttribute<uint8_t>(mobile_handover_info.handover_type, u"handover_type", true, 0, 0, 0x0F) &&
             mobileElements[0]->getIntEnumAttribute(mobile_handover_info.origin_type, OriginTypeNames, u"origin_type", true) &&
             mobileElements[0]->getIntAttribute<uint16_t>(mobile_handover_info.network_id, u"network_id",
                                                          mobile_handover_info.handover_type >= 1 && mobile_handover_info.handover_type <= 3) &&
             mobileElements[0]->getIntAttribute<uint16_t>(mobile_handover_info.initial_service_id, u"initial_service_id",
                                                          mobile_handover_info.origin_type == 0x00);
    }

    if (ok && !eventElements.empty()) {
        ok = eventElements[0]->getIntAttribute<uint16_t>(event_linkage_info.target_event_id, u"target_event_id", true) &&
             eventElements[0]->getBoolAttribute(event_linkage_info.target_listed, u"target_listed", true) &&
             eventElements[0]->getBoolAttribute(event_linkage_info.event_simulcast, u"event_simulcast", true);
    }

    if (ok && !extEventElements.empty()) {
        ok = extEventElements[0]->getChildren(eventElements, u"event");
        for (size_t i = 0; ok && i < eventElements.size(); ++i) {
            ExtendedEventLinkageInfo info;
            ok = eventElements[i]->getIntAttribute<uint16_t>(info.target_event_id, u"target_event_id", true) &&
                 eventElements[i]->getBoolAttribute(info.target_listed, u"target_listed", true) &&
                 eventElements[i]->getBoolAttribute(info.event_simulcast, u"event_simulcast", true) &&
                 eventElements[i]->getIntAttribute<uint8_t>(info.link_type, u"link_type", true, 0, 0, 3) &&
                 eventElements[i]->getIntAttribute<uint8_t>(info.target_id_type, u"target_id_type", true, 0, 0, 3) &&
                 eventElements[i]->getIntAttribute<uint16_t>(info.user_defined_id, u"user_defined_id", info.target_id_type == 3) &&
                 eventElements[i]->getIntAttribute<uint16_t>(info.target_transport_stream_id, u"target_transport_stream_id", info.target_id_type == 1) &&
                 eventElements[i]->getOptionalIntAttribute<uint16_t>(info.target_original_network_id, u"target_original_network_id") &&
                 eventElements[i]->getOptionalIntAttribute<uint16_t>(info.target_service_id, u"target_service_id");
            extended_event_linkage_info.push_back(info);
        }
    }
    return ok;
}
