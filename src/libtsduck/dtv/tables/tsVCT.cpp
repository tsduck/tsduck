//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVCT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VCT::VCT(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid, xml_name, standards, version_, is_current_),
    channels(this),
    descs(this)
{
}

ts::VCT::VCT(const VCT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    transport_stream_id(other.transport_stream_id),
    channels(this, other.channels),
    descs(this, other.descs)
{
}

ts::VCT::Channel::Channel(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::VCT::tableIdExtension() const
{
    return transport_stream_id;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::VCT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the VCT section is limited to 1024 bytes in ATSC A/65.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::VCT::clearContent()
{
    protocol_version = 0;
    transport_stream_id = 0;
    channels.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Set all known values in a Service object.
//----------------------------------------------------------------------------

void ts::VCT::Channel::updateService(Service& service) const
{
    service.setId(program_number);
    service.setTSId(channel_TSID);
    service.setName(short_name);
    service.setMajorIdATSC(major_channel_number);
    service.setMinorIdATSC(minor_channel_number);
    service.setTypeATSC(service_type);
    service.setCAControlled(access_controlled);
    service.setHidden(hidden);
}


//----------------------------------------------------------------------------
// Search a service by name or id.
//----------------------------------------------------------------------------

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t id, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (id == it->second.program_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(uint16_t major, uint16_t minor, bool same_ts) const
{
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        if (!same_ts || it->second.channel_TSID == transport_stream_id) {
            if (major == it->second.major_channel_number && minor == it->second.minor_channel_number) {
                return it;
            }
        }
    }

    // Service not found
    return channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findService(const UString& name, bool exact_match, bool same_ts) const
{
    // Search using various interpretations of "name".
    Service service(name);
    return findServiceInternal(service, exact_match, same_ts);
}

bool ts::VCT::findService(Service& service, bool exact_match, bool same_ts) const
{
    return findServiceInternal(service, exact_match, same_ts) != channels.end();
}

ts::VCT::ChannelList::const_iterator ts::VCT::findServiceInternal(Service& service, bool exact_match, bool same_ts) const
{
    auto srv = channels.end();
    if (service.hasId()) {
        // Search by service id.
        srv = findService(service.getId(), same_ts);
    }
    else if (service.hasMajorIdATSC() && service.hasMinorIdATSC()) {
        // Search by major.minor id.
        srv = findService(service.getMajorIdATSC(), service.getMinorIdATSC(), same_ts);
    }
    else if (service.hasName()) {
        // Search by service name.
        const UString name(service.getName());
        for (srv = channels.begin(); srv != channels.end(); ++srv) {
            if (!same_ts || srv->second.channel_TSID == transport_stream_id) {
                if ((exact_match && name == srv->second.short_name) || (!exact_match && name.similar(srv->second.short_name))) {
                    break;
                }
            }
        }
    }

    if (srv != channels.end()) {
        // Service found, set known fields.
        srv->second.updateService(service);
    }

    return srv;
}


//----------------------------------------------------------------------------
// Collect all informations about all services in the VCT.
//----------------------------------------------------------------------------

void ts::VCT::updateServices(DuckContext& duck, ServiceList& slist) const
{
    // Loop on all services in the SDT.
    for (const auto& vct_it : channels) {
        const Channel& chan(vct_it.second);

        // Consider only services in this TS.
        if (chan.channel_TSID == transport_stream_id) {
            // Try to find an existing matching service. The service id must match.
            // The TS is and orig. netw. id must either not exist or match.
            auto srv = slist.begin();
            while (srv != slist.end() && (!srv->hasId(chan.program_number) || (srv->hasTSId() && !srv->hasTSId(transport_stream_id)))) {
                ++srv;
            }
            if (srv == slist.end()) {
                // Service was not found, create one at end of list.
                srv = slist.emplace(srv, chan.program_number);
            }

            // Now fill the service with known information.
            chan.updateService(*srv);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    transport_stream_id = section.tableIdExtension();
    protocol_version = buf.getUInt8();

    // Loop on all channel definitions.
    uint8_t num_channels = buf.getUInt8();
    while (!buf.error() && num_channels-- > 0) {

        // Add a new Channel at the end of the list.
        // We do not need to search for a similar channel to extend
        // because A/65 specifies that a channel shall fit in one section.
        Channel& ch(channels.newEntry());

        buf.getUTF16(ch.short_name, 14);
        buf.skipBits(4);
        buf.getBits(ch.major_channel_number, 10);
        buf.getBits(ch.minor_channel_number, 10);
        ch.modulation_mode = buf.getUInt8();
        ch.carrier_frequency = buf.getUInt32();
        ch.channel_TSID = buf.getUInt16();
        ch.program_number = buf.getUInt16();
        buf.getBits(ch.ETM_location, 2);
        ch.access_controlled = buf.getBool();
        ch.hidden = buf.getBool();
        if (_table_id == TID_CVCT) {
            // The following two bits are used in CVCT only.
            ch.path_select = buf.getBit();
            ch.out_of_band = buf.getBool();
        }
        else {
            // Unused field in other forms of VCT.
            buf.skipBits(2);
            ch.path_select = 0;
            ch.out_of_band = false;
        }
        ch.hide_guide = buf.getBool();
        buf.skipBits(3);
        buf.getBits(ch.service_type, 6);
        ch.source_id = buf.getUInt16();

        // Descriptors for this channel (with 10-bit length field).
        buf.getDescriptorListWithLength(ch.descs, 10);
    }

    // Get global descriptor list (with 10-bit length field).
    buf.getDescriptorListWithLength(descs, 10);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt8(protocol_version);

    // Save position before num_channels_in_section. Will be updated at each channel.
    uint8_t num_channels_in_section = 0;
    buf.pushState();
    buf.putUInt8(num_channels_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Loop on channel definitions.
    for (size_t i = 0; !buf.error() && i < channels.size(); ++i) {
        const Channel& ch(channels[i]);

        // Binary size of the channel definition.
        const size_t entry_size = 32 + ch.descs.binarySize();

        // If we are not at the beginning of the channel loop, make sure that the entire
        // channel fits in the section. If it does not fit, start a new section.
        // Take into account at least 2 bytes for the trailing descriptor list.
        if (entry_size + 2 > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create an empty trailing descriptor list.
            buf.putUInt16(0xFC00);
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of num_channels_in_section in the new section.
            buf.putUInt8(num_channels_in_section = 0);
        }

        // Serialize the channel definition.
        buf.putFixedUTF16(ch.short_name, 14);
        buf.putBits(0xFF, 4);
        buf.putBits(ch.major_channel_number, 10);
        buf.putBits(ch.minor_channel_number, 10);
        buf.putUInt8(ch.modulation_mode);
        buf.putUInt32(ch.carrier_frequency);
        buf.putUInt16(ch.channel_TSID);
        buf.putUInt16(ch.program_number);
        buf.putBits(ch.ETM_location, 2);
        buf.putBit(ch.access_controlled);
        buf.putBit(ch.hidden);
        buf.putBit(_table_id != TID_CVCT ? 1 : ch.path_select);
        buf.putBit(_table_id != TID_CVCT || ch.out_of_band);
        buf.putBit(ch.hide_guide);
        buf.putBits(0xFF, 3);
        buf.putBits(ch.service_type, 6);
        buf.putUInt16(ch.source_id);

        // Descriptors for this channel (with 10-bit length field).
        // Temporarily remove 2 trailing bytes for minimal additional_descriptor loop.
        buf.pushWriteSize(buf.size() - 2);
        buf.putPartialDescriptorListWithLength(ch.descs, 0, NPOS, 10);
        buf.popState();

        // Now increment the field num_channels_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++num_channels_in_section);
        buf.popState();
        buf.swapState();
    }

    // There should be at least two remaining bytes if there was no error.
    assert(buf.error() || buf.remainingWriteBytes() >= 2);

    // Serialize additional_descriptor loop. May overflow on additional sections.
    size_t start = 0;
    while (!buf.error()) {
        start = buf.putPartialDescriptorListWithLength(descs, start, NPOS, 10);
        if (start < descs.size()) {
            // Too many descriptors to fit in this section, flush current section.
            addOneSection(table, buf);
            // We are at the position of num_channels_in_section in the new section.
            // There is no channel entry in this section.
            buf.putUInt8(0);
        }
        else {
            // Descriptor list completed.
            break;
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a VCT section.
//----------------------------------------------------------------------------

void ts::VCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    uint16_t num_channels = 0;

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()});
        disp << UString::Format(u", number of channels: %d", {num_channels = buf.getUInt8()}) << std::endl;
    }

    // Loop on all channel definitions.
    while (!buf.error() && num_channels-- > 0) {

        if (!buf.canReadBytes(32)) {
            buf.setUserError();
            break;
        }

        const UString name(buf.getUTF16(14));
        buf.skipBits(4);
        disp << margin << "- Channel " << buf.getBits<uint16_t>(10);
        disp << "." << buf.getBits<uint16_t>(10);
        disp << ", short name: \"" << name << "\"" << std::endl;
        disp << margin << "  Modulation: " << NameFromDTV(u"ATSCModulationModes", buf.getUInt8());
        disp << UString::Format(u", frequency: %'d", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"  TS id: 0x%X (%<d)", {buf.getUInt16()});
        disp << UString::Format(u", program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"  ETM location: %d", {buf.getBits<uint8_t>(2)});
        disp << ", access controlled: " << UString::YesNo(buf.getBool()) << std::endl;
        const bool hidden = buf.getBool();
        if (section.tableId() == TID_CVCT) {
            // The following two bits are used in CVCT only.
            disp << margin << UString::Format(u"  Path select: %d", {buf.getBit()});
            disp << ", out of band: " << UString::YesNo(buf.getBool()) << std::endl;
        }
        else {
            buf.skipBits(2);
        }
        disp << margin << "  Hidden: " << UString::YesNo(hidden) << ", hide guide: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipBits(3);
        disp << margin << "  Service type: " << NameFromDTV(u"ATSCServiceType", buf.getBits<uint8_t>(6));
        disp << UString::Format(u", source id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;

        disp.displayDescriptorListWithLength(section, buf, margin + u"  ", UString(), UString(), 10);
    }

    // Common descriptors.
    disp.displayDescriptorListWithLength(section, buf, margin, u"Additional descriptors:", UString(), 10);
}


//----------------------------------------------------------------------------
// XML values for modulation mode and service_type.
//----------------------------------------------------------------------------

const ts::Enumeration ts::VCT::ModulationModeEnum({
    {u"analog",  0x01},
    {u"64-QAM",  0x02},
    {u"256-QAM", 0x03},
    {u"8-VSB",   0x04},
    {u"16-VSB",  0x05},
});

const ts::Enumeration ts::VCT::ServiceTypeEnum({
    {u"analog",   0x01},
    {u"dtv",      0x02},
    {u"audio",    0x03},
    {u"data",     0x04},
    {u"software", 0x05},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (const auto& it : channels) {
        xml::Element* e = root->addElement(u"channel");
        e->setAttribute(u"short_name", it.second.short_name);
        e->setIntAttribute(u"major_channel_number", it.second.major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it.second.minor_channel_number, false);
        e->setEnumAttribute(ModulationModeEnum, u"modulation_mode", it.second.modulation_mode);
        e->setIntAttribute(u"carrier_frequency", it.second.carrier_frequency, false);
        e->setIntAttribute(u"channel_TSID", it.second.channel_TSID, true);
        e->setIntAttribute(u"program_number", it.second.program_number, true);
        e->setIntAttribute(u"ETM_location", it.second.ETM_location, false);
        e->setBoolAttribute(u"access_controlled", it.second.access_controlled);
        e->setBoolAttribute(u"hidden", it.second.hidden);
        if (_table_id == TID_CVCT) {
            // CVCT-specific fields.
            e->setIntAttribute(u"path_select", it.second.path_select, false);
            e->setBoolAttribute(u"out_of_band", it.second.out_of_band);
        }
        e->setBoolAttribute(u"hide_guide", it.second.hide_guide);
        e->setEnumAttribute(ServiceTypeEnum, u"service_type", it.second.service_type);
        e->setIntAttribute(u"source_id", it.second.source_id, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        descs.fromXML(duck, children, element, u"channel");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Channel at the end of the list.
        Channel& ch(channels.newEntry());
        ok = children[index]->getAttribute(ch.short_name, u"short_name", true, UString(), 0, 7) &&
             children[index]->getIntAttribute(ch.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntAttribute(ch.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF) &&
             children[index]->getIntEnumAttribute(ch.modulation_mode, ModulationModeEnum, u"modulation_mode", true) &&
             children[index]->getIntAttribute(ch.carrier_frequency, u"carrier_frequency", false, 0) &&
             children[index]->getIntAttribute(ch.channel_TSID, u"channel_TSID", true) &&
             children[index]->getIntAttribute(ch.program_number, u"program_number", true) &&
             children[index]->getIntAttribute(ch.ETM_location, u"ETM_location", false, 0, 0x00, 0x03) &&
             children[index]->getBoolAttribute(ch.access_controlled, u"access_controlled", false, false) &&
             children[index]->getBoolAttribute(ch.hidden, u"hidden", false, false) &&
             children[index]->getBoolAttribute(ch.hide_guide, u"hide_guide", false, false) &&
             children[index]->getIntEnumAttribute(ch.service_type, ServiceTypeEnum, u"service_type", false, ATSC_STYPE_DTV) &&
             children[index]->getIntAttribute(ch.source_id, u"source_id", true) &&
             ch.descs.fromXML(duck, children[index]);

        if (ok && _table_id == TID_CVCT) {
            // CVCT-specific fields.
            ok = children[index]->getIntAttribute(ch.path_select, u"path_select", false, 0, 0, 1) &&
                 children[index]->getBoolAttribute(ch.out_of_band, u"out_of_band", false, false);
        }
    }
    return ok;
}
