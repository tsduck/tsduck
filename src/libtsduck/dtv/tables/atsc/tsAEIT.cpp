//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAEIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AEIT"
#define MY_CLASS ts::AEIT
#define MY_TID ts::TID_AEIT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AEIT::AEIT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // AEIT is always "current"
    sources(this)
{
}

ts::AEIT::AEIT(DuckContext& duck, const BinaryTable& table) :
    AEIT()
{
    deserialize(duck, table);
}


ts::AEIT::AEIT(const AEIT& other) :
    AbstractLongTable(other),
    AEIT_subtype(other.AEIT_subtype),
    MGT_tag(other.MGT_tag),
    sources(this, other.sources)
{
}

ts::AEIT::Source::Source(const AbstractTable* table, const Source& other) :
    source_id(other.source_id),
    events(table, other.events)
{
}

ts::AEIT::Source::Source(const AbstractTable* table, Source&& other) :
    source_id(other.source_id),
    events(table, other.events)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::AEIT::tableIdExtension() const
{
    return uint16_t(uint16_t(AEIT_subtype) << 8) | MGT_tag;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::AEIT::clearContent()
{
    AEIT_subtype = 0;
    MGT_tag = 0;
    sources.clear();
    reserved.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AEIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    AEIT_subtype = uint8_t(section.tableIdExtension() >> 8);
    MGT_tag = uint8_t(section.tableIdExtension());
    if (AEIT_subtype == 0) {
        size_t num_sources_in_section = buf.getUInt8();

        // Get event sources descriptions.
        while (!buf.error() && num_sources_in_section-- > 0) {
            Source& src(sources.newEntry());
            src.source_id = buf.getUInt16();
            size_t num_events = buf.getUInt8();

            // Get events descriptions.
            while (!buf.error() && num_events-- > 0) {
                Event& event(src.events.newEntry());
                event.off_air = buf.getBool();
                buf.skipReservedBits(1);
                buf.getBits(event.event_id, 14);
                event.start_time = Time::GPSSecondsToUTC(cn::seconds(buf.getUInt32()));
                buf.skipReservedBits(4);
                buf.getBits(event.duration, 20);
                buf.getMultipleStringWithLength(event.title_text);
                buf.getDescriptorListWithLength(event.descs);
            }
        }
    }
    else {
        buf.getBytes(reserved);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AEIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    if (AEIT_subtype == 0) {
        // Save position before num_sources_in_section. Will be updated at each event.
        uint8_t num_sources_in_section = 0;
        buf.pushState();
        buf.putUInt8(num_sources_in_section);
        const size_t payload_min_size = buf.currentWriteByteOffset();

        // Loop on event sources.
        for (auto src_it = sources.begin(); !buf.error() && src_it != sources.end(); ++src_it) {
            const Source& src(src_it->second);

            // We don't know the total size of the serialized event source and we don't know if it will fit in
            // the current section. So, we serialize the complete event source into one specific buffer first.
            // Then, we will know if we can copy it in the current section or if we must create a new one.
            PSIBuffer srcbuf(buf.duck(), MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE - 1);

            // Serialize the complete event source in srcbuf.
            srcbuf.putUInt16(src.source_id);
            srcbuf.putUInt8(uint8_t(src.events.size()));
            for (const auto& event : src.events) {
                srcbuf.putBit(event.second.off_air);
                srcbuf.putReserved(1);
                srcbuf.putBits(event.second.event_id, 14);
                srcbuf.putUInt32(uint32_t(event.second.start_time.toGPSSeconds().count()));
                srcbuf.putReserved(4);
                srcbuf.putBits(event.second.duration, 20);
                srcbuf.putMultipleStringWithLength(event.second.title_text);
                srcbuf.putDescriptorListWithLength(event.second.descs);
            }
            const size_t src_size = srcbuf.currentWriteByteOffset();

            // If we are not at the beginning of the event source loop, make sure that the entire
            // event source fits in the section. If it does not fit, start a new section.
            if (src_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
                // Create a new section.
                addOneSection(table, buf);
                // We are at the position of num_sources_in_section in the new section.
                buf.putUInt8(num_sources_in_section = 0);
            }

            // Copy the serialized data source definition.
            buf.putBytes(srcbuf.currentReadAddress(), src_size);

            // Now increment the field num_sources_in_section at saved position.
            buf.swapState();
            buf.pushState();
            buf.putUInt8(++num_sources_in_section);
            buf.popState();
            buf.swapState();
        }
    }
    else {
        // AEIT_subtype != 0: Assume only one section in that case.
        buf.putBytes(reserved);
    }
}


//----------------------------------------------------------------------------
// A static method to display a AEIT section.
//----------------------------------------------------------------------------

void ts::AEIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    const uint8_t AEIT_subtype = uint8_t(section.tableIdExtension() >> 8);
    disp << margin << UString::Format(u"AEIT subtype: %n, MGT tag: %n", AEIT_subtype, uint8_t(section.tableIdExtension())) << std::endl;
    if (AEIT_subtype != 0) {
        disp.displayPrivateData(u"Reserved", buf, NPOS, margin);
    }
    else if (buf.canReadBytes(1)) {
        size_t num_sources_in_section = buf.getUInt8();
        disp << margin << "Number of event sources: " << num_sources_in_section << std::endl;

        // Loop on data sources.
        while (buf.canReadBytes(3) && num_sources_in_section-- > 0) {
            disp << margin << UString::Format(u"- Source Id: %n", buf.getUInt16());
            size_t num_events = buf.getUInt8();
            disp << UString::Format(u", number of events: %d", num_events) << std::endl;

            // Loop on all event definitions.
            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
            while (buf.canReadBytes(10) && num_events-- > 0) {
                const bool off_air = buf.getBool();
                buf.skipReservedBits(1);
                disp << margin << UString::Format(u"  - Event Id: %n, off air: %s", buf.getBits<uint16_t>(14), off_air) << std::endl;
                disp << margin << "    Start UTC: " << Time::GPSSecondsToUTC(cn::seconds(buf.getUInt32())).format(Time::DATETIME) << std::endl;
                buf.skipReservedBits(4);
                disp << margin << UString::Format(u"    Duration: %d seconds", buf.getBits<uint32_t>(20)) << std::endl;
                disp.displayATSCMultipleString(buf, 1, margin + u"    ", u"Title text: ");
                disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"    ");
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AEIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"AEIT_subtype", AEIT_subtype, true);
    root->setIntAttribute(u"MGT_tag", MGT_tag, true);
    if (AEIT_subtype != 0) {
        root->addHexaTextChild(u"reserved", reserved, true);
    }
    else {
        for (const auto& src : sources) {
            xml::Element* xsrc = root->addElement(u"source");
            xsrc->setIntAttribute(u"source_id", src.second.source_id, true);
            for (const auto& it : src.second.events) {
                xml::Element* e = xsrc->addElement(u"event");
                e->setBoolAttribute(u"off_air", it.second.off_air);
                e->setIntAttribute(u"event_id", it.second.event_id, true);
                e->setDateTimeAttribute(u"start_time", it.second.start_time);
                e->setChronoAttribute(u"duration", it.second.duration, false);
                it.second.title_text.toXML(duck, e, u"title_text", true);
                it.second.descs.toXML(duck, e);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AEIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xsources;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(AEIT_subtype, u"AEIT_subtype", false, 0) &&
        element->getIntAttribute(MGT_tag, u"MGT_tag", true) &&
        element->getChildren(xsources, u"source", 0, AEIT_subtype != 0 ? 0 : xml::UNLIMITED) &&
        (AEIT_subtype == 0 || element->getHexaTextChild(reserved, u"reserved"));                                                                                                                                                                                                (AEIT_subtype == 0 || element->getHexaTextChild(reserved, u"reserved"));

    for (const auto& xsrc : xsources) {
        Source& src(sources.newEntry());
        xml::ElementVector xevent;
        ok = xsrc->getIntAttribute(src.source_id, u"source_id", true) &&
             xsrc->getChildren(xevent, u"event") &&
             ok;
        for (const auto& xd : xevent) {
            Event& event(src.events.newEntry());
            xml::ElementVector xtitle;
            ok = xd->getBoolAttribute(event.off_air, u"off_air", true) &&
                 xd->getIntAttribute(event.event_id, u"event_id", true, 0, 0, 0x3FFF) &&
                 xd->getDateTimeAttribute(event.start_time, u"start_time", true) &&
                 xd->getChronoAttribute(event.duration, u"duration", true, cn::seconds::zero(), cn::seconds::zero(), cn::seconds(0x000FFFFF)) &&
                 event.descs.fromXML(duck, xtitle, xd, u"title_text") &&
                 (xtitle.empty() || event.title_text.fromXML(duck, xtitle[0])) &&
                 ok;
        }
    }
    return ok;
}
