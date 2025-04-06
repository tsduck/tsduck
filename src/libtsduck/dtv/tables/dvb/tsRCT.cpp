//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRCT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RCT"
#define MY_CLASS ts::RCT
#define MY_TID ts::TID_RCT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RCT::RCT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_)
{
}

ts::RCT::RCT(const RCT& other) :
    AbstractLongTable(other),
    service_id(other.service_id),
    year_offset(other.year_offset),
    links(this, other.links),
    descs(this, other.descs)
{
}

ts::RCT::RCT(DuckContext& duck, const BinaryTable& table) :
    RCT()
{
    deserialize(duck, table);
}

ts::RCT::Link::Link(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::RCT::tableIdExtension() const
{
    return service_id;
}


//----------------------------------------------------------------------------
// This table has a "top-level descriptor list".
//----------------------------------------------------------------------------

ts::DescriptorList* ts::RCT::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::RCT::topLevelDescriptorList() const
{
    return &descs;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::RCT::clearContent()
{
    service_id = 0;
    year_offset = 0;
    links.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    service_id = section.tableIdExtension();
    year_offset = buf.getUInt16();

    // Get link descriptions.
    const size_t link_count = buf.getUInt8();
    for (size_t i = 0; i < link_count; ++i) {
        buf.skipReservedBits(4);
        buf.pushReadSizeFromLength(12);
        Link& link(links.newEntry());
        link.deserializePayload(buf);
        if (buf.remainingReadBits() > 0) {
            buf.setUserError();
        }
        buf.popState();
    }

    // Get main descriptor loop.
    buf.skipReservedBits(4);
    buf.getDescriptorListWithLength(descs, 12);
}

void ts::RCT::DVBBinaryLocator::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(identifier_type, 2);
    scheduled_time_reliability = buf.getBool();
    inline_service = buf.getBool();
    buf.skipReservedBits(1);
    buf.getBits(start_date, 9);
    if (inline_service) {
        buf.skipReservedBits(2);
        transport_stream_id = buf.getUInt16();
        original_network_id = buf.getUInt16();
        service_id = buf.getUInt16();
    }
    else {
        buf.getBits(dvb_service_triplet_id, 10);
    }
    start_time = buf.getUInt16();
    duration = buf.getUInt16();
    switch (identifier_type) {
        case 1:
            event_id = buf.getUInt16();
            break;
        case 2:
            TVA_id = buf.getUInt16();
            break;
        case 3:
            TVA_id = buf.getUInt16();
            component_tag = buf.getUInt8();
            break;
        case 0:
            if (scheduled_time_reliability) {
                buf.getBits(early_start_window, 3);
                buf.getBits(late_end_window, 5);
            }
            break;
        default:
            break;
    }
}

void ts::RCT::PromotionalText::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getStringWithByteLength(text);
}

