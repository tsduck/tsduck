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

#include "tsEIT.h"
#include "tsAlgorithm.h"
#include "tsNames.h"
#include "tsRST.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"EIT"
#define MY_CLASS ts::EIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, ts::Range<ts::TID>(ts::TID_EIT_MIN, ts::TID_EIT_MAX),
                  MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr,
                  // DVB has only one standard PID for EIT, ISDB adds two others.
                  {ts::PID_EIT, ts::PID_ISDB_EIT_2, ts::PID_ISDB_EIT_3});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EIT::EIT(bool is_actual_,
             bool is_pf_,
             uint8_t eits_index_,
             uint8_t version_,
             bool is_current_,
             uint16_t service_id_,
             uint16_t ts_id_,
             uint16_t onetw_id_) :
    AbstractLongTable(ComputeTableId(is_actual_, is_pf_, eits_index_), MY_XML_NAME, MY_STD, version_, is_current_),
    service_id(service_id_),
    ts_id(ts_id_),
    onetw_id(onetw_id_),
    last_table_id(_table_id),
    events(this)
{
}

ts::EIT::EIT(DuckContext& duck, const BinaryTable& table) :
    AbstractLongTable(TID_EIT_PF_ACT, MY_XML_NAME, MY_STD, 0, true),  // TID will be updated by deserialize()
    service_id(0),
    ts_id(0),
    onetw_id(0),
    last_table_id(0),
    events(this)
{
    deserialize(duck, table);
}


ts::EIT::EIT(const EIT& other) :
    AbstractLongTable(other),
    service_id(other.service_id),
    ts_id(other.ts_id),
    onetw_id(other.onetw_id),
    last_table_id(other.last_table_id),
    events(this, other.events)
{
}

