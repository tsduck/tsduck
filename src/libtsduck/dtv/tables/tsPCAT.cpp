//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPCAT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    EntryWithDescriptors(table)
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

void ts::PCAT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    service_id = section.tableIdExtension();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    content_id = buf.getUInt32();

    // Loop on content version entries.
    for (size_t version_count = buf.getUInt8(); !buf.error() && version_count > 0; version_count--) {

        ContentVersion& cv(versions.newEntry());
        cv.content_version = buf.getUInt16();
        cv.content_minor_version = buf.getUInt16();
        buf.getBits(cv.version_indicator, 2);
        buf.skipBits(2);

        // [Warning #1] Here, ARIB STD-B10 is ambiguous. It says "content_descriptor_length: This 12-bit
        // field gives the total length in bytes of the following schedule loop and descriptor loop."
        // Question: Does this include the following 2-byte schedule_description_length field?
        // We assume here that this 2-byte field is included but this can be wrong.
        buf.pushReadSizeFromLength(12);

        // Start the schedule_description_length sequence.
        buf.skipBits(4);
        buf.pushReadSizeFromLength(12);

        // Get schedule loop.
        while (buf.canRead()) {
            Schedule sched;
            // [Warning #2] Here, ARIB STD-B10 is ambiguous again. It says "duration: A 24-bit field
            // indicates the duration of the partial contents announcement by hours, minutes, and seconds."
            // It does not say if this is binary or BCD. We assume here the same format as in EIT, ie. BCD.
            sched.start_time = buf.getFullMJD();
            sched.duration = buf.getSecondsBCD();
            cv.schedules.push_back(sched);
        }

        // Close the schedule_description_length sequence.
        buf.popState();

        // Get descriptor loop.
        buf.getDescriptorList(cv.descs);

        // Close the content_descriptor_length sequence.
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PCAT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.putUInt32(content_id);

    // Save position before num_of_content_version. Will be updated at each version.
    // This position will also be restored after each call to addOneSection().
    uint8_t num_of_content_version = 0;
    buf.pushState();
    buf.putUInt8(num_of_content_version);

    // Minimum size of the payload (after fixed size).
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all content versions.
    for (const auto& it1 : versions) {
        const ContentVersion& cv(it1.second);

        // Binary size of the channel definition.
        const size_t entry_size = 8 + 8 * cv.schedules.size() + cv.descs.binarySize();

        // If we are not at the beginning of the content loop, make sure that the entire
        // entry fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of num_of_content_version in the new section.
            buf.putUInt8(num_of_content_version = 0);
        }

        // Fill fixed part of the content version.
        buf.putUInt16(cv.content_version);
        buf.putUInt16(cv.content_minor_version);
        buf.putBits(cv.version_indicator, 2);
        buf.putBits(0xFF, 2);

        // Start the content_descriptor_length sequence. See [Warning #1] above.
        buf.pushWriteSequenceWithLeadingLength(12);

        // Start the schedule_description_length sequence.
        buf.putBits(0xFF, 4);
        buf.pushWriteSequenceWithLeadingLength(12);

        // Fill schedule loop.
        for (const auto& it2 : cv.schedules) {
            // Serialize the schedule. See [Warning #2] above.
            buf.putFullMJD(it2.start_time);
            buf.putSecondsBCD(it2.duration);
        }

        // Close the schedule_description_length sequence.
        buf.popState();

        // Add descriptor loop. Must fit completely in the section.
        buf.putDescriptorList(cv.descs);

        // Close the content_descriptor_length sequence.
        buf.popState();

        // Now increment the field num_of_content_version at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++num_of_content_version);
        buf.popState();
        buf.swapState();
    }
}


//----------------------------------------------------------------------------
// A static method to display a PCAT section.
//----------------------------------------------------------------------------

void ts::PCAT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Service id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(9)) {
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Content id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;

        // Loop across all content versions.
        for (size_t version_count = buf.getUInt8(); buf.canReadBytes(8) && version_count > 0; version_count--) {
            disp << margin << UString::Format(u"- Content version: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Content minor version: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << "  Version indicator: " << DataName(MY_XML_NAME, u"VersionIndicator", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
            buf.skipBits(2);

            // Start the content_descriptor_length sequence. See [Warning #1] above.
            buf.pushReadSizeFromLength(12);

            // Start the schedule_description_length sequence.
            buf.skipBits(4);
            buf.pushReadSizeFromLength(12);

            // Display schedule loop.
            while (buf.canReadBytes(8)) {
                // See [Warning #2] above.
                disp << margin << "  Schedule start: " << buf.getFullMJD().format(Time::DATETIME);
                disp << UString::Format(u", duration: %02d", {buf.getBCD<int>(2)});
                disp << UString::Format(u":%02d", {buf.getBCD<int>(2)});
                disp << UString::Format(u":%02d", {buf.getBCD<int>(2)}) << std::endl;
            }

            // Close the schedule_description_length sequence.
            disp.displayPrivateData(u"Extraneous schedule bytes", buf);
            buf.popState();

            // Display descriptor loop.
            disp.displayDescriptorList(section, buf, margin + u"  ");

            // Close the content_descriptor_length sequence.
            disp.displayPrivateData(u"Extraneous version content bytes", buf);
            buf.popState();
        }
    }
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

    for (const auto& it1 : versions) {
        xml::Element* e1 = root->addElement(u"version");
        e1->setIntAttribute(u"content_version", it1.second.content_version, true);
        e1->setIntAttribute(u"content_minor_version", it1.second.content_minor_version, true);
        e1->setIntAttribute(u"version_indicator", it1.second.version_indicator, false);
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

bool ts::PCAT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xversion;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(service_id, u"service_id", true) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getIntAttribute(content_id, u"content_id", true) &&
        element->getChildren(xversion, u"version");

    for (auto it1 = xversion.begin(); ok && it1 != xversion.end(); ++it1) {
        ContentVersion& cv(versions.newEntry());
        xml::ElementVector xschedule;
        ok = (*it1)->getIntAttribute(cv.content_version, u"content_version", true) &&
             (*it1)->getIntAttribute(cv.content_minor_version, u"content_minor_version", true) &&
             (*it1)->getIntAttribute(cv.version_indicator, u"version_indicator", true, 0, 0, 3) &&
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