void ts::RCT::Link::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(link_type, 4);
    buf.skipReservedBits(2);
    buf.getBits(how_related_classification_scheme_id, 6);
    buf.getBits(term_id, 12);
    buf.getBits(group_id, 4);
    buf.getBits(precedence, 4);
    if (link_type == 0 || link_type == 2) {
        buf.getStringWithByteLength(media_uri);
    }
    if (link_type == 1 || link_type == 2) {
        dvb_binary_locator.deserializePayload(buf);
    }
    buf.skipReservedBits(2);
    const size_t number_items = buf.getBits<size_t>(6);
    for (size_t i = 0; i < number_items; ++i) {
        PromotionalText text;
        text.deserializePayload(buf);
        promotional_texts.push_back(std::move(text));
    }
    default_icon_flag = buf.getBool();
    buf.getBits(icon_id, 3);
    buf.getDescriptorListWithLength(descs, 12);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(year_offset);

    // Will write link_count here. Initially zero.
    const size_t link_count_pos = buf.currentWriteByteOffset();
    uint8_t link_count = 0;
    buf.putUInt8(link_count);

    // Restart here at each new section.
    buf.pushState();

    // Add all download contents.
    for (const auto& it : links) {
        const Link& link(it.second);

        // Try to serialize this link in this section. If we overflow because we approach
        // the end of section, we will revert at this point.
        buf.pushState();

        buf.putReserved(4);
        buf.pushWriteSequenceWithLeadingLength(12);
        link.serializePayload(buf);
        buf.popState(); // write length

        if (buf.error()) {
            // Write error, probable overflow, revert.
            buf.clearError();
            buf.popState();
            // Close the section, open a new one.
            addOneSection(table, buf);
            // Reset number of links in buffer for next section.
            link_count = 0;
            buf.pushState();
            buf.writeSeek(link_count_pos);
            buf.putUInt8(link_count);
            buf.popState();
            // Now reserialize the link in a new section.
            buf.putReserved(4);
            buf.pushWriteSequenceWithLeadingLength(12);
            link.serializePayload(buf);
            buf.popState(); // write length
            // If there is still an error, this is an invalid section.
            if (buf.error()) {
                return;
            }
        }
        else {
            // No error, the link was serialized, drop the save state.
            buf.dropState();

            // Adjust number of links in this section.
            buf.pushState();
            buf.writeSeek(link_count_pos);
            buf.putUInt8(++link_count);
            buf.popState();
        }
    }

    // Insert top level descriptor list (with leading length field).
    // Add new section when the descriptor list overflows.
    for (size_t start = 0;;) {
        start = buf.putPartialDescriptorListWithLength(descs, start);
        if (buf.error() || start >= descs.size()) {
            break;
        }
        else {
            addOneSection(table, buf);
            // Reset number of links in buffer for next section.
            link_count = 0;
            buf.pushState();
            buf.writeSeek(link_count_pos);
            buf.putUInt8(link_count);
            buf.popState();
        }
    }
}

void ts::RCT::DVBBinaryLocator::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(identifier_type, 2);
    buf.putBit(scheduled_time_reliability);
    buf.putBit(inline_service);
    buf.putReserved(1);
    buf.putBits(start_date, 9);
    if (inline_service) {
        buf.putReserved(2);
        buf.putUInt16(transport_stream_id);
        buf.putUInt16(original_network_id);
        buf.putUInt16(service_id);
    }
    else {
        buf.putBits(dvb_service_triplet_id, 10);
    }
    buf.putUInt16(start_time);
    buf.putUInt16(duration);
    switch (identifier_type) {
        case 1:
            buf.putUInt16(event_id);
            break;
        case 2:
            buf.putUInt16(TVA_id);
            break;
        case 3:
            buf.putUInt16(TVA_id);
            buf.putUInt8(component_tag);
            break;
        case 0:
            if (scheduled_time_reliability) {
                buf.putBits(early_start_window, 3);
                buf.putBits(late_end_window, 5);
            }
            break;
        default:
            break;
    }
}

void ts::RCT::PromotionalText::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putStringWithByteLength(text);
}

void ts::RCT::Link::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(link_type, 4);
    buf.putReserved(2);
    buf.putBits(how_related_classification_scheme_id, 6);
    buf.putBits(term_id, 12);
    buf.putBits(group_id, 4);
    buf.putBits(precedence, 4);
    if (link_type == 0 || link_type == 2) {
        buf.putStringWithByteLength(media_uri);
    }
    if (link_type == 1 || link_type == 2) {
        dvb_binary_locator.serializePayload(buf);
    }
    buf.putReserved(2);
    buf.putBits(promotional_texts.size(), 6);
    for (const auto& text : promotional_texts) {
        text.serializePayload(buf);
    }
    buf.putBit(default_icon_flag);
    buf.putBits(icon_id, 3);
    buf.putDescriptorListWithLength(descs);
}


//----------------------------------------------------------------------------
// A static method to display a RCT section.
//----------------------------------------------------------------------------

