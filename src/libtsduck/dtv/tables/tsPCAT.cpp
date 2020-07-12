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

#include "tsPCAT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
#include "tsMJD.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"PCAT"
#define MY_CLASS ts::PCAT
#define MY_TID ts::TID_PCAT
#define MY_PID ts::PID_PCAT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::PCAT::PCAT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    service_id(0),
    transport_stream_id(0),
    original_network_id(0),
    content_id(0),
    versions(this)
{
}

ts::PCAT::PCAT(const PCAT& other) :
    AbstractLongTable(other),
    service_id(other.service_id),
    transport_stream_id(other.transport_stream_id),
    original_network_id(other.original_network_id),
    content_id(other.content_id),
    versions(this, other.versions)
{
}

ts::PCAT::PCAT(DuckContext& duck, const BinaryTable& table) :
    PCAT()
{
    deserialize(duck, table);
}

ts::PCAT::ContentVersion::ContentVersion(const AbstractTable* table) :
    EntryWithDescriptors(table),
    content_version(0),
    content_minor_version(0),
    version_indicator(0),
    schedules()
{
}

ts::PCAT::Schedule::Schedule() :
    start_time(),
    duration(0)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::PCAT::tableIdExtension() const
{
    return service_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::PCAT::clearContent()
{
    service_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    content_id = 0;
    versions.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PCAT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    versions.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table or payload too short.
        if (sect.tableId() != _table_id || remain < 9) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        service_id = sect.tableIdExtension();
        transport_stream_id = GetUInt16(data);
        original_network_id = GetUInt16(data + 2);
        content_id = GetUInt32(data + 4);
        size_t version_count = data[8];
        data += 9; remain -= 9;

        // Loop across all content versions.
        while (version_count > 0 && remain >= 8) {

            // Get fixed part.
            ContentVersion& cv(versions.newEntry());
            cv.content_version = GetUInt16(data);
            cv.content_minor_version = GetUInt16(data + 2);
            cv.version_indicator = (data[4] >> 6) & 0x03;
            const size_t content_length = GetUInt16(data + 4) & 0x0FFF;
            size_t schedule_length = GetUInt16(data + 6) & 0x0FFF;
            data += 8; remain -= 8;

            // [Warning #1] Here, ARIB STD-B10 is ambiguous. It says "content_descriptor_length: This 12-bit
            // field gives the total length in bytes of the following schedule loop and descriptor loop."
            // Question: Does this include the 2-byte schedule_description_length field?
            // We assume here that the 2-byte field is included but this can be wrong.
            if (content_length < 2 || remain + 2 < content_length || schedule_length + 2 > content_length || schedule_length % 8 != 0) {
                return;
            }
            const size_t descriptor_length = content_length - schedule_length - 2;

            // Get schedule loop.
            while (schedule_length >= 8) {
                assert(remain >= 8);
                Schedule sched;
                // [Warning #2] Here, ARIB STD-B10 is ambiguous again. It says "duration: A 24-bit field
                // indicates the duration of the partial contents announcement by hours, min- utes, and seconds."
                // It does not say if this is binary or BCD. We assume here the same format as in EIT, ie. BCD.
                DecodeMJD(data, 5, sched.start_time);
                const int hour = DecodeBCD(data[5]);
                const int min = DecodeBCD(data[6]);
                const int sec = DecodeBCD(data[7]);
                sched.duration = (hour * 3600) + (min * 60) + sec;
                cv.schedules.push_back(sched);
                data += 8; remain -= 8; schedule_length -= 8;
            }

            // Get descriptor loop.
            assert(remain >= descriptor_length);
            cv.descs.add(data, descriptor_length);
            data += descriptor_length; remain -= descriptor_length;
            version_count--;
        }

        // Abort in case of extra data in section.
        if (remain != 0) {
            return;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PCAT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;

    // Add fixed part (9 bytes). Will remain unmodified in all sections.
    PutUInt16(payload, transport_stream_id);
    PutUInt16(payload + 2, original_network_id);
    PutUInt32(payload + 4, content_id);
    payload[8] = 0; // number of content versions.

    uint8_t* data = payload + 9;
    size_t remain = sizeof(payload) - 9;

    // Add all content versions.
    for (auto it1 = versions.begin(); it1 != versions.end(); ++it1) {

        // If we cannot at least add the fixed part of a description, open a new section
        if (remain < 9) {
            addSection(table, section_number, payload, data, remain);
        }

        // If we are not at the beginning of the content version loop, make sure that the entire
        // content version fits in the section. If it does not fit, start a new section. Huge
        // content versions may not fit into one section, even when starting at the beginning
        // of the description loop. In that case, the description will span two sections later.
        const ContentVersion& cv(it1->second);
        const DescriptorList& dlist(cv.descs);
        const size_t version_length = 8 + cv.schedules.size() * 8 + dlist.binarySize();
        if (data > payload + 9 && version_length > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
        }

        // Increment number of content versions.
        payload[8]++;

        // Fill fixed part of the content version.
        uint8_t* cv_base = serializeContentVersionFixedPart(cv, data, remain);

        // Fill schedule loop.
        for (auto it2 = cv.schedules.begin(); it2 != cv.schedules.end(); ++it2) {
            if (remain < 8) {
                // No room for this schedule, start a new section.
                addSection(table, section_number, payload, data, remain);
                payload[8] = 1; // first content version of new section.
                cv_base = serializeContentVersionFixedPart(cv, data, remain);
            }

            // Serialize the schedule.
            // See [Warning #2] above.
            assert(remain >= 8);
            EncodeMJD(it2->start_time, data, 5);
            data[5] = EncodeBCD(int(it2->duration / 3600));
            data[6] = EncodeBCD(int((it2->duration / 60) % 60));
            data[7] = EncodeBCD(int(it2->duration % 60));
            data += 8; remain -= 8;

            // Add 8 bytes in content_descriptor_length and schedule_description_length.
            // Assume no overflow on 12 LSB.
            PutUInt16(cv_base + 4, GetUInt16(cv_base + 4) + 8);
            PutUInt16(cv_base + 6, GetUInt16(cv_base + 6) + 8);
        }

        // Serialize the descriptor loop.
        for (size_t start_index = 0; ; ) {

            // Insert descriptors (all or some).
            uint8_t* const dloop_base = data;
            start_index = dlist.serialize(data, remain, start_index);
            const size_t dloop_length = data - dloop_base;

            // Add descriptor loop length in content_descriptor_length. Assume no overflow on 12 LSB.
            PutUInt16(cv_base + 4, uint16_t(GetUInt16(cv_base + 4) + dloop_length));

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this content version.
            addSection(table, section_number, payload, data, remain);
            payload[8] = 1; // first content version of new section.
            cv_base = serializeContentVersionFixedPart(cv, data, remain);
        }

    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::PCAT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 service_id,              // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers after fixed part of payload (9 bytes).
    remain += data - payload - 9;
    data = payload + 9;
    payload[8] = 0; // reset number of content versions
    section_number++;
}


//----------------------------------------------------------------------------
// Serialize the fixed part of a ContentVersion.
// Update data and remain, return the base address of the content version entry.
//----------------------------------------------------------------------------

uint8_t* ts::PCAT::serializeContentVersionFixedPart(const ContentVersion& cv, uint8_t*& data, size_t& remain) const
{
    assert(remain >= 8);
    uint8_t* const base = data;
    PutUInt16(data, cv.content_version);
    PutUInt16(data + 2, cv.content_minor_version);
    // See [Warning #1] above. Initial content_descriptor_length is 2.
    // Initial schedule_description_length is 0.
    data[4] = uint8_t(cv.version_indicator << 6) | 0x30;
    data[5] = 0x02;
    data[6] = 0xF0;
    data[7] = 0x00;
    data += 8; remain -= 8;
    return base;
}


//----------------------------------------------------------------------------
// A static method to display a PCAT section.
//----------------------------------------------------------------------------

void ts::PCAT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Service id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 9) {

        // Display common information
        strm << margin << UString::Format(u"Transport stream id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << UString::Format(u"Original network id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
             << margin << UString::Format(u"Content id: 0x%X (%d)", {GetUInt32(data + 4), GetUInt32(data + 4)}) << std::endl;
        size_t version_count = data[8];
        data += 9; size -= 9;

        // Loop across all content versions.
        while (version_count > 0 && size >= 8) {

            // Display fixed part.
            strm << margin << UString::Format(u"- Content version: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << UString::Format(u"  Content minor version: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
                 << margin << "  Version indicator: " << NameFromSection(u"PCATVersionIndicator", (data[4] >> 6) & 0x03, names::DECIMAL_FIRST) << std::endl;

            // See [Warning #1] above.
            const size_t content_length = std::max<size_t>(2, std::min<size_t>(GetUInt16(data + 4) & 0x0FFF, size - 6));
            size_t schedule_length = std::min<size_t>(GetUInt16(data + 6) & 0x0FFF, content_length - 2);
            const size_t descriptor_length = content_length - schedule_length - 2;
            data += 8; size -= 8;

            // Display schedule loop.
            while (schedule_length >= 8) {
                assert(size >= 8);
                // See [Warning #2] above.
                Time start;
                DecodeMJD(data, 5, start);
                const int hour = DecodeBCD(data[5]);
                const int min = DecodeBCD(data[6]);
                const int sec = DecodeBCD(data[7]);
                strm << margin
                     << UString::Format(u"  Schedule start: %s, duration: %02d:%02d:%02d", {start.format(Time::DATE | Time::TIME), hour, min, sec})
                     << std::endl;
                data += 8; size -= 8; schedule_length -= 8;
            }

            // Display descriptor loop.
            assert(size >= descriptor_length);
            display.displayDescriptorList(section, data, descriptor_length, indent + 2);
            data += descriptor_length; size -= descriptor_length;
            version_count--;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PCAT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setIntAttribute(u"content_id", content_id, true);

    for (auto it1 = versions.begin(); it1 != versions.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"version");
        e1->setIntAttribute(u"content_version", it1->second.content_version, true);
        e1->setIntAttribute(u"content_minor_version", it1->second.content_minor_version, true);
        e1->setIntAttribute(u"version_indicator", it1->second.version_indicator, false);
        for (auto it2 = it1->second.schedules.begin(); it2 != it1->second.schedules.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"schedule");
            e2->setDateTimeAttribute(u"start_time", it2->start_time);
            e2->setTimeAttribute(u"duration", it2->duration);
        }
        it1->second.descs.toXML(duck, e1);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PCAT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xversion;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true) &&
        element->getIntAttribute<uint16_t>(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute<uint16_t>(original_network_id, u"original_network_id", true) &&
        element->getIntAttribute<uint32_t>(content_id, u"content_id", true) &&
        element->getChildren(xversion, u"version");

    for (auto it1 = xversion.begin(); ok && it1 != xversion.end(); ++it1) {
        ContentVersion& cv(versions.newEntry());
        xml::ElementVector xschedule;
        ok = (*it1)->getIntAttribute<uint16_t>(cv.content_version, u"content_version", true) &&
             (*it1)->getIntAttribute<uint16_t>(cv.content_minor_version, u"content_minor_version", true) &&
             (*it1)->getIntAttribute<uint8_t>(cv.version_indicator, u"version_indicator", true, 0, 0, 3) &&
             cv.descs.fromXML(duck, xschedule, *it1, u"schedule");
        for (auto it2 = xschedule.begin(); ok && it2 != xschedule.end(); ++it2) {
            Schedule sched;
            ok = (*it2)->getDateTimeAttribute(sched.start_time, u"start_time", true) &&
                 (*it2)->getTimeAttribute(sched.duration, u"duration", true);
            if (ok) {
                cv.schedules.push_back(sched);
            }
        }
    }
    return ok;
}
