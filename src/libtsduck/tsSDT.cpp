//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  Representation of a Service Description Table (SDT)
//
//----------------------------------------------------------------------------

#include "tsSDT.h"
#include "tsRST.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"SDT"
#define MY_STD ts::STD_DVB

TS_XML_TABLE_FACTORY(ts::SDT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::SDT, ts::TID_SDT_ACT, MY_STD);
TS_ID_TABLE_FACTORY(ts::SDT, ts::TID_SDT_OTH, MY_STD);
TS_ID_SECTION_DISPLAY(ts::SDT::DisplaySection, ts::TID_SDT_ACT);
TS_ID_SECTION_DISPLAY(ts::SDT::DisplaySection, ts::TID_SDT_OTH);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SDT::SDT(bool is_actual_, uint8_t version_, bool is_current_, uint16_t ts_id_, uint16_t onetw_id_) :
    AbstractLongTable(TID(is_actual_ ? TID_SDT_ACT : TID_SDT_OTH), MY_XML_NAME, MY_STD, version_, is_current_),
    ts_id(ts_id_),
    onetw_id(onetw_id_),
    services(this)
{
    _is_valid = true;
}

ts::SDT::SDT(const BinaryTable& table, const DVBCharset* charset) :
    SDT()
{
    deserialize(table, charset);
}

ts::SDT::SDT(const SDT& other) :
    AbstractLongTable(other),
    ts_id(other.ts_id),
    onetw_id(other.onetw_id),
    services(this, other.services)
{
}


//----------------------------------------------------------------------------
// Search a service by name.
//----------------------------------------------------------------------------

bool ts::SDT::findService(const UString& name, uint16_t& service_id, bool exact_match) const
{
    for (ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it) {
        const UString service_name(it->second.serviceName());
        if ((exact_match && service_name == name) || (!exact_match && service_name.similar(name))) {
            service_id = it->first;
            return true;
        }
    }

    // Service not found
    service_id = 0;
    return false;
}

