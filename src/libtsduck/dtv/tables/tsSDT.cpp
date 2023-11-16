//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsFatal.h"

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

ts::SDT::ServiceEntry::ServiceEntry(const AbstractTable* table) :
    EntryWithDescriptors(table)
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
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::SDT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the SDT section is limited to 1024 bytes in ETSI EN 300 468.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
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
    for (auto& it : services) {
        const UString service_name(it.second.serviceName(duck));
        if ((exact_match && service_name == name) || (!exact_match && service_name.similar(name))) {
            service_id = it.first;
            return true;
        }
    }

    // Service not found
    service_id = 0;
    return false;
}

bool ts::SDT::findService(DuckContext& duck, Service& service, bool exact_match) const
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
// Collect all informations about the service.
//----------------------------------------------------------------------------

void ts::SDT::ServiceEntry::updateService(DuckContext& duck, Service& service) const
{
    service.setRunningStatus(running_status);
    service.setCAControlled(CA_controlled);
    service.setEITpfPresent(EITpf_present);
    service.setEITsPresent(EITs_present);

    // Look for more information in the descriptors of the service entry.
    ServiceDescriptor srv_desc;
    if (locateServiceDescriptor(duck, srv_desc)) {
        service.setName(srv_desc.service_name);
        service.setProvider(srv_desc.provider_name);
        service.setTypeDVB(srv_desc.service_type);
    }
}


//----------------------------------------------------------------------------
// Collect all informations about all services in the SDT.
//----------------------------------------------------------------------------

void ts::SDT::updateServices(DuckContext& duck, ServiceList& slist) const
{
    // Loop on all services in the SDT.
    for (auto& sdt_it : services) {

        // Service id is the index in the service map.
        const uint16_t service_id = sdt_it.first;
        const ServiceEntry& service(sdt_it.second);

        // Try to find an existing matching service. The service id must match.
        // The TS is and orig. netw. id must either not exist or match.
        auto srv = slist.begin();
        while (srv != slist.end() && (!srv->hasId(service_id) || (srv->hasTSId() && !srv->hasTSId(ts_id)) || (srv->hasONId() && !srv->hasONId(onetw_id)))) {
            ++srv;
        }
        if (srv == slist.end()) {
            // Service was not found, create one at end of list.
            srv = slist.emplace(srv, service_id);
        }

        // Now fill the service with known information.
        srv->setTSId(ts_id);
        srv->setONId(onetw_id);
        service.updateService(duck, *srv);
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
    buf.skipReservedBits(8);

    // Get services description
    while (buf.canRead()) {
        ServiceEntry& serv(services[buf.getUInt16()]);
        buf.skipReservedBits(6);
        serv.EITs_present = buf.getBool();
        serv.EITpf_present = buf.getBool();
        buf.getBits(serv.running_status, 3);
        serv.CA_controlled = buf.getBool();
        buf.getDescriptorListWithLength(serv.descs);
    }
}


//---------------------------------------------------------------s-------------
// Serialization
//----------------------------------------------------------------------------

void ts::SDT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(onetw_id);
    buf.putUInt8(0xFF);
    buf.pushState();

    // Minimum size of a section: fixed part.
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all services
    for (auto& it : services) {

        // Binary size of the service entry.
        const size_t entry_size = 5 + it.second.descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, buf);
        }

        // Insert service entry
        buf.putUInt16(it.first); // service_id
        buf.putBits(0xFF, 6);
        buf.putBit(it.second.EITs_present);
        buf.putBit(it.second.EITpf_present);
        buf.putBits(it.second.running_status, 3);
        buf.putBit(it.second.CA_controlled);
        buf.putPartialDescriptorListWithLength(it.second.descs);
    }
}


//----------------------------------------------------------------------------
// Locate and deserialize the first DVB service_descriptor inside the entry.
//----------------------------------------------------------------------------

bool ts::SDT::ServiceEntry::locateServiceDescriptor(DuckContext& duck, ServiceDescriptor& desc) const
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

uint8_t ts::SDT::ServiceEntry::serviceType(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.service_type : 0; // 0 is a "reserved" service_type value
}

ts::UString ts::SDT::ServiceEntry::providerName(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.provider_name : UString();
}

ts::UString ts::SDT::ServiceEntry::serviceName(DuckContext& duck) const
{
    ServiceDescriptor sd;
    return locateServiceDescriptor(duck, sd) ? sd.service_name : UString();
}


//----------------------------------------------------------------------------
// Set a string value (typically provider or service name).
//----------------------------------------------------------------------------

void ts::SDT::ServiceEntry::setString(DuckContext& duck, UString ServiceDescriptor::* field, const UString& value, uint8_t service_type)
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

void ts::SDT::ServiceEntry::setType(uint8_t service_type)
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

void ts::SDT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Transport Stream Id: %d (0x%<X)", {section.tableIdExtension()}) << std::endl;
    disp << margin << UString::Format(u"Original Network Id: %d (0x%<X)", {buf.getUInt16()}) << std::endl;
    buf.skipReservedBits(8);

    // Services description
    while (buf.canRead()) {
        disp << margin << UString::Format(u"Service Id: %d (0x%<X)", {buf.getUInt16()});
        buf.skipReservedBits(6);
        disp << ", EITs: " << UString::YesNo(buf.getBool());
        disp << ", EITp/f: " << UString::YesNo(buf.getBool());
        const uint8_t running_status = buf.getBits<uint8_t>(3);
        disp << ", CA mode: " << (buf.getBool() ? "controlled" : "free") << std::endl;
        disp << margin << "Running status: " << names::RunningStatus(running_status) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
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

    for (auto& it : services) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.first, true);
        e->setBoolAttribute(u"EIT_schedule", it.second.EITs_present);
        e->setBoolAttribute(u"EIT_present_following", it.second.EITpf_present);
        e->setBoolAttribute(u"CA_mode", it.second.CA_controlled);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it.second.running_status);
        it.second.descs.toXML(duck, e);
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
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute(onetw_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        element->getChildren(children, u"service");

    setActual(actual);

    for (size_t index = 0; ok && index < children.size(); ++index) {
        uint16_t id = 0;
        int rs = 0;
        ok = children[index]->getIntAttribute(id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
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
