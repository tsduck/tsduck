//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSDTT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SDTT"
#define MY_CLASS ts::SDTT
#define MY_TID ts::TID_SDTT
#define MY_PID ts::PID_SDTT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::SDTT::SDTT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    contents(this)
{
}

ts::SDTT::SDTT(const SDTT& other) :
    AbstractLongTable(other),
    table_id_ext(other.table_id_ext),
    transport_stream_id(other.transport_stream_id),
    original_network_id(other.original_network_id),
    service_id(other.service_id),
    contents(this, other.contents)
{
}

ts::SDTT::SDTT(DuckContext& duck, const BinaryTable& table) :
    SDTT()
{
    deserialize(duck, table);
}

ts::SDTT::Content::Content(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::SDTT::tableIdExtension() const
{
    return table_id_ext;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::SDTT::clearContent()
{
    table_id_ext = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    service_id = 0;
    contents.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SDTT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    table_id_ext = section.tableIdExtension();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    service_id = buf.getUInt16();
    const size_t num_of_contents = buf.getUInt8();

    size_t content_description_length = 0;
    size_t schedule_description_length = 0;

    // Loop across all download content.
    for (size_t index = 0; index < num_of_contents; ++index) {
        Content& cnt(contents.newEntry());
        buf.getBits(cnt.group, 4);
        buf.getBits(cnt.target_version, 12);
        buf.getBits(cnt.new_version, 12);
        buf.getBits(cnt.download_level, 2);
        buf.getBits(cnt.version_indicator, 2);
        buf.getBits(content_description_length, 12);
        buf.skipBits(1);  // ignore maker_id_flag, redundant with table_id_ext
        buf.skipReservedBits(3);
        buf.getBits(schedule_description_length, 12);
        buf.getBits(cnt.schedule_timeshift_information, 4);
        buf.pushReadSize(buf.currentReadByteOffset() + content_description_length);  // level 1
        buf.pushReadSize(buf.currentReadByteOffset() + schedule_description_length); // level 2
        while (buf.canReadBytes(8)) {
            cnt.schedules.emplace_back();
            cnt.schedules.back().start_time = buf.getFullMJD();
            buf.getSecondsBCD(cnt.schedules.back().duration);
        }
        buf.popState(); // level 2
        buf.getDescriptorList(cnt.descs);
        buf.popState(); // level 1
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SDTT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUInt16(service_id);

    // Will write num_of_contents here. Initially zero.
    const size_t num_of_contents_pos = buf.currentWriteByteOffset();
    uint8_t num_of_contents = 0;
    buf.putUInt8(num_of_contents);

    // Restart here at each new section.
    buf.pushState();

    // Add all download contents.
    for (const auto& it : contents) {
        const Content& cnt(it.second);

        // Total required binary size of this entry.
        const size_t entry_size = 8 + 8 * cnt.schedules.size() + cnt.descs.binarySize();

        // If we are not at the beginning of the download content loop, make sure that the entire
        // content fits in the section. If it does not fit, start a new section. In the case we
        // have an entry which is too large, even in first position, it will be truncated.
        if (entry_size > buf.remainingWriteBytes() && num_of_contents > 0) {
            // Create a new section.
            addOneSection(table, buf);
            // Reset number of contents in buffer for next section.
            num_of_contents = 0;
            buf.pushState();
            buf.writeSeek(num_of_contents_pos);
            buf.putUInt8(num_of_contents);
            buf.popState();
        }

        // Serialize the download content entry. If the schedule loop or descriptor loop is too long, it is truncated.
        buf.putBits(cnt.group, 4);
        buf.putBits(cnt.target_version, 12);
        buf.putBits(cnt.new_version, 12);
        buf.putBits(cnt.download_level, 2);
        buf.putBits(cnt.version_indicator, 2);

        // Will overwrite 32 bits here later: content_description_length / maker_id_flag / reserved / schedule_description_length / schedule_timeshift_information
        const size_t len_pos = buf.currentWriteByteOffset();
        buf.putUInt32(0);

        // Serialize as many schedules as possible.
        const size_t sched_pos = buf.currentWriteByteOffset();
        for (auto it2 = cnt.schedules.begin(); buf.canWriteBytes(8) && it2 != cnt.schedules.end(); ++it2) {
            buf.putFullMJD(it2->start_time);
            buf.putSecondsBCD(it2->duration);
        }
        const size_t sched_size = buf.currentWriteByteOffset() - sched_pos;

        // Serialize as many descriptors as possible.
        buf.putPartialDescriptorList(cnt.descs);
        const size_t content_size = buf.currentWriteByteOffset() - sched_pos;

        // Now adjust content_description_length / maker_id_flag / reserved / schedule_description_length / schedule_timeshift_information
        buf.pushState();
        buf.writeSeek(len_pos);
        buf.putBits(content_size, 12);
        buf.putBit(!hasExtendedMakerId());
        buf.putReserved(3);
        buf.putBits(sched_size, 12);
        buf.putBits(cnt.schedule_timeshift_information, 4);

        // Adjust number of contents in this section.
        buf.writeSeek(num_of_contents_pos);
        buf.putUInt8(++num_of_contents);
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// A static method to display a SDTT section.
//----------------------------------------------------------------------------

void ts::SDTT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    const uint16_t tidext = section.tableIdExtension();
    const bool extended_maker_id = tidext >= 0xE000 && tidext <= 0xEFFF;
    disp << margin << UString::Format(u"Table extension id: %n", tidext) << std::endl;

    if (buf.canReadBytes(7)) {
        size_t content_description_length = 0;
        size_t schedule_description_length = 0;

        disp << margin << UString::Format(u"Transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Service id: %n", buf.getUInt16()) << std::endl;
        const size_t num_of_contents = buf.getUInt8();
        disp << margin << "Number of download contents: " << num_of_contents << std::endl;

        // Loop across all download content.
        for (size_t index = 0; buf.canReadBytes(8) && index < num_of_contents; ++index) {
            disp << margin << "- Download content #" << index << ":" << std::endl;
            disp << margin << "  Group: " << buf.getBits<int>(4);
            disp << ", target version: " << buf.getBits<int>(12);
            disp << ", new version: " << buf.getBits<int>(12) << std::endl;
            disp << margin << "  Download level: " << DataName(MY_XML_NAME, u"download_level", buf.getBits<uint8_t>(2), NamesFlags::DEC_VALUE_NAME) << std::endl;
            disp << margin << "  Version indicator: " << DataName(MY_XML_NAME, u"version_indicator", buf.getBits<uint8_t>(2), NamesFlags::DEC_VALUE_NAME) << std::endl;
            buf.getBits(content_description_length, 12);
            const bool maker_id_flag = buf.getBool();
            const bool valid = (maker_id_flag && !extended_maker_id) || (!maker_id_flag && extended_maker_id);
            disp << margin << "  Maker id flag: " << UString::TrueFalse(maker_id_flag) << (valid ? " (valid)" : " (invalid)") << std::endl;
            buf.skipReservedBits(3);
            buf.getBits(schedule_description_length, 12);
            disp << margin << "  Schedule timeshift: " << DataName(MY_XML_NAME, u"schedule_timeshift_information", buf.getBits<uint8_t>(4), NamesFlags::DEC_VALUE_NAME) << std::endl;
            buf.pushReadSize(buf.currentReadByteOffset() + content_description_length);  // level 1
            buf.pushReadSize(buf.currentReadByteOffset() + schedule_description_length); // level 2
            for (size_t i = 0; buf.canReadBytes(8); ++i) {
                disp << margin << "  Schedule #" << i << ": start: " << buf.getFullMJD().format(Time::DATETIME);
                disp << UString::Format(u", duration: %02d", buf.getBCD<int>(2));
                disp << UString::Format(u":%02d", buf.getBCD<int>(2));
                disp << UString::Format(u":%02d", buf.getBCD<int>(2)) << std::endl;
            }
            buf.popState(); // level 2
            disp.displayDescriptorList(section, context, false, buf, margin + u"  ");
            buf.popState(); // level 1
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SDTT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"table_id_ext", table_id_ext, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"service_id", service_id, true);

    for (const auto& it1 : contents) {
        xml::Element* e1 = root->addElement(u"content");
        e1->setIntAttribute(u"group", it1.second.group);
        e1->setIntAttribute(u"target_version", it1.second.target_version);
        e1->setIntAttribute(u"new_version", it1.second.new_version);
        e1->setIntAttribute(u"download_level", it1.second.download_level);
        e1->setIntAttribute(u"version_indicator", it1.second.version_indicator);
        e1->setIntAttribute(u"schedule_timeshift_information", it1.second.schedule_timeshift_information);
        for (const auto& it2 : it1.second.schedules) {
            xml::Element* e2 = e1->addElement(u"schedule");
            e2->setDateTimeAttribute(u"start_time", it2.start_time);
            e2->setTimeAttribute(u"duration", it2.duration);
        }
        it1.second.descs.toXML(duck, e1);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SDTT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcontent;
    bool ok =
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getIntAttribute(table_id_ext, u"table_id_ext", true) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getIntAttribute(service_id, u"service_id", true) &&
        element->getChildren(xcontent, u"content");

    for (auto it1 = xcontent.begin(); ok && it1 != xcontent.end(); ++it1) {
        xml::ElementVector xsched;
        Content& cnt(contents.newEntry());
        ok = (*it1)->getIntAttribute(cnt.group, u"group", true, 0, 0, 0x0F) &&
             (*it1)->getIntAttribute(cnt.target_version, u"target_version", true, 0, 0, 0x0FFF) &&
             (*it1)->getIntAttribute(cnt.new_version, u"new_version", true, 0, 0, 0x0FFF) &&
             (*it1)->getIntAttribute(cnt.download_level, u"download_level", true, 0, 0, 0x03) &&
             (*it1)->getIntAttribute(cnt.version_indicator, u"version_indicator", true, 0, 0, 0x03) &&
             (*it1)->getIntAttribute(cnt.schedule_timeshift_information, u"schedule_timeshift_information", true, 0, 0, 0x0F) &&
             cnt.descs.fromXML(duck, xsched, *it1, u"schedule");
        for (auto it2 = xsched.begin(); ok && it2 != xsched.end(); ++it2) {
            cnt.schedules.emplace_back();
            ok = (*it2)->getDateTimeAttribute(cnt.schedules.back().start_time, u"start_time", true) &&
                 (*it2)->getTimeAttribute(cnt.schedules.back().duration, u"duration", true);
        }
    }
    return ok;
}
