//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLTST.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"LTST"
#define MY_CLASS ts::LTST
#define MY_TID ts::TID_LTST
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LTST::LTST(uint8_t version_, uint16_t table_id_extension_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // LTST is always "current"
    table_id_extension(table_id_extension_),
    sources(this)
{
}

ts::LTST::LTST(DuckContext& duck, const BinaryTable& table) :
    LTST()
{
    deserialize(duck, table);
}


ts::LTST::LTST(const LTST& other) :
    AbstractLongTable(other),
    table_id_extension(other.table_id_extension),
    protocol_version(other.protocol_version),
    sources(this, other.sources)
{
}

ts::LTST::Source::Source(const AbstractTable* table, const Source& other) :
    source_id(other.source_id),
    data(table, other.data)
{
}

ts::LTST::Source::Source(const AbstractTable* table, Source&& other) :
    source_id(other.source_id),
    data(table, other.data)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::LTST::tableIdExtension() const
{
    return table_id_extension;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::LTST::clearContent()
{
    table_id_extension = 0;
    protocol_version = 0;
    sources.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LTST::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    table_id_extension = section.tableIdExtension();
    protocol_version = buf.getUInt8();
    size_t num_source_id_in_section = buf.getUInt8();

    // Get data sources descriptions.
    while (!buf.error() && num_source_id_in_section-- > 0) {
        Source& src(sources.newEntry());
        src.source_id = buf.getUInt16();
        size_t num_data_events = buf.getUInt8();

        // Get data events descriptions.
        while (!buf.error() && num_data_events-- > 0) {
            Data& event(src.data.newEntry());
            buf.skipReservedBits(2);
            buf.getBits(event.data_id, 14);
            event.start_time = Time::GPSSecondsToUTC(cn::seconds(buf.getUInt32()));
            buf.skipReservedBits(2);
            buf.getBits(event.ETM_location, 2);
            buf.getBits(event.length_in_seconds, 20);
            buf.getMultipleStringWithLength(event.title_text);
            buf.getDescriptorListWithLength(event.descs);
        }
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LTST::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt8(protocol_version);

    // Save position before num_source_id_in_section. Will be updated at each event.
    uint8_t num_source_id_in_section = 0;
    buf.pushState();
    buf.putUInt8(num_source_id_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Loop on data sources.
    for (auto src_it = sources.begin(); !buf.error() && src_it != sources.end(); ++src_it) {
        const Source& src(src_it->second);

        // We don't know the total size of the serialized data source and we don't know if it will fit in
        // the current section. So, we serialize the complete data source into one specific buffer first.
        // Then, we will know if we can copy it in the current section or if we must create a new one.
        PSIBuffer srcbuf(buf.duck(), MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE - 2);

        // Serialize the complete data source in srcbuf.
        srcbuf.putUInt16(src.source_id);
        srcbuf.putUInt8(uint8_t(src.data.size()));
        for (const auto& event : src.data) {
            srcbuf.putReserved(2);
            srcbuf.putBits(event.second.data_id, 14);
            srcbuf.putUInt32(uint32_t(event.second.start_time.toGPSSeconds().count()));
            srcbuf.putReserved(2);
            srcbuf.putBits(event.second.ETM_location, 2);
            srcbuf.putBits(event.second.length_in_seconds, 20);
            srcbuf.putMultipleStringWithLength(event.second.title_text);
            srcbuf.putDescriptorListWithLength(event.second.descs);
        }
        const size_t src_size = srcbuf.currentWriteByteOffset();


        // If we are not at the beginning of the data source loop, make sure that the entire
        // data source fits in the section. If it does not fit, start a new section.
        if (src_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of num_source_id_in_section in the new section.
            buf.putUInt8(num_source_id_in_section = 0);
        }

        // Copy the serialized data source definition.
        buf.putBytes(srcbuf.currentReadAddress(), src_size);

        // Now increment the field num_source_id_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++num_source_id_in_section);
        buf.popState();
        buf.swapState();
    }
}


//----------------------------------------------------------------------------
// A static method to display a LTST section.
//----------------------------------------------------------------------------

void ts::LTST::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Table id extension: %n", section.tableIdExtension()) << std::endl;

    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Protocol version: %d", buf.getUInt8());
        size_t num_source_id_in_section  = buf.getUInt8();
        disp << UString::Format(u", number of data sources: %d", num_source_id_in_section) << std::endl;

        // Loop on data sources.
        while (buf.canReadBytes(3) && num_source_id_in_section-- > 0) {
            disp << margin << UString::Format(u"- Source Id: %n", buf.getUInt16());
            size_t num_data_events  = buf.getUInt8();
            disp << UString::Format(u", number of data events: %d", num_data_events) << std::endl;

            // Loop on all event definitions.
            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
            while (buf.canReadBytes(8) && num_data_events-- > 0) {
                buf.skipReservedBits(2);
                disp << margin << UString::Format(u"  - Data Id: %n", buf.getBits<uint16_t>(14)) << std::endl;
                disp << margin << "    Start UTC: " << Time::GPSSecondsToUTC(cn::seconds(buf.getUInt32())).format(Time::DATETIME) << std::endl;
                buf.skipReservedBits(2);
                disp << margin << UString::Format(u"    ETM location: %d", buf.getBits<uint8_t>(2)) << std::endl;
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

void ts::LTST::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"table_id_extension", table_id_extension, true);
    root->setIntAttribute(u"protocol_version", protocol_version);

    for (const auto& src : sources) {
        xml::Element* xsrc = root->addElement(u"source");
        xsrc->setIntAttribute(u"source_id", src.second.source_id, true);
        for (const auto& it : src.second.data) {
            xml::Element* e = xsrc->addElement(u"data");
            e->setIntAttribute(u"data_id", it.second.data_id, true);
            e->setDateTimeAttribute(u"start_time", it.second.start_time);
            e->setIntAttribute(u"ETM_location", it.second.ETM_location, true);
            e->setChronoAttribute(u"length_in_seconds", it.second.length_in_seconds, false);
            it.second.title_text.toXML(duck, e, u"title_text", true);
            it.second.descs.toXML(duck, e);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LTST::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xsources;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(table_id_extension, u"table_id_extension", true) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getChildren(xsources, u"source");

    for (const auto& xsrc : xsources) {
        Source& src(sources.newEntry());
        xml::ElementVector xdata;
        ok = xsrc->getIntAttribute(src.source_id, u"source_id", true) &&
             xsrc->getChildren(xdata, u"data") &&
             ok;
        for (const auto& xd : xdata) {
            Data& event(src.data.newEntry());
            xml::ElementVector xtitle;
            ok = xd->getIntAttribute(event.data_id, u"data_id", true, 0, 0, 0x3FFF) &&
                 xd->getDateTimeAttribute(event.start_time, u"start_time", true) &&
                 xd->getIntAttribute(event.ETM_location, u"ETM_location", true, 0, 0, 3) &&
                 xd->getChronoAttribute(event.length_in_seconds, u"length_in_seconds", true, cn::seconds::zero(), cn::seconds::zero(), cn::seconds(0x000FFFFF)) &&
                 event.descs.fromXML(duck, xtitle, xd, u"title_text") &&
                 (xtitle.empty() || event.title_text.fromXML(duck, xtitle[0])) &&
                 ok;
        }
    }
    return ok;
}