ts::EIT::Event::Event(const AbstractTable* table) :
    EntryWithDescriptors(table),
    event_id(0),
    start_time(),
    duration(0),
    running_status(0),
    CA_controlled(false)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::EIT::tableIdExtension() const
{
    return service_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::EIT::clearContent()
{
    service_id = 0;
    ts_id = 0;
    onetw_id = 0;
    last_table_id = 0;
    events.clear();
}


//----------------------------------------------------------------------------
// Comparison operator for events according to their start time.
//----------------------------------------------------------------------------

bool ts::EIT::Event::operator<(const Event& other) const
{
    return this->start_time < other.start_time;
}

bool ts::EIT::LessEventPtr(const Event* ev1, const Event* ev2)
{
    return ev1 != nullptr && ev2 != nullptr && *ev1 < *ev2;
}

bool ts::EIT::BinaryEvent::operator<(const BinaryEvent& other) const
{
    return this->start_time < other.start_time;
}

bool ts::EIT::LessBinaryEventPtr(const BinaryEventPtr& ev1, const BinaryEventPtr& ev2)
{
    return !ev1.isNull() && !ev2.isNull() && *ev1 < *ev2;
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::EIT::isValidTableId(TID tid) const
{
    return IsEIT(tid);
}


//----------------------------------------------------------------------------
// Compute an EIT table id.
//----------------------------------------------------------------------------

ts::TID ts::EIT::ComputeTableId(bool is_actual, bool is_pf, uint8_t eits_index)
{
    if (is_pf) {
        return is_actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
    }
    else {
        return (is_actual ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN) + (eits_index & 0x0F);
    }
}

ts::TID ts::EIT::SegmentToTableId(bool is_actual, size_t segment)
{
    // Each table id has 32 segments (SEGMENTS_PER_TABLE).
    return TID((is_actual ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN) + (std::min(segment, SEGMENTS_COUNT - 1) / SEGMENTS_PER_TABLE));
}


//----------------------------------------------------------------------------
// Compute the segment of an event in an EIT schedule.
//----------------------------------------------------------------------------

size_t ts::EIT::TimeToSegment(const Time& last_midnight, const Time& event_start_time)
{
    if (event_start_time < last_midnight) {
        // Should not happen, last midnight is the start time of the reference period.
        return 0;
    }
    else {
        // Each segment covers 3 hours (SEGMENT_DURATION).
        return size_t((event_start_time - last_midnight) / SEGMENT_DURATION);
    }
}


//----------------------------------------------------------------------------
// Toggle an EIT table id between Actual and Other.
//----------------------------------------------------------------------------

ts::TID ts::EIT::ToggleActual(TID tid, bool actual)
{
    if (tid == TID_EIT_PF_ACT && !actual) {
        return TID_EIT_PF_OTH;
    }
    else if (tid == TID_EIT_PF_OTH && actual) {
        return TID_EIT_PF_ACT;
    }
    else if (tid >= TID_EIT_S_ACT_MIN && tid <= TID_EIT_S_ACT_MAX && !actual) {
        return tid + 0x10;
    }
    else if (tid >= TID_EIT_S_OTH_MIN && tid <= TID_EIT_S_OTH_MAX && actual) {
        return tid - 0x10;
    }
    else {
        return tid;
    }
}


//----------------------------------------------------------------------------
// Set if this is an "actual" EIT.
//----------------------------------------------------------------------------

void ts::EIT::setActual(bool is_actual)
{
    if (isPresentFollowing()) {
        _table_id = is_actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        last_table_id = _table_id;
    }
    else if (is_actual) {
        _table_id = TID_EIT_S_ACT_MIN + (_table_id & 0x0F);
        last_table_id = TID_EIT_S_ACT_MIN + (last_table_id & 0x0F);
    }
    else {
        _table_id = TID_EIT_S_OTH_MIN + (_table_id & 0x0F);
        last_table_id = TID_EIT_S_ACT_MIN + (last_table_id & 0x0F);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    service_id = section.tableIdExtension();
    ts_id = buf.getUInt16();
    onetw_id = buf.getUInt16();
    buf.skipBytes(1); // segment_last_section_number
    last_table_id = buf.getUInt8();

    // Get events description
    while (buf.canRead()) {
        Event& event(events.newEntry());
        event.event_id = buf.getUInt16();
        event.start_time = buf.getFullMJD();
        const int hour = buf.getBCD<int>(2);
        const int min = buf.getBCD<int>(2);
        const int sec = buf.getBCD<int>(2);
        event.duration = (hour * 3600) + (min * 60) + sec;
        event.running_status = buf.getBits<uint8_t>(3);
        event.CA_controlled = buf.getBool();
        buf.getDescriptorListWithLength(event.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // In the serialize() method, we do not attempt to reorder events and
    // sections according to rules from ETSI TS 101 211. This is impossible in
    // the general case since those rules prescript to skip sessions between
    // segments, making the result an "invalid" table in the MPEG-TS sense.
    // Applications wanting to produce a conformant set of EIT sections shall
    // use the static method EIT::ReorganizeSections().

    // Fixed part, to be repeated on all sections.
    buf.putUInt16(ts_id);
    buf.putUInt16(onetw_id);
    buf.putUInt8(0); // segment_last_section_number, will be fixed later.
    buf.putUInt8(last_table_id);
    buf.pushState();

    // Minimum size of a section: fixed part.
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all events in time order.
    for (auto evit = events.begin(); evit != events.end(); ++evit) {
        const Event& ev(evit->second);

        // Binary size of the event entry.
        const size_t entry_size = EIT_EVENT_FIXED_SIZE + ev.descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, buf);
        }

        // Insert event entry.
        buf.putUInt16(ev.event_id);
        buf.putFullMJD(ev.start_time);
        buf.putBCD(ev.duration / 3600, 2);
        buf.putBCD((ev.duration / 60) % 60, 2);
        buf.putBCD(ev.duration % 60, 2);
        buf.putBits(ev.running_status, 3);
        buf.putBit(ev.CA_controlled);
        buf.putPartialDescriptorListWithLength(ev.descs);
    }

    // Add partial section (if there is one). Normally, we do not have to do this.
    // This is done automatically in the caller. However, in the specific case of
    // an EIT, we must have a complete binary table to call Fix().
    if (buf.currentWriteByteOffset() > payload_min_size || table.sectionCount() == 0) {
        addOneSection(table, buf);
    }

    // Finally, fix the segmentation values in the serialized binary table.
    Fix(table, FIX_EXISTING);
}


//----------------------------------------------------------------------------
// Static method to fix the segmentation of a binary EIT.
//----------------------------------------------------------------------------

void ts::EIT::Fix(BinaryTable& table, FixMode mode)
{
    const TID tid = table.tableId();

    // Filter non-EIT tables.
    if (tid < TID_EIT_MIN || tid > TID_EIT_MAX || table.sectionCount() == 0) {
        return;
    }

    // Common EIT fields in all sections.
    const bool is_schedule = tid >= TID_EIT_S_ACT_MIN;
    const bool is_actual = tid <= TID_EIT_S_ACT_MAX;
    const uint8_t last_section = uint8_t(table.sectionCount() - 1);
    bool is_private = true;
    bool is_current = true;

    // Last table id: same as table id for EIT p/f, max 0x5F or 0x6F for EIT schedule.
    TID last_table_id = tid;
    const TID max_table_id = is_schedule ? TID(is_actual ? TID_EIT_S_ACT_MAX : TID_EIT_S_OTH_MAX) : tid;

    // Payload of an empty section (without event).
    // The field segment_last_section_number will be updated segment by segment.
    uint8_t empty_payload[EIT_PAYLOAD_FIXED_SIZE];
    TS_ZERO(empty_payload);
    bool got_empty_payload = false;

    // Array of segment_last_section_number value by segment, with their default values.
    uint8_t segment_last_section_number[SEGMENTS_PER_TABLE];
    if (is_schedule) {
        // EIT schedule: default is first section of each segment.
        for (uint8_t i = 0; i < SEGMENTS_PER_TABLE; ++i) {
            segment_last_section_number[i] = i * SECTIONS_PER_SEGMENT;
        }
    }
    else {
        // EIT p/f: no segment, always use last section of table.
        ::memset(segment_last_section_number, last_section, SEGMENTS_PER_TABLE);
    }

    // Search meaningful content for empty payload and other parameters.
    for (size_t si = 0; si < table.sectionCount(); si++) {
        const SectionPtr& sec(table.sectionAt(si));
        if (!sec.isNull() && sec->isValid() && sec->payloadSize() >= EIT_PAYLOAD_FIXED_SIZE) {
            // Get a copy of a valid empty payload from the first valid section.
            if (!got_empty_payload) {
                ::memcpy(empty_payload, sec->payload(), EIT_PAYLOAD_FIXED_SIZE);
                got_empty_payload = true;
                is_private = sec->isPrivateSection();
                is_current = sec->isCurrent();
            }
            // Get common section fields for EIT schedule.
            if (is_schedule) {
                last_table_id = std::min(max_table_id, std::max(sec->payload()[5], last_table_id));
                // Update known last section in segment.
                const size_t seg = si / SECTIONS_PER_SEGMENT;
                const uint8_t max_section = std::min(last_section, uint8_t((seg + 1) * SECTIONS_PER_SEGMENT - 1));
                assert(seg < SEGMENTS_PER_TABLE);
                segment_last_section_number[seg] = std::min(max_section, std::max(segment_last_section_number[seg], sec->payload()[4]));
            }
        }
    }

    // Complete empty payload.
    empty_payload[5] = last_table_id;

    // Now add or fix sections.
    for (size_t si = 0; si < table.sectionCount(); si++) {

        // Section pointer.
        const SectionPtr& sec(table.sectionAt(si));
        // Segment number of this section:
        const size_t seg = si / SECTIONS_PER_SEGMENT;
        // Identified last section in this segment:
        const uint8_t seg_last = segment_last_section_number[seg];

        // Process non-existent section.
        if (sec.isNull()) {
            // Create an empty section if required.
            if (mode > FILL_SEGMENTS || si > seg_last) {
                empty_payload[4] = seg_last;
                table.addSection(new Section(tid,
                                             is_private,
                                             table.tableIdExtension(),
                                             table.version(),
                                             is_current,
                                             uint8_t(si),  // section_number
                                             last_section, // last_section_number
                                             empty_payload,
                                             EIT_PAYLOAD_FIXED_SIZE));

            }
        }
        else if (mode == FIX_EXISTING && sec->isValid() && sec->payloadSize() >= EIT_PAYLOAD_FIXED_SIZE) {
            // Patch section: update last section in segment and last table id.
            uint8_t* const pl = const_cast<uint8_t*>(sec->payload());
            if (pl[4] != seg_last || pl[5] != last_table_id) {
                pl[4] = seg_last;
                pl[5] = last_table_id;
                sec->recomputeCRC();
            }
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display an EIT section.
//----------------------------------------------------------------------------

void ts::EIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    // The time reference is UTC as defined by DVB, but JST in Japan.
    const char* const zone = (disp.duck().standards() & Standards::JAPAN) == Standards::JAPAN ? "JST" : "UTC";

    disp << margin << UString::Format(u"Service Id: %d (0x%<X)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"TS Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original Network Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Segment last section: %d (0x%<X)", {buf.getUInt8()}) << std::endl;
        const uint8_t last_tid = buf.getUInt8();
        disp << margin << UString::Format(u"Last Table Id: %d (0x%<X), %s", {last_tid, names::TID(disp.duck(), last_tid)}) << std::endl;

        while (buf.canReadBytes(12)) {
            disp << margin << UString::Format(u"- Event Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
            disp << margin << "  Start " << zone << ": " << buf.getFullMJD().format(Time::DATE | Time::TIME) << std::endl;
            disp << margin << UString::Format(u"  Duration: %02d", {buf.getBCD<int>(2)});
            disp << UString::Format(u":%02d", {buf.getBCD<int>(2)});
            disp << UString::Format(u":%02d", {buf.getBCD<int>(2)}) << std::endl;
            disp << margin << "  Running status: " << names::RunningStatus(buf.getBits<uint8_t>(3)) << std::endl;
            disp << margin << "  CA mode: " << (buf.getBool() ? "controlled" : "free") << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    if (isPresentFollowing()) {
        root->setAttribute(u"type", u"pf");
    }
    else {
        root->setIntAttribute(u"type", _table_id - (isActual() ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN));
    }
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setBoolAttribute(u"actual", isActual());
    root->setIntAttribute(u"service_id", service_id, true);
    root->setIntAttribute(u"transport_stream_id", ts_id, true);
    root->setIntAttribute(u"original_network_id", onetw_id, true);
    root->setIntAttribute(u"last_table_id", last_table_id, true);

    for (auto it = events.begin(); it != events.end(); ++it) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"event_id", it->second.event_id, true);
        e->setDateTimeAttribute(u"start_time", it->second.start_time);
        e->setTimeAttribute(u"duration", it->second.duration);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it->second.running_status);
        e->setBoolAttribute(u"CA_mode", it->second.CA_controlled);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        getTableId(element) &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(onetw_id, u"original_network_id", true, 0, 0x00, 0xFFFF) &&
        element->getIntAttribute<TID>(last_table_id, u"last_table_id", false, _table_id, 0x00, 0xFF) &&
        element->getChildren(children, u"event");

    // Get all events.
    for (size_t i = 0; ok && i < children.size(); ++i) {
        Event& event(events.newEntry());
        ok = children[i]->getIntAttribute<uint16_t>(event.event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getDateTimeAttribute(event.start_time, u"start_time", true) &&
             children[i]->getTimeAttribute(event.duration, u"duration", true) &&
             children[i]->getIntEnumAttribute<uint8_t>(event.running_status, RST::RunningStatusNames, u"running_status", false, 0) &&
             children[i]->getBoolAttribute(event.CA_controlled, u"CA_mode", false, false) &&
             event.descs.fromXML(duck, children[i]);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Get the table id from XML element.
//----------------------------------------------------------------------------

bool ts::EIT::getTableId(const xml::Element* element)
{
    UString type;
    bool actual = 0;

    if (!element->getAttribute(type, u"type", false, u"pf") || !element->getBoolAttribute(actual, u"actual", false, true)) {
        // Invalid XML.
        return false;
    }
    else if (type.similar(u"pf")) {
        // This is an EIT p/f
        _table_id = actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        return true;
    }
    else if (type.toInteger(_table_id)) {
        // This is an EIT schedule
        _table_id += actual ? TID_EIT_S_ACT_MIN : TID_EIT_S_OTH_MIN;
        return true;
    }
    else {
        element->report().error(u"'%s' is not a valid value for attribute 'type' in <%s>, line %d", {type, element->name(), element->lineNumber()});
        return false;
    }
}


//----------------------------------------------------------------------------
// An internal structure to store binary events from sections.
// Constructor based on EIT section payload.
//----------------------------------------------------------------------------

ts::EIT::BinaryEvent::BinaryEvent(TID tid, const uint8_t*& data, size_t& size) :
    actual(IsActual(tid)),
    start_time(),
    event_data()
{
    // The fixed header size of an event is 12 bytes.
    if (data != nullptr && size >= EIT_EVENT_FIXED_SIZE) {
        const size_t event_size = EIT_EVENT_FIXED_SIZE + (GetUInt16(data + EIT_EVENT_FIXED_SIZE - 2) & 0x0FFF);
        if (size >= event_size) {
            DecodeMJD(data + 2, 5, start_time);
            event_data.copy(data, event_size);
            data += event_size;
            size -= event_size;
        }
    }
}


//----------------------------------------------------------------------------
// Build an empty EIT section for a given service. Return null pointer on error.
//----------------------------------------------------------------------------

ts::SectionPtr ts::EIT::BuildEmptySection(TID tid, uint8_t section_number, const ServiceIdTriplet& serv, SectionPtrVector& sections)
{
    // Build section data.
    ByteBlockPtr section_data(new ByteBlock(LONG_SECTION_HEADER_SIZE + EIT_PAYLOAD_FIXED_SIZE + SECTION_CRC32_SIZE));
    CheckNonNull(section_data.pointer());
    uint8_t* data = section_data->data();

    // Section header
    PutUInt8(data, tid);
    PutUInt16(data + 1, 0xF000 | uint16_t(section_data->size() - 3));
    PutUInt16(data + 3, serv.service_id); // table id extension
    PutUInt8(data + 5, 0xC1 | uint8_t(serv.version << 1));
    PutUInt8(data + 6, section_number);
    PutUInt8(data + 7, section_number);  // last section number

    // EIT section payload, without event.
    PutUInt16(data + 8, serv.transport_stream_id);
    PutUInt16(data + 10, serv.original_network_id);
    PutUInt8(data + 12, section_number);  // segment last section number
    PutUInt8(data + 13, tid);  // last table id

    // Build a section from the binary data.
    const SectionPtr sec(new Section(section_data, PID_NULL, CRC32::IGNORE));

    // Insert the section in the list of them before returning it.
    sections.push_back(sec);
    return sec;
}


//----------------------------------------------------------------------------
// Extract the service id triplet from an EIT section.
//----------------------------------------------------------------------------

ts::ServiceIdTriplet ts::EIT::GetService(const SectionPtr& section)
{
    if (section->payloadSize() < EIT_PAYLOAD_FIXED_SIZE) {
        return ServiceIdTriplet();
    }
    else {
        const uint8_t* data = section->payload();
        return ServiceIdTriplet(section->tableIdExtension(), GetUInt16(data), GetUInt16(data + 2), section->version());
    }
}


//----------------------------------------------------------------------------
// Insert all events from an EIT section in a BinaryEventPtrMap.
//----------------------------------------------------------------------------

void ts::EIT::ExtractBinaryEvents(const SectionPtr& section, BinaryEventPtrMap& events)
{
    if (section->payloadSize() >= EIT_PAYLOAD_FIXED_SIZE) {
        // Section payload.
        const uint8_t* data = section->payload();
        size_t size = section->payloadSize();

        // Build the service id triplet.
        const ServiceIdTriplet servid(GetService(section));

        // Loop on all events in the EIT payload.
        data += EIT_PAYLOAD_FIXED_SIZE;
        size -= EIT_PAYLOAD_FIXED_SIZE;
        while (size >= EIT_EVENT_FIXED_SIZE) {
            // Get the next binary event.
            BinaryEventPtr ev(new BinaryEvent(section->tableId(), data, size));
            if (ev->event_data.empty()) {
                // Could not get the event, EIT payload is probably corrupted.
                break;
            }
            // Insert the binary event in the appropriate set of events.
            events[servid].push_back(ev);
        }
    }
}


//----------------------------------------------------------------------------
// Sort all events in a map, get oldest event date.
//----------------------------------------------------------------------------

void ts::EIT::SortEvents(BinaryEventPtrMap& events, Time& oldest)
{
    // Loop on all services.
    for (auto it = events.begin(); it != events.end(); ++it) {
        // Sort the events by start date.
        std::sort(it->second.begin(), it->second.end(), LessBinaryEventPtr);

        // Check if the first event (oldest) has an older date.
        if (!it->second.empty() && (oldest == Time::Epoch || it->second[0]->start_time < oldest)) {
            oldest = it->second[0]->start_time;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to reorganize EIT sections according to ETSI TS 101 211.
//----------------------------------------------------------------------------

void ts::EIT::ReorganizeSections(SectionPtrVector& sections, const Time& reftime)
{
    SectionPtrVector out_sections;   // List of output sections.
    BinaryEventPtrMap events_pf;     // Set of all events from EIT p/f.
    BinaryEventPtrMap events_sched;  // Set of all events from EIT schedule.

    // Pass 1: Analyze all input EIT sections and extract binary events.
    // Non-EIT sections are copied into the output vector of sections.
    for (size_t isec = 0; isec < sections.size(); ++isec) {
        const SectionPtr& sec(sections[isec]);
        if (!sec.isNull() && sec->isValid()) {
            if (IsEIT(sec->tableId())) {
                // This is a valid EIT section.
                ExtractBinaryEvents(sec, IsPresentFollowing(sec->tableId()) ? events_pf : events_sched);
            }
            else {
                // This is a valid non-EIT section.
                out_sections.push_back(sec);
            }
        }
    }

    // Pass 2: Sort events per service, get oldest start time.
    Time last_midnight;
    SortEvents(events_pf, last_midnight);
    SortEvents(events_sched, last_midnight);

    // Get the reference time ("last midnight").
    if (reftime != Time::Epoch) {
        last_midnight = reftime;
    }
    last_midnight = last_midnight.thisDay();

    // Pass 3: EIT p/f processing according to ETSI TS 101 211:
    // For each service, create exactly two sections: one for present and one for following event, possibly empty.
    // If more than 2 events exist for the service, keep only the last two. If only one exist, use it as present
    for (auto it = events_pf.begin(); it != events_pf.end(); ++it) {

        // Get ordered list of events for a given service. Cannot be empty.
        const BinaryEventPtrVector& events(it->second);
        const size_t evcount = events.size();
        assert(evcount > 0);

        // Build present and following sections.
        const TID tid = events[0]->actual ? TID_EIT_PF_ACT : TID_EIT_PF_OTH;
        const SectionPtr psec(BuildEmptySection(tid, 0, it->first, out_sections));
        const SectionPtr fsec(BuildEmptySection(tid, 1, it->first, out_sections));
        if (evcount == 1) {
            // Only a current event.
            psec->appendPayload(events[0]->event_data, false);
        }
        else {
            // Use last two events as present and following.
            psec->appendPayload(events[evcount - 2]->event_data, false);
            fsec->appendPayload(events[evcount - 1]->event_data, false);
        }

        // Fix last_section_number in both sections. Don't recompute CRC yet.
        psec->setLastSectionNumber(1, false);
        fsec->setLastSectionNumber(1, false);

        // Fix segment_last_section_number (offset 4 in payload). Recompute CRC now.
        psec->setUInt8(4, 1, true);
        fsec->setUInt8(4, 1, true);
    }

    // Pass 4: EIT schedule processing according to ETSI TS 101 211.
    for (auto it = events_sched.begin(); it != events_sched.end(); ++it) {

        // Get ordered list of events for a given service. Cannot be empty.
        const BinaryEventPtrVector& events(it->second);
        const size_t evcount = events.size();
        assert(evcount > 0);

        // Characteristics of the service.
        const ServiceIdTriplet service(it->first);
        const bool actual = events[0]->actual;

        // Create the section for segment 0. It can be empty, but all segments shall
        // have at least an empty section, until the last event in the service.
        size_t cur_segment = 0;
        SectionPtr cur_section(BuildEmptySection(SegmentToTableId(actual, cur_segment), SegmentToSection(cur_segment), service, out_sections));

        // Loop on all events for this service.
        for (size_t i = 0; i < events.size(); ++i)  {

            // If the event is before the reference "last midnight", it can't be scheduled and is ignored.
            if (events[i]->start_time < last_midnight) {
                continue;
            }

            // Compute the segment number of this event.
            const size_t segment = TimeToSegment(last_midnight, events[i]->start_time);

            // If we have changed segment, we need to create all intermediate segments as empty.
            while (cur_segment < segment) {
                cur_segment++;
                cur_section = BuildEmptySection(SegmentToTableId(actual, cur_segment), SegmentToSection(cur_segment), service, out_sections);
            }

            // Check if the current event can fit into the current section.
            if (cur_section->payloadSize() + events[i]->event_data.size() > MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE) {
                // Need to create another section in this segment.
                const uint8_t secnum = cur_section->sectionNumber() + 1;
                if (secnum >= SegmentToSection(cur_segment) + SECTIONS_PER_SEGMENT) {
                    // Too many events in that segment, drop this event.
                    continue;
                }
                cur_section = BuildEmptySection(SegmentToTableId(actual, cur_segment), secnum, service, out_sections);
            }

            // Now append the event to the section payload.
            cur_section->appendPayload(events[i]->event_data, false);
        }
    }

    // Pass 5: Fix synthetic fields in EIT-schedule sections: last_section_number, segment_last_section_number, last_table_id.
    // Browse through the list of sections from the end since sections are sorted and we need to fix "last" fields.
    // Although recompute CRC of all sections. Stop when a non-EIT-schedule section is found.

    uint8_t last_section_number = 0;          // last_section_number field in EIT
    uint8_t segment_last_section_number = 0;  // segment_last_section_number field in EIT
    TID last_table_id = TID_NULL;             // last_table_id field in EIT
    ServiceIdTriplet cur_service;             // current service id
    TID cur_table_id = TID_NULL;              // current table id
    bool new_service = true;                  // next iteration starts a new service
    bool new_table = true;                    // next iteration starts a new table id
    bool new_segment = true;                  // next iteration starts a new segment

    for (auto it = out_sections.rbegin(); it != out_sections.rend() && IsSchedule((*it)->tableId()); ++it) {

        const ServiceIdTriplet this_service(GetService(*it));
        const TID this_table_id = (*it)->tableId();
        const uint8_t this_section_number = (*it)->sectionNumber();

        // Update current data.
        if (new_service || cur_service != this_service) {
            cur_service = this_service;
            last_table_id = this_table_id;
            new_service = false;
            new_table = true;
        }
        if (new_table || cur_table_id != this_table_id) {
            cur_table_id = this_table_id;
            last_section_number = this_section_number;
            new_table = false;
            new_segment = true;
        }
        if (new_segment) {
            segment_last_section_number = this_section_number;
            new_segment = false;
        }
        new_segment = this_section_number % SECTIONS_PER_SEGMENT == 0;

        // Update the fields in the section. Recompute CRC the last time only.
        (*it)->setLastSectionNumber(last_section_number, false);
        (*it)->setUInt8(4, segment_last_section_number, false);
        (*it)->setUInt8(5, last_table_id, true);
    }

    // Return the list of output sections.
    sections.swap(out_sections);
}

//----------------------------------------------------------------------------
// Modify an EIT-schedule section to make it standalone, outside any table.
//----------------------------------------------------------------------------

bool ts::EIT::SetStandaloneSchedule(Section& section)
{
    if (!section.isValid() || !IsSchedule(section.tableId()) || (section.sectionNumber() == 0 && section.lastSectionNumber() == 0)) {
        // Nothing to modify.
        return false;
    }
    else {
        // Update the fields in the section. Recompute CRC the last time only.
        section.setSectionNumber(0, false);
        section.setLastSectionNumber(0, false);
        section.setUInt8(4, 0, false);  // segment_last_section_number
        section.setUInt8(5, section.tableId(), true);  // last_table_id
        return true;
    }
}
