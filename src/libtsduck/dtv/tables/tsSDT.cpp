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

#include "tsSDT.h"
#include "tsRST.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"SDT"
#define MY_CLASS ts::SDT
#define MY_PID ts::PID_SDT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {ts::TID_SDT_ACT, ts::TID_SDT_OTH}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SDT::SDT(bool is_actual_, uint8_t version_, bool is_current_, uint16_t ts_id_, uint16_t onetw_id_) :
    AbstractLongTable(TID(is_actual_ ? TID_SDT_ACT : TID_SDT_OTH), MY_XML_NAME, MY_STD, version_, is_current_),
    ts_id(ts_id_),
    onetw_id(onetw_id_),
    services(this)
{
}

ts::SDT::SDT(DuckContext& duck, const BinaryTable& table) :
    SDT()
{
    deserialize(duck, table);
}

ts::SDT::SDT(const SDT& other) :
    AbstractLongTable(other),
    ts_id(other.ts_id),
    onetw_id(other.onetw_id),
    services(this, other.services)
{
}

ts::SDT::Service::Service(const AbstractTable* table) :
    EntryWithDescriptors(table),
    EITs_present(false),
    EITpf_present(false),
    running_status(0),
    CA_controlled(false)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::SDT::tableIdExtension() const
{
    return ts_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::SDT::clearContent()
{
    ts_id = 0;
    onetw_id = 0;
    services.clear();
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::SDT::isValidTableId(TID tid) const
{
    return tid == TID_SDT_ACT || tid == TID_SDT_OTH;
}


//----------------------------------------------------------------------------
// Search a service by name.
//----------------------------------------------------------------------------

bool ts::SDT::findService(DuckContext& duck, const UString& name, uint16_t& service_id, bool exact_match) const
{
    for (ServiceMap::const_iterator it = services.begin(); it != services.end(); ++it) {
        const UString service_name(it->second.serviceName(duck));
        if ((exact_match && service_name == name) || (!exact_match && service_name.similar(name))) {
            service_id = it->first;
            return true;
        }
    }

    // Service not found
    service_id = 0;
    return false;
}

bool ts::SDT::findService(DuckContext& duck, ts::Service& service, bool exact_match) const
{
    uint16_t service_id = 0;
    if (!service.hasName() || !findService(duck, service.getName(), service_id, exact_match)) {
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

void ts::SDT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get fixed part.
    ts_id = section.tableIdExtension();
    onetw_id = buf.getUInt16();
    buf.skipBits(8);

    // Get services description
    while (!buf.error() && !buf.endOfRead()) {
        Service& serv(services[buf.getUInt16()]);
        buf.skipBits(6);
        serv.EITs_present = buf.getBit() != 0;
        serv.EITpf_present = buf.getBit() != 0;
        serv.running_status = buf.getBits<uint8_t>(3);
        serv.CA_controlled = buf.getBit() != 0;
        buf.getDescriptorListWithLength(serv.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SDT::serializePayload(BinaryTable& table, PSIBuffer& payload) const
{
    // Fixed part, to be repeated on all sections.
    payload.putUInt16(onetw_id);
    payload.putUInt8(0xFF);
    payload.pushReadWriteState();

    // Minimum size of a section: fixed part .
    constexpr size_t payload_min_size = 3;

    // Add all services
    for (auto it = services.begin(); it != services.end(); ++it) {

        // Binary size of the service entry.
        const size_t entry_size = 5 + it->second.descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > payload.remainingWriteBytes() && payload.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, payload);
        }

        // Insert service entry
        payload.putUInt16(it->first); // service_id
        payload.putBits(0xFF, 6);
        payload.putBit(it->second.EITs_present);
        payload.putBit(it->second.EITpf_present);
        payload.putBits(it->second.running_status, 3);
        payload.putBit(it->second.CA_controlled);
        payload.putPartialDescriptorListWithLength(it->second.descs);
    }
}


//----------------------------------------------------------------------------
// Locate and deserialize the first DVB service_descriptor inside the entry.
//----------------------------------------------------------------------------

bool ts::SDT::Service::locateServiceDescriptor(DuckContext& duck, ServiceDescriptor& desc) const
{
    const size_t index = descs.search(DID_SERVICE);

    if (index >= descs.count()) {
        desc.invalidate();
        return false;
    }
    else {
        assert(!descs[index].isNull());
        desc.deserialize(duck, *descs[index]);
        return desc.isValid();
    }
}


//----------------------------------------------------------------------------
// Return the service type, service name and provider name (all found from
// the first DVB "service descriptor", if there is one in the list).
//----------------------------------------------------------------------------

uint8_t ts::SDT::Service::serviceType(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.service_type : 0; // 0 is a "reserved" service_type value
}

ts::UString ts::SDT::Service::providerName(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.provider_name : UString();
}

ts::UString ts::SDT::Service::serviceName(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.service_name : UString();
}


//----------------------------------------------------------------------------
// Set a string value (typically provider or service name).
//----------------------------------------------------------------------------

void ts::SDT::Service::setString(DuckContext& duck, UString ServiceDescriptor::* field, const UString& value, uint8_t service_type)
{
    // Locate the service descriptor
    const size_t index = descs.search(DID_SERVICE);

    if (index >= descs.count()) {
        // No valid service_descriptor, add a new one.
        ServiceDescriptor sd(service_type);
        sd.*field = value;
        DescriptorPtr dp(new Descriptor);
        CheckNonNull(dp.pointer());
        sd.serialize(duck, *dp);
        if (dp->isValid()) {
            descs.add(dp);
        }
    }
    else {
        // Replace service name in existing descriptor
        assert(!descs[index].isNull());
        ServiceDescriptor sd;
        sd.deserialize(duck, *descs[index]);
        if (sd.isValid()) {
            sd.*field = value;
            sd.serialize(duck, *descs[index]);
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
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    PSIBuffer buf(duck, section.payload(), section.payloadSize());

    // Fixed part.
    strm << margin << UString::Format(u"Transport Stream Id: %d (0x%<X)", {section.tableIdExtension()}) << std::endl;
    strm << margin << UString::Format(u"Original Network Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
    buf.skipBits(8);

    // Services description
    while (!buf.error() && !buf.endOfRead()) {
        strm << margin << UString::Format(u"Service Id: %d (0x%<X)", {buf.getUInt16()});
        buf.skipBits(6);
        strm << ", EITs: " << UString::YesNo(buf.getBit() != 0);
        strm << ", EITp/f: " << UString::YesNo(buf.getBit() != 0);
        const uint8_t running_status = buf.getBits<uint8_t>(3);
        strm << ", CA mode: " << (buf.getBit() != 0 ? "controlled" : "free") << std::endl;
        strm << margin << "Running status: " << names::RunningStatus(running_status) << std::endl;
        display.displayDescriptorListWithLength(section, buf, indent);
    }

    display.displayExtraData(buf, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SDT::buildXML(DuckContext& duck, xml::Element* root) const
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
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SDT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool actual = true;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint16_t>(onetw_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        element->getChildren(children, u"service");

    setActual(actual);

    for (size_t index = 0; ok && index < children.size(); ++index) {
        uint16_t id = 0;
        int rs = 0;
        ok = children[index]->getIntAttribute<uint16_t>(id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getBoolAttribute(services[id].EITs_present, u"EIT_schedule", false, false) &&
             children[index]->getBoolAttribute(services[id].EITpf_present, u"EIT_present_following", false, false) &&
             children[index]->getBoolAttribute(services[id].CA_controlled, u"CA_mode", false, false) &&
             children[index]->getEnumAttribute(rs, RST::RunningStatusNames, u"running_status", false, 0) &&
             services[id].descs.fromXML(duck, children[index]);
        if (ok) {
            services[id].running_status = uint8_t(rs);
        }
    }
    return ok;
}