bool ts::SDT::findService(ts::Service& service, bool exact_match) const
{
    uint16_t service_id = 0;
    if (!service.hasName() || !findService(service.getName(), service_id, exact_match)) {
        return false;
    }
    else {
        service.setId(service_id);
        return true;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SDT::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    ts_id = 0;
    onetw_id = 0;
    services.clear();

    if (!table.isValid()) {
        return;
    }

    // Check table id: SDT Actual or Other
    if ((_table_id = table.tableId()) != TID_SDT_ACT && _table_id != TID_SDT_OTH) {
        return;
    }

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect (*table.sectionAt(si));

        // Abort if not expected table
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        ts_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data (sect.payload());
        size_t remain (sect.payloadSize());

        // Get original_network_id (should be identical on all sections).
        // Note that there is one trailing reserved byte.
        if (remain < 3) {
            return;
        }
        onetw_id = GetUInt16 (data);
        data += 3;
        remain -= 3;

        // Get services description
        while (remain >= 5) {
            uint16_t service_id = GetUInt16 (data);
            Service& serv (services[service_id]);
            serv.EITs_present = (data[2] & 0x02) != 0;
            serv.EITpf_present = (data[2] & 0x01) != 0;
            serv.running_status = data[3] >> 5;
            serv.CA_controlled = (data[3] & 0x10) != 0;
            size_t info_length (GetUInt16 (data + 3) & 0x0FFF);
            data += 5;
            remain -= 5;
            info_length = std::min (info_length, remain);
            serv.descs.add (data, info_length);
            data += info_length;
            remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Section number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::SDT::addSection(BinaryTable& table,
                         int& section_number,
                         uint8_t* payload,
                         uint8_t*& data,
                         size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,    // is_private_section
                                 ts_id,   // tid_ext
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number),   //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    // Restart after constant part of payload (3 bytes).
    remain += data - payload - 3;
    data = payload + 3;
    section_number++;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SDT::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the sections
    uint8_t payload[MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    int section_number(0);
    uint8_t* data(payload);
    size_t remain(sizeof(payload));

    // Add original_network_id and one reserved byte at beginning of the
    // payload (will remain identical in all sections).
    PutUInt16(data, onetw_id);
    data[2] = 0xFF;
    data += 3;
    remain -= 3;

    // Add all services
    for (ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it) {

        const uint16_t service_id(it->first);
        const Service& serv(it->second);

        // If we cannot at least add the fixed part, open a new section
        if (remain < 5) {
            addSection(table, section_number, payload, data, remain);
        }

        // Insert the characteristics of the service. When the section is
        // not large enough to hold the entire descriptor list, open a
        // new section for the rest of the descriptors. In that case, the
        // common properties of the service must be repeated.
        bool starting(true);
        size_t start_index(0);

        while (starting || start_index < serv.descs.count()) {

            // If we are at the beginning of a service description,
            // make sure that the entire service description fits in
            // the section. If it does not fit, start a new section.
            // Note that huge service descriptions may not fit into
            // one section. In that case, the service description
            // will span two sections later.
            if (starting && 5 + serv.descs.binarySize() > remain) {
                addSection(table, section_number, payload, data, remain);
            }

            starting = false;

            // Insert common characteristics of the service
            assert(remain >= 5);
            PutUInt16(data, service_id);
            data[2] = 0xFC | (serv.EITs_present ? 0x02 : 0x00) | (serv.EITpf_present ? 0x01 : 0x00);
            data += 3;
            remain -= 3;

            // Insert descriptors (all or some).
            uint8_t* flags(data);
            start_index = serv.descs.lengthSerialize(data, remain, start_index);

            // The following fields are inserted in the 4 "reserved" bits
            // of the descriptor_loop_length.
            flags[0] = (flags[0] & 0x0F) | (serv.running_status << 5) | (serv.CA_controlled ? 0x10 : 0x00);

            // If not all descriptors were written, the section is full.
            // Open a new one and continue with this service.
            if (start_index < serv.descs.count()) {
                addSection(table, section_number, payload, data, remain);
            }
        }
    }

    // Add partial section (if there is one)
    if (data > payload + 3 || table.sectionCount() == 0) {
        addSection(table, section_number, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// Default constructor for SDT::Service
//----------------------------------------------------------------------------

ts::SDT::Service::Service(const AbstractTable* table) :
    EntryWithDescriptors(table),
    EITs_present(false),
    EITpf_present(false),
    running_status(0),
    CA_controlled(false)
{
}


//----------------------------------------------------------------------------
// Locate and deserialize the first DVB service_descriptor inside the entry.
//----------------------------------------------------------------------------

bool ts::SDT::Service::locateServiceDescriptor(ServiceDescriptor& desc, const DVBCharset* charset) const
{
    const size_t index = descs.search(DID_SERVICE);

    if (index >= descs.count()) {
        desc.invalidate();
        return false;
    }
    else {
        assert(!descs[index].isNull());
        desc.deserialize(*descs[index], charset);
        return desc.isValid();
    }
}


//----------------------------------------------------------------------------
// Return the service type, service name and provider name (all found from
// the first DVB "service descriptor", if there is one in the list).
//----------------------------------------------------------------------------

uint8_t ts::SDT::Service::serviceType() const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(sd) ? sd.service_type : 0; // 0 is a "reserved" service_type value
}

ts::UString ts::SDT::Service::providerName(const DVBCharset* charset) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(sd, charset) ? sd.provider_name : UString();
}

ts::UString ts::SDT::Service::serviceName(const DVBCharset* charset) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(sd, charset) ? sd.service_name : UString();
}


//----------------------------------------------------------------------------
// Set a string value (typically provider or service name).
//----------------------------------------------------------------------------

void ts::SDT::Service::setString(UString ServiceDescriptor::* field, const UString& value, uint8_t service_type, const DVBCharset* charset)
{
    // Locate the service descriptor
    const size_t index = descs.search(DID_SERVICE);

    if (index >= descs.count()) {
        // No valid service_descriptor, add a new one.
        ServiceDescriptor sd(service_type);
        sd.*field = value;
        DescriptorPtr dp(new Descriptor);
        CheckNonNull(dp.pointer());
        sd.serialize(*dp, charset);
        if (dp->isValid()) {
            descs.add(dp);
        }
    }
    else {
        // Replace service name in existing descriptor
        assert(!descs[index].isNull());
        ServiceDescriptor sd;
        sd.deserialize(*descs[index], charset);
        if (sd.isValid()) {
            sd.*field = value;
            sd.serialize(*descs[index], charset);
        }
    }
}


//----------------------------------------------------------------------------
// Modify the service_descriptor with the new service type.
//----------------------------------------------------------------------------

void ts::SDT::Service::setType(uint8_t service_type)
{
    // Locate the service descriptor
    const size_t index(descs.search(DID_SERVICE));

    if (index >= descs.count() || descs[index]->payloadSize() < 2) {
        // No valid service_descriptor, add a new one.
        ByteBlock data(5);
        data[0] = DID_SERVICE;  // tag
        data[1] = 3;            // descriptor length
        data[2] = service_type;
        data[3] = 0;            // provider name length
        data[4] = 0;            // service name length
        descs.add(DescriptorPtr(new Descriptor(data)));
    }
    else if (descs[index]->payloadSize() > 0) {
        // Replace service type in existing descriptor
        uint8_t* payload = descs[index]->payload();
        payload[0] = service_type;
    }
}


//----------------------------------------------------------------------------
// A static method to display a SDT section.
//----------------------------------------------------------------------------

void ts::SDT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Transport Stream Id: %d (0x%X)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 2) {
        uint16_t nwid = GetUInt16(data);
        strm << margin << UString::Format(u"Original Network Id: %d (0x%04X)", {nwid, nwid}) << std::endl;
        data += 2; size -= 2;
        if (size >= 1) {
            data += 1; size -= 1; // unused byte
        }

        // Loop across all services
        while (size >= 5) {
            uint16_t servid = GetUInt16(data);
            bool eits = (data[2] >> 1) & 0x01;
            bool eitpf = data[2] & 0x01;
            uint16_t length_bytes = GetUInt16(data + 3);
            uint8_t running_status = uint8_t(length_bytes >> 13);
            bool ca_mode = (length_bytes >> 12) & 0x01;
            size_t length = length_bytes & 0x0FFF;
            data += 5; size -= 5;
            if (length > size) {
                length = size;
            }
            strm << margin << UString::Format(u"Service Id: %d (0x%04X)", {servid, servid})
                 << ", EITs: " << UString::YesNo(eits)
                 << ", EITp/f: " << UString::YesNo(eitpf)
                 << ", CA mode: " << (ca_mode ? "controlled" : "free")
                 << std::endl << margin
                 << "Running status: " << names::RunningStatus(running_status)
                 << std::endl;
            display.displayDescriptorList(data, length, indent, section.tableId());
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SDT::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"transport_stream_id", ts_id, true);
    root->setIntAttribute(u"original_network_id", onetw_id, true);
    root->setBoolAttribute(u"actual", isActual());

    for (ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it->first, true);
        e->setBoolAttribute(u"EIT_schedule", it->second.EITs_present);
        e->setBoolAttribute(u"EIT_present_following", it->second.EITpf_present);
        e->setBoolAttribute(u"CA_mode", it->second.CA_controlled);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it->second.running_status);
        it->second.descs.toXML(e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SDT::fromXML(const xml::Element* element)
{
    services.clear();

    xml::ElementVector children;
    bool actual = true;

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(onetw_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        element->getChildren(children, u"service");

    setActual(actual);

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        uint16_t id = 0;
        int rs = 0;
        _is_valid =
            children[index]->getIntAttribute<uint16_t>(id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
            children[index]->getBoolAttribute(services[id].EITs_present, u"EIT_schedule", false, false) &&
            children[index]->getBoolAttribute(services[id].EITpf_present, u"EIT_present_following", false, false) &&
            children[index]->getBoolAttribute(services[id].CA_controlled, u"CA_mode", false, false) &&
            children[index]->getEnumAttribute(rs, RST::RunningStatusNames, u"running_status", false, 0) &&
            services[id].descs.fromXML(children[index]);
        if (_is_valid) {
            services[id].running_status = uint8_t(rs);
        }
    }
}