void ts::RCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    disp << margin << UString::Format(u"Service id: %n", section.tableIdExtension()) << std::endl;

    if (buf.canReadBytes(3)) {
        const uint16_t year_offset = buf.getUInt16();
        disp << margin << "Year offset: " << year_offset << std::endl;
        const size_t link_count = buf.getUInt8();
        bool ok = buf.canReadBytes(2);
        for (size_t i = 0; ok && i < link_count; ++i) {
            buf.skipReservedBits(4);
            buf.pushReadSizeFromLength(12);
            disp << margin << "- Link #" << i << std::endl;
            ok = Link::Display(disp, section, context, buf, margin + u"  ", year_offset);
            buf.popState();
            ok = buf.canReadBytes(2) && ok;
        }
        if (ok) {
            disp.displayDescriptorListWithLength(section, context, true, buf, margin);
        }
    }
}

bool ts::RCT::DVBBinaryLocator::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, uint16_t year_offset)
{
    const Time start(year_offset, 1, 1, 0, 0);
    bool ok = buf.canReadBytes(2);
    if (ok) {
        const uint8_t identifier_type = buf.getBits<uint8_t>(2);
        const bool scheduled_time_reliability = buf.getBool();
        const bool inline_service = buf.getBool();
        buf.skipReservedBits(1);
        const uint16_t start_date = buf.getBits<uint16_t>(9);
        disp << margin << "Identifier type: " << DataName(MY_XML_NAME, u"dvb_identifier_type", identifier_type, NamesFlags::VALUE_NAME) << std::endl
             << margin << UString::Format(u"Scheduled time reliability: %s, inline service: %s", scheduled_time_reliability, inline_service) << std::endl
             << margin << "Start date: " << start_date << " (" << (start + cn::days(start_date)).format(Time::DATE) << ")" << std::endl;
        if (inline_service) {
            ok = buf.canReadBits(50);
            if (ok) {
                buf.skipReservedBits(2);
                disp << margin << UString::Format(u"Transport stream id: %n", buf.getUInt16()) << std::endl;
                disp << margin << UString::Format(u"Original network id: %n", buf.getUInt16()) << std::endl;
                disp << margin << UString::Format(u"Service id: %n", buf.getUInt16()) << std::endl;
            }
        }
        else {
            ok = buf.canReadBits(50);
            if (ok) {
                disp << margin << "DVB service triplet id: " << buf.getBits<uint16_t>(10) << std::endl;
            }
        }
        ok = ok && buf.canReadBytes(4);
        if (ok) {
            const int start_time = buf.getUInt16();
            const int duration = buf.getUInt16();
            disp << margin << "Start time: " << start_time << " x 2s (" << (start + cn::seconds(2 * start_time)).format(Time::TIME) << ")" << std::endl;
            disp << margin << "Duration: " << duration << " x 2s (" << (start + cn::seconds(2 * duration)).format(Time::TIME) << ")" << std::endl;

        }
        switch (identifier_type) {
            case 1:
                ok = buf.canReadBytes(2);
                if (ok) {
                    disp << margin << UString::Format(u"Event id: %n", buf.getUInt16()) << std::endl;
                }
                break;
            case 2:
                ok = buf.canReadBytes(2);
                if (ok) {
                    disp << margin << UString::Format(u"TVA id: %n", buf.getUInt16()) << std::endl;
                }
                break;
            case 3:
                ok = buf.canReadBytes(3);
                if (ok) {
                    disp << margin << UString::Format(u"TVA id: %n", buf.getUInt16()) << std::endl;
                    disp << margin << UString::Format(u"Component tag: %n", buf.getUInt8()) << std::endl;
                }
                break;
            case 0:
                if (scheduled_time_reliability) {
                    disp << margin << UString::Format(u"Early start window: %d", buf.getBits<uint8_t>(3));
                    disp << UString::Format(u", late end window: %d", buf.getBits<uint8_t>(5)) << std::endl;
                }
                break;
            default:
                break;
        }
    }
    return ok;
}

bool ts::RCT::PromotionalText::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(4);
    if (ok) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
    return ok;
}

