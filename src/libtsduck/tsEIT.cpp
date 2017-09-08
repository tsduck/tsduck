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
//  Representation of an Event Information Table (EIT)
//
//----------------------------------------------------------------------------

#include "tsEIT.h"
#include "tsFormat.h"
#include "tsNames.h"
#include "tsBCD.h"
#include "tsMJD.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_TABLE_FACTORY(ts::EIT, "EIT");
TS_ID_TABLE_RANGE_FACTORY(ts::EIT, ts::TID_EIT_MIN, ts::TID_EIT_MAX);
TS_ID_SECTION_RANGE_DISPLAY(ts::EIT::DisplaySection, ts::TID_EIT_MIN, ts::TID_EIT_MAX);


//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------

ts::EIT::EIT(bool is_actual_,
             bool is_pf_,
             uint8_t eits_index_,
             uint8_t version_,
             bool is_current_,
             uint16_t service_id_,
             uint16_t ts_id_,
             uint16_t onetw_id_) :
    AbstractLongTable(ComputeTableId(is_actual_, is_pf_, eits_index_), "EIT", version_, is_current_),
    service_id(service_id_),
    ts_id(ts_id_),
    onetw_id(onetw_id_),
    segment_last(0),
    last_table_id(_table_id),
    events()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::EIT::EIT(const BinaryTable& table) :
    AbstractLongTable(TID_EIT_PF_ACT, "EIT"),  // TID will be updated by deserialize()
    service_id(0),
    ts_id(0),
    onetw_id(0),
    segment_last(0),
    last_table_id(0),
    events()
{
    deserialize(table);
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


//----------------------------------------------------------------------------
// Check if this is an "actual" EIT.
//----------------------------------------------------------------------------

bool ts::EIT::isActual() const
{
    return _table_id == TID_EIT_PF_ACT || (_table_id >= TID_EIT_S_ACT_MIN &&_table_id <= TID_EIT_S_ACT_MAX);
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

void ts::EIT::deserialize(const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;
    service_id = 0;
    ts_id = 0;
    onetw_id = 0;
    segment_last = 0;
    last_table_id = _table_id;
    events.clear();

    if (!table.isValid()) {
        return;
    }

    // Check table id.
    if ((_table_id = table.tableId()) < TID_EIT_MIN || _table_id > TID_EIT_MAX) {
        return;
    }

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Abort if not expected table
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        service_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        if (remain < 6) {
            return;
        }

        ts_id = GetUInt16(data);
        onetw_id = GetUInt16(data + 2);
        segment_last = data[4];
        last_table_id = data[5];
        data += 6;
        remain -= 6;

        // Get services description
        while (remain >= 12) {
            uint16_t event_id = GetUInt16(data);
            Event& event(events[event_id]);
            DecodeMJD(data + 2, 5, event.start_time);
            const int hour = DecodeBCD(data[7]);
            const int min = DecodeBCD(data[8]);
            const int sec = DecodeBCD(data[9]);
            event.duration = ((hour * 24) + min) * 60 + sec;
            event.running_status = (data[10] >> 5) & 0x07;
            event.CA_controlled = (data[10] >> 4) & 0x01;

            size_t info_length = GetUInt16(data + 10) & 0x0FFF;
            data += 12;
            remain -= 12;

            info_length = std::min(info_length, remain);
            event.descs.add(data, info_length);
            data += info_length;
            remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EIT::serialize(BinaryTable& table) const
{
    //@@ TODO
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Session number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::EIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,         // is_private_section
                                 service_id,   // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

 
    // Reinitialize pointers.
    // Restart after constant part of payload (3 bytes).
    remain += data - payload - 3;
    data = payload + 3;
    section_number++;
}


//----------------------------------------------------------------------------
// Event description constructor
//----------------------------------------------------------------------------

ts::EIT::Event::Event() :
    start_time(),
    duration(0),
    running_status(0),
    CA_controlled(false),
    descs()
{
}


//----------------------------------------------------------------------------
// A static method to display an EIT section.
//----------------------------------------------------------------------------

void ts::EIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    const uint16_t sid = section.tableIdExtension();

    strm << margin << Format("Service Id: %d (0x%04X)", int(sid), int(sid)) << std::endl;

    if (size >= 6) {
        uint16_t tsid = GetUInt16(data);
        uint16_t onid = GetUInt16(data + 2);
        uint8_t seg_last = data[4];
        uint8_t last_tid = data[5];
        data += 6; size -= 6;

        strm << margin << Format("TS Id: %d (0x%04X)", int(tsid), int(tsid)) << std::endl
             << margin << Format("Original Network Id: %d (0x%04X)", int(onid), int(onid)) << std::endl
             << margin << Format("Segment last section: %d (0x%02X)", int(seg_last), int(seg_last)) << std::endl
             << margin << Format("Last Table Id: %d (0x%02X), ", int(last_tid), int(last_tid))
             << names::TID(last_tid, CAS_OTHER) << std::endl;
    }

    while (size >= 12) {
        uint16_t evid = GetUInt16(data);
        Time start;
        DecodeMJD(data + 2, 5, start);
        int hour = DecodeBCD(data[7]);
        int min = DecodeBCD(data[8]);
        int sec = DecodeBCD(data[9]);
        uint8_t run = (data[10] >> 5) & 0x07;
        uint8_t ca_mode = (data[10] >> 4) & 0x01;
        size_t loop_length = GetUInt16(data + 10) & 0x0FFF;
        data += 12; size -= 12;
        if (loop_length > size) {
            loop_length = size;
        }
        strm << margin << Format("Event Id: %d (0x%04X)", int(evid), int(evid)) << std::endl
             << margin << "Start UTC: " << start.format(Time::DATE | Time::TIME) << std::endl
             << margin << Format("Duration: %02d:%02d:%02d", hour, min, sec) << std::endl
             << margin << "Running status: " << names::RunningStatus(run) << std::endl
             << margin << "CA mode: " << (ca_mode ? "controlled" : "free") << std::endl;
        display.displayDescriptorList(data, loop_length, indent, section.tableId());
        data += loop_length; size -= loop_length;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::EIT::toXML(XML& xml, XML::Element* parent) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::EIT::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