bool ts::RCT::Link::Display(TablesDisplay& disp, const ts::Section& section, DescriptorContext& context, PSIBuffer& buf, const UString& margin, uint16_t year_offset)
{
    bool ok = buf.canReadBytes(5);
    if (ok) {
        const uint8_t link_type = buf.getBits<uint8_t>(4);
        buf.skipReservedBits(2);
        disp << margin << "Link type: " << DataName(MY_XML_NAME, u"link_type", link_type, NamesFlags::VALUE_NAME) << std::endl;
        disp << margin << "Related classification: " << DataName(MY_XML_NAME, u"how_related_classification_scheme_id", buf.getBits<uint8_t>(6), NamesFlags::VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Term id: %n", buf.getBits<uint16_t>(12));
        disp << UString::Format(u", group id: %n", buf.getBits<uint16_t>(4)) << std::endl;
        disp << margin << "Precedence: " << buf.getBits<uint16_t>(4) << std::endl;
        if (link_type == 0 || link_type == 2) {
            disp << margin << "Media URI: " << buf.getStringWithByteLength() << std::endl;
        }
        if (link_type == 1 || link_type == 2) {
            disp << margin << "DVB binary locator:" << std::endl;
            ok = DVBBinaryLocator::Display(disp, buf, margin + u"  ", year_offset);
        }
        buf.skipReservedBits(2);
        const size_t number_items = buf.getBits<size_t>(6);
        for (size_t i = 0; ok && i < number_items; ++i) {
            disp << margin << "Promotional text #" << i << ":" << std::endl;
            ok = PromotionalText::Display(disp, buf, margin + u"  ");
        }
        ok = ok && buf.canReadBytes(2);
        if (ok) {
            disp << margin << "Default icon flag: " << buf.getBool();
            disp << ", icon id: " << buf.getBits<uint16_t>(3) << std::endl;
            disp.displayDescriptorListWithLength(section, context, false, buf, margin);
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"year_offset", year_offset);
    for (const auto& link : links) {
        link.second.buildXML(duck, root);
    }
    descs.toXML(duck, root);
}

void ts::RCT::DVBBinaryLocator::buildXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"dvb_binary_locator");
    e->setIntAttribute(u"identifier_type", identifier_type);
    e->setBoolAttribute(u"scheduled_time_reliability", scheduled_time_reliability);
    e->setBoolAttribute(u"inline_service", inline_service);
    e->setIntAttribute(u"start_date", start_date);
    if (inline_service) {
        e->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", original_network_id, true);
        e->setIntAttribute(u"service_id", service_id, true);
    }
    else {
        e->setIntAttribute(u"dvb_service_triplet_id", dvb_service_triplet_id);
    }
    e->setIntAttribute(u"start_time", start_time);
    e->setIntAttribute(u"duration", duration);
    switch (identifier_type) {
        case 1:
            e->setIntAttribute(u"event_id", event_id, true);
            break;
        case 2:
            e->setIntAttribute(u"TVA_id", TVA_id, true);
            break;
        case 3:
            e->setIntAttribute(u"TVA_id", TVA_id, true);
            e->setIntAttribute(u"component_tag", component_tag, true);
            break;
        case 0:
            if (scheduled_time_reliability) {
                e->setIntAttribute(u"early_start_window", early_start_window);
                e->setIntAttribute(u"late_end_window", late_end_window);
            }
            break;
        default:
            break;
    }
}

void ts::RCT::PromotionalText::buildXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"promotional_text");
    e->setAttribute(u"language_code", language_code);
    e->setAttribute(u"text", text);
}

void ts::RCT::Link::buildXML(DuckContext& duck, xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"link");
    e->setIntAttribute(u"link_type", link_type);
    e->setIntAttribute(u"how_related_classification_scheme_id", how_related_classification_scheme_id);
    e->setIntAttribute(u"term_id", term_id, true);
    e->setIntAttribute(u"group_id", group_id, true);
    e->setIntAttribute(u"precedence", precedence);
    if (link_type == 0 || link_type == 2) {
        e->setAttribute(u"media_uri", media_uri);
    }
    e->setBoolAttribute(u"default_icon_flag", default_icon_flag);
    e->setIntAttribute(u"icon_id", icon_id);
    if (link_type == 1 || link_type == 2) {
        dvb_binary_locator.buildXML(duck, e);
    }
    for (const auto& text : promotional_texts) {
        text.buildXML(duck, e);
    }
    descs.toXML(duck, e);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xlink;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getBoolAttribute(_is_current, u"current", false, true) &&
              element->getIntAttribute(service_id, u"service_id", true) &&
              element->getIntAttribute(year_offset, u"year_offset", true) &&
              descs.fromXML(duck, xlink, element, u"link");
    for (auto e : xlink) {
        Link& link(links.newEntry());
        ok = link.analyzeXML(duck, e) && ok;
    }
    return ok;
}

bool ts::RCT::DVBBinaryLocator::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(identifier_type, u"identifier_type", true, 0, 0, 0x03) &&
           element->getBoolAttribute(scheduled_time_reliability, u"scheduled_time_reliability", true) &&
           element->getBoolAttribute(inline_service, u"inline_service", true) &&
           element->getIntAttribute(start_date, u"start_date", true, 0, 0, 0x01FF) &&
           element->getIntAttribute(dvb_service_triplet_id, u"dvb_service_triplet_id", !inline_service, 0, 0, 0x03FF) &&
           element->getIntAttribute(transport_stream_id, u"transport_stream_id", inline_service) &&
           element->getIntAttribute(original_network_id, u"original_network_id", inline_service) &&
           element->getIntAttribute(service_id, u"service_id", inline_service) &&
           element->getIntAttribute(start_time, u"start_time", true) &&
           element->getIntAttribute(duration, u"duration", true) &&
           element->getIntAttribute(event_id, u"event_id", identifier_type == 1) &&
           element->getIntAttribute(TVA_id, u"TVA_id", identifier_type == 2 || identifier_type == 3) &&
           element->getIntAttribute(component_tag, u"component_tag", identifier_type == 3) &&
           element->getIntAttribute(early_start_window, u"early_start_window", identifier_type == 0 && scheduled_time_reliability, 0, 0, 0x07) &&
           element->getIntAttribute(late_end_window, u"late_end_window", identifier_type == 0 && scheduled_time_reliability, 0, 0, 0x1F);
}


bool ts::RCT::PromotionalText::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
           element->getAttribute(text, u"text", true);
}


bool ts::RCT::Link::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtext, xdvb, xother;
    bool ok = element->getIntAttribute(link_type, u"link_type", true, 0, 0, 0x0F) &&
              element->getIntAttribute(how_related_classification_scheme_id, u"how_related_classification_scheme_id", true, 0, 0, 0x3F) &&
              element->getIntAttribute(term_id, u"term_id", true, 0, 0, 0x0FFF) &&
              element->getIntAttribute(group_id, u"group_id", true, 0, 0, 0x0F) &&
              element->getIntAttribute(precedence, u"precedence", true, 0, 0, 0x0F) &&
              element->getAttribute(media_uri, u"media_uri", link_type == 0 || link_type == 2) &&
              element->getBoolAttribute(default_icon_flag, u"default_icon_flag", true) &&
              element->getIntAttribute(icon_id, u"icon_id", true, 0, 0, 0x07) &&
              element->getChildren(xdvb, u"dvb_binary_locator", link_type == 1 || link_type == 2 ? 1 : 0, 1) &&
              element->getChildren(xtext, u"promotional_text") &&
              descs.fromXML(duck, xother, element, u"dvb_binary_locator,promotional_text");

    if (ok && (link_type == 1 || link_type == 2) && !xdvb.empty()) {
        ok = dvb_binary_locator.analyzeXML(duck, xdvb[0]);
    }
    for (auto e : xtext) {
        PromotionalText text;
        ok = text.analyzeXML(duck, e) && ok;
        promotional_texts.push_back(std::move(text));
    }
    return ok;
}
